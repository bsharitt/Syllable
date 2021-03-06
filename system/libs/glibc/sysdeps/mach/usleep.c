/* Copyright (C) 1992, 93, 94, 97, 98 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <errno.h>
#include <mach.h>
#include <sys/time.h>
#include <unistd.h>

/* Sleep USECONDS microseconds, or until a previously set timer goes off.  */
void
usleep (unsigned int useconds)
{
  mach_port_t recv;
  struct timeval before, after;

  recv = __mach_reply_port ();

  if (__gettimeofday (&before, NULL) < 0)
    return;
  (void) __mach_msg (NULL, MACH_RCV_MSG|MACH_RCV_TIMEOUT|MACH_RCV_INTERRUPT,
		     0, 0, recv, (useconds + 999) / 1000, MACH_PORT_NULL);
  __mach_port_destroy (mach_task_self (), recv);
  if (__gettimeofday (&after, NULL) < 0)
    return;
}
