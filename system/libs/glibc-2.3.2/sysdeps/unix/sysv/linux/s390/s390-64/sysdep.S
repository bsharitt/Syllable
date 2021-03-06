/* Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
   Contributed by Martin Schwidefsky (schwidefsky@de.ibm.com).
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <sysdep.h>


/* The following code is only used in the shared library when we
   compile the reentrant version.  Otherwise each system call defines
   each own version.  */

/* The syscall stubs jump here when they detect an error.  */

#undef CALL_MCOUNT
#define CALL_MCOUNT

.text
ENTRY(__syscall_error)
#ifndef PIC
# if USE___THREAD
#  ifndef NOT_IN_libc
#   define SYSCALL_ERROR_ERRNO __libc_errno
#  else
#   define SYSCALL_ERROR_ERRNO errno
#  endif
	basr	%r1,0
0:	lg	%r1,1f-0b(%r1)
	ear	%r3,%a0
	sllg	%r3,%r3,32
	ear	%r3,%a1
	lcr	%r2,%r2
	st	%r2,0(%r1,%r3)
	lghi	%r2,-1
	br	%r14
1:	.quad	SYSCALL_ERROR_ERRNO@ntpoff
# elif !defined _LIBC_REENTRANT
	larl	%r1,errno
	lcr	%r2,%r2
	st	%r2,0(%r1)
	lghi	%r2,-1
	br	%r14
# else
	stmg	%r13,%r15,104(%r15)
	lgr	%r0,%r15
	aghi	%r15,-160
	lcr	%r13,%r2
	stg	%r0,0(%r15)
	brasl	%r14,__errno_location
	st	%r13,0(%r2)
	lmg	%r13,%r15,264(%r15)
	lghi	%r2,-1
	br	%r14
#endif
#else
# if RTLD_PRIVATE_ERRNO
	larl	%r1,errno
	lcr	%r2,%r2
	st	%r2,0(%r1)
	lghi	%r2,-1
	br	%r14
# elif USE___THREAD
#  ifndef NOT_IN_libc
#   define SYSCALL_ERROR_ERRNO __libc_errno
#  else
#   define SYSCALL_ERROR_ERRNO errno
#  endif
	larl	%r1,_GLOBAL_OFFSET_TABLE_
	lg	%r1,SYSCALL_ERROR_ERRNO@gotntpoff(%r1)
	ear	%r3,%a0
	sllg	%r3,%r3,32
	ear	%r3,%a1
	lcr	%r2,%r2
	st	%r2,0(%r1,%r3)
	lghi	%r2,-1
	br	%r14
# elif !defined _LIBC_REENTRANT
	larl	%r1,_GLOBAL_OFFSET_TABLE_
	lg	%r1,errno@GOT(%r1)
	lcr	%r2,%r2
	st	%r2,0(%r1)
	lghi	%r2,-1
	br	%r14	
# else
	stmg	%r13,%r15,104(%r15)
	lgr	%r0,%r15
	aghi	%r15,-160
	lcr	%r13,%r2
	stg	%r0,0(%r15)
	brasl	%r14,__errno_location@PLT
	st	%r13,0(%r2)
	lmg	%r13,%r15,264(%r15)
	lghi	%r2,-1
	br	%r14
# endif
#endif

END (__syscall_error)
