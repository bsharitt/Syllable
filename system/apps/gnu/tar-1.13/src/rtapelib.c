/* Functions for communicating with a remote tape drive.
   Copyright (C) 1988, 1992, 1994, 1996 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* The man page rmt(8) for /etc/rmt documents the remote mag tape protocol
   which rdump and rrestore use.  Unfortunately, the man page is *WRONG*.
   The author of the routines I'm including originally wrote his code just
   based on the man page, and it didn't work, so he went to the rdump source
   to figure out why.  The only thing he had to change was to check for the
   'F' return code in addition to the 'E', and to separate the various
   arguments with \n instead of a space.  I personally don't think that this
   is much of a problem, but I wanted to point it out. -- Arnold Robbins

   Originally written by Jeff Lee, modified some by Arnold Robbins.  Redone
   as a library that can replace open, read, write, etc., by Fred Fish, with
   some additional work by Arnold Robbins.  Modified to make all rmt* calls
   into macros for speed by Jay Fenlason.  Use -DWITH_REXEC for rexec
   code, courtesy of Dan Kegel.  */

#include "system.h"

#include "basename.h"
#include "safe-read.h"

/* Try hard to get EOPNOTSUPP defined.  486/ISC has it in net/errno.h,
   3B2/SVR3 has it in sys/inet.h.  Otherwise, like on MSDOS, use EINVAL.  */

#ifndef EOPNOTSUPP
# if HAVE_NET_ERRNO_H
#  include <net/errno.h>
# endif
# if HAVE_SYS_INET_H
#  include <sys/inet.h>
# endif
# ifndef EOPNOTSUPP
#  define EOPNOTSUPP EINVAL
# endif
#endif

#include <signal.h>

#if HAVE_NETDB_H
# include <netdb.h>
#endif

#include "rmt.h"

/* FIXME: Just to shut up -Wall.  */
int rexec ();

/* Exit status if exec errors.  */
#define EXIT_ON_EXEC_ERROR 128

/* FIXME: Size of buffers for reading and writing commands to rmt.  */
#define COMMAND_BUFFER_SIZE 64

#ifndef RETSIGTYPE
# define RETSIGTYPE void
#endif

/* FIXME: Maximum number of simultaneous remote tape connections.  */
#define MAXUNIT	4

#define	PREAD 0			/* read  file descriptor from pipe() */
#define	PWRITE 1		/* write file descriptor from pipe() */

/* Return the parent's read side of remote tape connection Fd.  */
#define READ_SIDE(Fd) (from_remote[Fd][PREAD])

/* Return the parent's write side of remote tape connection Fd.  */
#define WRITE_SIDE(Fd) (to_remote[Fd][PWRITE])

/* The pipes for receiving data from remote tape drives.  */
static int from_remote[MAXUNIT][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

/* The pipes for sending data to remote tape drives.  */
static int to_remote[MAXUNIT][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

/* Temporary variable used by macros in rmt.h.  */
char *rmt_path__;


/*----------------------------------------------------------------------.
| Close remote tape connection HANDLE, and reset errno to ERRNO_VALUE.  |
`----------------------------------------------------------------------*/

static void
_rmt_shutdown (int handle, int errno_value)
{
  close (READ_SIDE (handle));
  close (WRITE_SIDE (handle));
  READ_SIDE (handle) = -1;
  WRITE_SIDE (handle) = -1;
  errno = errno_value;		/* FIXME: errno should be read-only */
}

/*-------------------------------------------------------------------------.
| Attempt to perform the remote tape command specified in BUFFER on remote |
| tape connection HANDLE.  Return 0 if successful, -1 on error.		   |
`-------------------------------------------------------------------------*/

static int
do_command (int handle, const char *buffer)
{
  size_t length;
  RETSIGTYPE (*pipe_handler) ();

  /* Save the current pipe handler and try to make the request.  */

  pipe_handler = signal (SIGPIPE, SIG_IGN);
  length = strlen (buffer);
  if (full_write (WRITE_SIDE (handle), buffer, length) == length)
    {
      signal (SIGPIPE, pipe_handler);
      return 0;
    }

  /* Something went wrong.  Close down and go home.  */

  signal (SIGPIPE, pipe_handler);
  _rmt_shutdown (handle, EIO);
  return -1;
}

static char *
get_status_string (int handle, char *command_buffer)
{
  char *cursor;
  int counter;

  /* Read the reply command line.  */

  for (counter = 0, cursor = command_buffer;
       counter < COMMAND_BUFFER_SIZE;
       counter++, cursor++)
    {
      if (safe_read (READ_SIDE (handle), cursor, 1) != 1)
	{
	  _rmt_shutdown (handle, EIO);
	  return 0;
	}
      if (*cursor == '\n')
	{
	  *cursor = '\0';
	  break;
	}
    }

  if (counter == COMMAND_BUFFER_SIZE)
    {
      _rmt_shutdown (handle, EIO);
      return 0;
    }

  /* Check the return status.  */

  for (cursor = command_buffer; *cursor; cursor++)
    if (*cursor != ' ')
      break;

  if (*cursor == 'E' || *cursor == 'F')
    {
      errno = atoi (cursor + 1); /* FIXME: errno should be read-only */

      /* Skip the error message line.  */

      /* FIXME: there is better to do than merely ignoring error messages
	 coming from the remote end.  Translate them, too...  */

      {
	char character;

	while (safe_read (READ_SIDE (handle), &character, 1) == 1)
	  if (character == '\n')
	    break;
      }

      if (*cursor == 'F')
	_rmt_shutdown (handle, errno);

      return 0;
    }

  /* Check for mis-synced pipes.  */

  if (*cursor != 'A')
    {
      _rmt_shutdown (handle, EIO);
      return 0;
    }

  /* Got an `A' (success) response.  */

  return cursor + 1;
}

/*----------------------------------------------------------------------.
| Read and return the status from remote tape connection HANDLE.  If an |
| error occurred, return -1 and set errno.			        |
`----------------------------------------------------------------------*/

static long
get_status (int handle)
{
  char command_buffer[COMMAND_BUFFER_SIZE];
  const char *status = get_status_string (handle, command_buffer);
  return status ? atol (status) : -1L;
}

static off_t
get_status_off (int handle)
{
  char command_buffer[COMMAND_BUFFER_SIZE];
  const char *status = get_status_string (handle, command_buffer);

  if (! status)
    return -1;
  else
    {
      /* Parse status, taking care to check for overflow.
	 We can't use standard functions,
	 since off_t might be longer than long.  */

      off_t count = 0;
      int negative;

      for (;  *status == ' ' || *status == '\t';  status++)
	continue;
      
      negative = *status == '-';
      status += negative || *status == '+';
      
      for (;;)
	{
	  int digit = *status++ - '0';
	  if (9 < (unsigned) digit)
	    break;
	  else
	    {
	      off_t c10 = 10 * count;
	      off_t nc = negative ? c10 - digit : c10 + digit;
	      if (c10 / 10 != count || (negative ? c10 < nc : nc < c10))
		return -1;
	      count = nc;
	    }
	}

      return count;
    }
}

#if WITH_REXEC

/*-------------------------------------------------------------------------.
| Execute /etc/rmt as user USER on remote system HOST using rexec.  Return |
| a file descriptor of a bidirectional socket for stdin and stdout.  If	   |
| USER is NULL, use the current username.				   |
| 									   |
| By default, this code is not used, since it requires that the user have  |
| a .netrc file in his/her home directory, or that the application	   |
| designer be willing to have rexec prompt for login and password info.	   |
| This may be unacceptable, and .rhosts files for use with rsh are much	   |
| more common on BSD systems.						   |
`-------------------------------------------------------------------------*/

static int
_rmt_rexec (char *host, char *user)
{
  int saved_stdin = dup (STDIN_FILENO);
  int saved_stdout = dup (STDOUT_FILENO);
  struct servent *rexecserv;
  int result;

  /* When using cpio -o < filename, stdin is no longer the tty.  But the
     rexec subroutine reads the login and the passwd on stdin, to allow
     remote execution of the command.  So, reopen stdin and stdout on
     /dev/tty before the rexec and give them back their original value
     after.  */

  if (freopen ("/dev/tty", "r", stdin) == NULL)
    freopen ("/dev/null", "r", stdin);
  if (freopen ("/dev/tty", "w", stdout) == NULL)
    freopen ("/dev/null", "w", stdout);

  if (rexecserv = getservbyname ("exec", "tcp"), !rexecserv)
    error (EXIT_ON_EXEC_ERROR, 0, _("exec/tcp: Service not available"));

  result = rexec (&host, rexecserv->s_port, user, NULL,
		   "/etc/rmt", (int *) NULL);
  if (fclose (stdin) == EOF)
    error (0, errno, _("stdin"));
  fdopen (saved_stdin, "r");
  if (fclose (stdout) == EOF)
    error (0, errno, _("stdout"));
  fdopen (saved_stdout, "w");

  return result;
}

#endif /* WITH_REXEC */

/*------------------------------------------------------------------------.
| Open a file (a magnetic tape device?) on the system specified in PATH,  |
| as the given user.  PATH has the form `[USER@]HOST:FILE'.  OPEN_MODE is |
| O_RDONLY, O_WRONLY, etc.  If successful, return the remote pipe number  |
| plus BIAS.  REMOTE_SHELL may be overriden.  On error, return -1.	  |
`------------------------------------------------------------------------*/

int
rmt_open__ (const char *path, int open_mode, int bias, const char *remote_shell)
{
  int remote_pipe_number;	/* pseudo, biased file descriptor */
  char *path_copy ;		/* copy of path string */
  char *remote_host;		/* remote host name */
  char *remote_file;		/* remote file name (often a device) */
  char *remote_user;		/* remote user name */

  /* Find an unused pair of file descriptors.  */

  for (remote_pipe_number = 0;
       remote_pipe_number < MAXUNIT;
       remote_pipe_number++)
    if (READ_SIDE (remote_pipe_number) == -1
	&& WRITE_SIDE (remote_pipe_number) == -1)
      break;

  if (remote_pipe_number == MAXUNIT)
    {
      errno = EMFILE;		/* FIXME: errno should be read-only */
      return -1;
    }

  /* Pull apart the system and device, and optional user.  */

  {
    char *cursor;

    path_copy = xstrdup (path);
    remote_host = path_copy;
    remote_user = NULL;
    remote_file = NULL;

    for (cursor = path_copy; *cursor; cursor++)
      switch (*cursor)
	{
	default:
	  break;

	case '@':
	  if (!remote_user)
	    {
	      remote_user = remote_host;
	      *cursor = '\0';
	      remote_host = cursor + 1;
	    }
	  break;

	case ':':
	  if (!remote_file)
	    {
	      *cursor = '\0';
	      remote_file = cursor + 1;
	    }
	  break;
	}
  }

  /* FIXME: Should somewhat validate the decoding, here.  */

  if (remote_user && *remote_user == '\0')
    remote_user = NULL;

#if WITH_REXEC

  /* Execute the remote command using rexec.  */

  READ_SIDE (remote_pipe_number) = _rmt_rexec (remote_host, remote_user);
  if (READ_SIDE (remote_pipe_number) < 0)
    {
      free (path_copy);
      return -1;
    }

  WRITE_SIDE (remote_pipe_number) = READ_SIDE (remote_pipe_number);

#else /* not WITH_REXEC */
  {
    const char *remote_shell_basename;
    pid_t status;

    /* Identify the remote command to be executed.  */

    if (!remote_shell)
      {
#ifdef REMOTE_SHELL
	remote_shell = REMOTE_SHELL;
#else
	errno = EIO;		/* FIXME: errno should be read-only */
	free (path_copy);
	return -1;
#endif
      }
    remote_shell_basename = base_name (remote_shell);
    if (remote_shell_basename)
      remote_shell_basename++;
    else
      remote_shell_basename = remote_shell;

    /* Set up the pipes for the `rsh' command, and fork.  */

    if (pipe (to_remote[remote_pipe_number]) == -1
	|| pipe (from_remote[remote_pipe_number]) == -1)
      {
	free (path_copy);
	return -1;
      }

    status = fork ();
    if (status == -1)
      {
	free (path_copy);
	return -1;
      }

    if (status == 0)
      {
	/* Child.  */

	close (STDIN_FILENO);
	dup (to_remote[remote_pipe_number][PREAD]);
	close (to_remote[remote_pipe_number][PREAD]);
	close (to_remote[remote_pipe_number][PWRITE]);

	close (STDOUT_FILENO);
	dup (from_remote[remote_pipe_number][PWRITE]);
	close (from_remote[remote_pipe_number][PREAD]);
	close (from_remote[remote_pipe_number][PWRITE]);

#if !MSDOS
	setuid (getuid ());
	setgid (getgid ());
#endif

	if (remote_user)
	  execl (remote_shell, remote_shell_basename, remote_host,
		 "-l", remote_user, "/etc/rmt", (char *) 0);
	else
	  execl (remote_shell, remote_shell_basename, remote_host,
		 "/etc/rmt", (char *) 0);

	/* Bad problems if we get here.  */

	/* In a previous version, _exit was used here instead of exit.  */
	error (EXIT_ON_EXEC_ERROR, errno, _("Cannot execute remote shell"));
      }

    /* Parent.  */

    close (from_remote[remote_pipe_number][PWRITE]);
    close (to_remote[remote_pipe_number][PREAD]);
  }
#endif /* not WITH_REXEC */

  /* Attempt to open the tape device.  */

  {
    char command_buffer[COMMAND_BUFFER_SIZE];

    sprintf (command_buffer, "O%s\n%d\n", remote_file, open_mode);
    if (do_command (remote_pipe_number, command_buffer) == -1
	|| get_status (remote_pipe_number) == -1)
      {
	_rmt_shutdown (remote_pipe_number, errno);
	free (path_copy);
	return -1;
      }
  }

  free (path_copy);
  return remote_pipe_number + bias;
}

/*----------------------------------------------------------------.
| Close remote tape connection HANDLE and shut down.  Return 0 if |
| successful, -1 on error.					  |
`----------------------------------------------------------------*/

int
rmt_close__ (int handle)
{
  int status;

  if (do_command (handle, "C\n") == -1)
    return -1;

  status = get_status (handle);
  _rmt_shutdown (handle, errno);
  return status;
}

/*-------------------------------------------------------------------------.
| Read up to LENGTH bytes into BUFFER from remote tape connection HANDLE.  |
| Return the number of bytes read on success, -1 on error.		   |
`-------------------------------------------------------------------------*/

ssize_t
rmt_read__ (int handle, char *buffer, size_t length)
{
  char command_buffer[COMMAND_BUFFER_SIZE];
  ssize_t status, rlen;
  size_t counter;

  sprintf (command_buffer, "R%lu\n", (unsigned long) length);
  if (do_command (handle, command_buffer) == -1
      || (status = get_status (handle)) == -1)
    return -1;

  for (counter = 0; counter < status; counter += rlen, buffer += rlen)
    {
      rlen = safe_read (READ_SIDE (handle), buffer, status - counter);
      if (rlen <= 0)
	{
	  _rmt_shutdown (handle, EIO);
	  return -1;
	}
    }

  return status;
}

/*-------------------------------------------------------------------------.
| Write LENGTH bytes from BUFFER to remote tape connection HANDLE.  Return |
| the number of bytes written on success, -1 on error.			   |
`-------------------------------------------------------------------------*/

ssize_t
rmt_write__ (int handle, char *buffer, size_t length)
{
  char command_buffer[COMMAND_BUFFER_SIZE];
  RETSIGTYPE (*pipe_handler) ();

  sprintf (command_buffer, "W%lu\n", (unsigned long) length);
  if (do_command (handle, command_buffer) == -1)
    return -1;

  pipe_handler = signal (SIGPIPE, SIG_IGN);
  if (full_write (WRITE_SIDE (handle), buffer, length) == length)
    {
      signal (SIGPIPE, pipe_handler);
      return get_status (handle);
    }

  /* Write error.  */

  signal (SIGPIPE, pipe_handler);
  _rmt_shutdown (handle, EIO);
  return -1;
}

/*------------------------------------------------------------------------.
| Perform an imitation lseek operation on remote tape connection HANDLE.  |
| Return the new file offset if successful, -1 if on error.		  |
`------------------------------------------------------------------------*/

off_t
rmt_lseek__ (int handle, off_t offset, int whence)
{
  char command_buffer[COMMAND_BUFFER_SIZE];
  char operand_buffer[UINTMAX_STRSIZE_BOUND];
  uintmax_t u = offset < 0 ? - (uintmax_t) offset : (uintmax_t) offset;
  char *p = operand_buffer + sizeof operand_buffer;

  do
    *--p = '0' + (int) (u % 10);
  while ((u /= 10) != 0);
  if (offset < 0)
    *--p = '-';

  switch (whence)
    {
    case SEEK_SET: whence = 0; break;
    case SEEK_CUR: whence = 1; break;
    case SEEK_END: whence = 2; break;
    default: abort ();
    }

  sprintf (command_buffer, "L%s\n%d\n", p, whence);

  if (do_command (handle, command_buffer) == -1)
    return -1;

  return get_status_off (handle);
}

/*-----------------------------------------------------------------------.
| Perform a raw tape operation on remote tape connection HANDLE.  Return |
| the results of the ioctl, or -1 on error.				 |
`-----------------------------------------------------------------------*/

int
rmt_ioctl__ (int handle, int operation, char *argument)
{
  switch (operation)
    {
    default:
      errno = EOPNOTSUPP;	/* FIXME: errno should be read-only */
      return -1;

    }
}
