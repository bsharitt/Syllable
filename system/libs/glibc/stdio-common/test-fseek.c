/* Copyright (C) 1991, 1992, 1996, 1997 Free Software Foundation, Inc.
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

#include <stdio.h>

#define TESTFILE "/tmp/test.dat"

int
main (void)
{
  FILE *fp;
  int i, j;

  puts ("\nFile seek test");
  fp = fopen (TESTFILE, "w");
  if (fp == NULL)
    {
      perror (TESTFILE);
      return 1;
    }

  for (i = 0; i < 256; i++)
    putc (i, fp);
  if (freopen (TESTFILE, "r", fp) != fp)
    {
      perror ("Cannot open file for reading");
      return 1;
    }

  for (i = 1; i <= 255; i++)
    {
      printf ("%3d\n", i);
      fseek (fp, (long) -i, SEEK_END);
      if ((j = getc (fp)) != 256 - i)
	{
	  printf ("SEEK_END failed %d\n", j);
	  break;
	}
      if (fseek (fp, (long) i, SEEK_SET))
	{
	  puts ("Cannot SEEK_SET");
	  break;
	}
      if ((j = getc (fp)) != i)
	{
	  printf ("SEEK_SET failed %d\n", j);
	  break;
	}
      if (fseek (fp, (long) i, SEEK_SET))
	{
	  puts ("Cannot SEEK_SET");
	  break;
	}
      if (fseek (fp, (long) (i >= 128 ? -128 : 128), SEEK_CUR))
	{
	  puts ("Cannot SEEK_CUR");
	  break;
	}
      if ((j = getc (fp)) != (i >= 128 ? i - 128 : i + 128))
	{
	  printf ("SEEK_CUR failed %d\n", j);
	  break;
	}
    }
  fclose (fp);
  remove (TESTFILE);

  puts ((i > 255) ? "Test succeeded." : "Test FAILED!");
  return (i > 255) ? 0 : 1;
}
