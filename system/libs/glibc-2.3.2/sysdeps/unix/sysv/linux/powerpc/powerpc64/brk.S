/* brk system call for Linux.  PowerPC64 version.
   Copyright (C) 1995,96,97,99, 2000, 2002 Free Software Foundation, Inc.
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
#define _ERRNO_H	1
#include <bits/errno.h>
#include <bp-sym.h>
#include <bp-asm.h>

	.comm	__curbrk,8,8
	.section ".text"
ENTRY (BP_SYM (__brk))
	DISCARD_BOUNDS (r3)	/* the bounds are meaningless, so toss 'em.  */

	stdu    r1,-64(r1)
	std	r3,48(r1)
	DO_CALL(SYS_ify(brk))
	ld     r6,48(r1)
	ld	r5,__curbrk@got(r2)
	std     r3,0(r5)
	cmpld   r6,r3
	addi    r1,r1,64
	li	r3,0
	blelr+
	li      r3,ENOMEM
	b	JUMPTARGET(__syscall_error)
END (BP_SYM (__brk))

weak_alias (BP_SYM (__brk), BP_SYM (brk))
