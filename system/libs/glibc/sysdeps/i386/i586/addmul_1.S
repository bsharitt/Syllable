/* Pentium __mpn_addmul_1 -- Multiply a limb vector with a limb and add
   the result to a second limb vector.
   Copyright (C) 1992, 94, 96, 97, 98 Free Software Foundation, Inc.
   This file is part of the GNU MP Library.

   The GNU MP Library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   The GNU MP Library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
   License for more details.

   You should have received a copy of the GNU Library General Public License
   along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA. */

/*
   INPUT PARAMETERS
   res_ptr	(sp + 4)
   s1_ptr	(sp + 8)
   size		(sp + 12)
   s2_limb	(sp + 16)
*/

#include "sysdep.h"
#include "asm-syntax.h"

#define res_ptr edi
#define s1_ptr esi
#define size ecx
#define s2_limb ebp

	.text
ENTRY(__mpn_addmul_1)

	INSN1(push,l	,R(edi))
	INSN1(push,l	,R(esi))
	INSN1(push,l	,R(ebx))
	INSN1(push,l	,R(ebp))

	INSN2(mov,l	,R(res_ptr),MEM_DISP(esp,20))
	INSN2(mov,l	,R(s1_ptr),MEM_DISP(esp,24))
	INSN2(mov,l	,R(size),MEM_DISP(esp,28))
	INSN2(mov,l	,R(s2_limb),MEM_DISP(esp,32))

	INSN2(lea,l	,R(res_ptr),MEM_INDEX(res_ptr,size,4))
	INSN2(lea,l	,R(s1_ptr),MEM_INDEX(s1_ptr,size,4))
	INSN1(neg,l	,R(size))
	INSN2(xor,l	,R(ebx),R(ebx))
	ALIGN (3)

L(oop):	INSN2(adc,l	,R(ebx),$0)
	INSN2(mov,l	,R(eax),MEM_INDEX(s1_ptr,size,4))

	INSN1(mul,l	,R(s2_limb))

	INSN2(add,l	,R(eax),R(ebx))
	INSN2(mov,l	,R(ebx),MEM_INDEX(res_ptr,size,4))

	INSN2(adc,l	,R(edx),$0)
	INSN2(add,l	,R(ebx),R(eax))

	INSN2(mov,l	,MEM_INDEX(res_ptr,size,4),R(ebx))
	INSN1(inc,l	,R(size))

	INSN2(mov,l	,R(ebx),R(edx))
	INSN1(jnz,	,L(oop))

	INSN2(adc,l	,R(ebx),$0)
	INSN2(mov,l	,R(eax),R(ebx))
	INSN1(pop,l	,R(ebp))
	INSN1(pop,l	,R(ebx))
	INSN1(pop,l	,R(esi))
	INSN1(pop,l	,R(edi))
	ret
#undef size
END(__mpn_addmul_1)
