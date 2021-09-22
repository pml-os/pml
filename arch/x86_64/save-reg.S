/* save-reg.S -- This file is part of PML.
   Copyright (C) 2021 XNSC

   PML is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   PML is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PML. If not, see <https://www.gnu.org/licenses/>. */

#include <pml/asm.h>

	.global int_save_registers
ASM_FUNC_BEGIN (int_save_registers):
	/* Save return pointer and write RAX */
	xchg	%rax, (%rsp)

	/* Save other general-purpose registers */
	push	%rcx
	push	%rdx
	push	%rbx
	push	%rbp
	push	%rsi
	push	%rdi
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	/* Save SSE registers */
	lea	-256(%rsp), %rsp
	movdqa	%xmm0, (%rsp)
	movdqa	%xmm1, 16(%rsp)
	movdqa	%xmm2, 32(%rsp)
	movdqa	%xmm3, 48(%rsp)
	movdqa	%xmm4, 64(%rsp)
	movdqa	%xmm5, 80(%rsp)
	movdqa	%xmm6, 96(%rsp)
	movdqa	%xmm7, 112(%rsp)
	movdqa	%xmm8, 128(%rsp)
	movdqa	%xmm9, 144(%rsp)
	movdqa	%xmm10, 160(%rsp)
	movdqa	%xmm11, 176(%rsp)
	movdqa	%xmm12, 192(%rsp)
	movdqa	%xmm13, 208(%rsp)
	movdqa	%xmm14, 224(%rsp)
	movdqa	%xmm15, 240(%rsp)

	/* Return to caller using saved return pointer */
	jmp	*%rax
ASM_FUNC_END (int_save_registers)

	.global int_restore_registers
ASM_FUNC_BEGIN (int_restore_registers):
	/* Save instruction pointer */
	pop	%rax

	/* Restore SSE registers */
	movdqa	(%rsp), %xmm0
	movdqa	16(%rsp), %xmm1
	movdqa	32(%rsp), %xmm2
	movdqa	48(%rsp), %xmm3
	movdqa	64(%rsp), %xmm4
	movdqa	80(%rsp), %xmm5
	movdqa	96(%rsp), %xmm6
	movdqa	112(%rsp), %xmm7
	movdqa	128(%rsp), %xmm8
	movdqa	144(%rsp), %xmm9
	movdqa	160(%rsp), %xmm10
	movdqa	176(%rsp), %xmm11
	movdqa	192(%rsp), %xmm12
	movdqa	208(%rsp), %xmm13
	movdqa	224(%rsp), %xmm14
	movdqa	240(%rsp), %xmm15
	lea	256(%rsp), %rsp

	/* Restore general-purpose registers */
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdi
	pop	%rsi
	pop	%rbp
	pop	%rbx
	pop	%rdx
	pop	%rcx

	/* Restore RAX and jump to saved pointer */
	xchg	%rax, (%rsp)
	ret
ASM_FUNC_END (int_restore_registers)
