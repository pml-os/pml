/* memcpy.S -- This file is part of PML.
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

	.section .text
	.global memcpy
ASM_FUNC_BEGIN (memcpy):
	mov	%rdi, %rax
	cmp	$16, %rdx
	jb	.bytecopy

	mov	%rdi, %r8
	and	$7, %r8
	jz	.aligned

	mov	$8, %rcx
	sub	%r8, %rcx
	sub	%rcx, %rdx
	rep
	movsb

.aligned:
	cmp	$256, %rdx
	jb	.quadcopy

	push	%rax
	push	%r12
	push	%r13
	push	%r14
	mov	%rdx, %rcx
	shr	$7, %rcx

	.align 16
.loop:
	prefetchnta	768(%rsi)
	prefetchnta	832(%rsi)
	mov	(%rsi), %rax
	mov	8(%rsi), %r8
	mov	16(%rsi), %r9
	mov	24(%rsi), %r10
	mov	32(%rsi), %r11
	mov	40(%rsi), %r12
	mov	48(%rsi), %r13
	mov	56(%rsi), %r14

	movnti	%rax, (%rdi)
	movnti	%r8, 8(%rdi)
	movnti	%r9, 16(%rdi)
	movnti	%r10, 24(%rdi)
	movnti	%r11, 32(%rdi)
	movnti	%r12, 40(%rdi)
	movnti	%r13, 48(%rdi)
	movnti	%r14, 56(%rdi)

	mov	64(%rsi), %rax
	mov	72(%rsi), %r8
	mov	80(%rsi), %r9
	mov	88(%rsi), %r10
	mov	96(%rsi), %r11
	mov	104(%rsi), %r12
	mov	112(%rsi), %r13
	mov	120(%rsi), %r14

	movnti	%rax, 64(%rdi)
	movnti	%r8, 72(%rdi)
	movnti	%r9, 80(%rdi)
	movnti	%r10, 88(%rdi)
	movnti	%r11, 96(%rdi)
	movnti	%r12, 104(%rdi)
	movnti	%r13, 112(%rdi)
	movnti	%r14, 120(%rdi)

	lea	128(%rsi), %rsi
	lea	128(%rdi), %rdi
	dec	%rcx
	jnz	.loop

	sfence
	mov	%rdx, %rcx
	and	$127, %rcx
	rep
	movsb
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rax
	ret

.bytecopy:
	mov	%rdx, %rcx
	rep
	movsb
	ret

.quadcopy:
	mov	%rdx, %rcx
	shr	$3, %rcx

	.align 16
	rep
	movsq
	mov	%rdx, %rcx
	and	$7, %rcx
	rep
	movsb
	ret
ASM_FUNC_END (memcpy)

	.global mempcpy
ASM_FUNC_BEGIN (mempcpy):
	call	memcpy
	lea	(%rdi,%rdx), %rax
	ret
ASM_FUNC_END (mempcpy)
