/* Copyright (C) 1999 Free Software Foundation, Inc.
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

#include <sysdep.h>

/* If no SA_RESTORER function was specified by the application we use
   one of these.  This avoids the need for the kernel to synthesise a return
   instruction on the stack, which would involve expensive cache flushes. */

ENTRY(__default_sa_restorer)
	swi	SYS_ify(sigreturn)

#ifdef __NR_rt_sigreturn

ENTRY(__default_rt_sa_restorer)
	swi	SYS_ify(rt_sigreturn)

#endif
