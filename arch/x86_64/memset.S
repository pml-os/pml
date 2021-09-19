/* memset.S -- This file is part of PML.
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

	.global memset
ASM_FUNC_BEGIN (memset):
	mov	%rdi, %r9
	mov	%rsi, %rax
	mov	%rdx, %rcx
	cmp	$16, %rdx
	jb	.byteset

	mov	%rdi, %r8
	and	$7, %r8
	jz	.aligned

	mov	$8, %rcx
	sub	%r8, %rcx
	sub	%rcx, %rdx
	rep
	stosb
	mov	%rdx, %rcx

.aligned:
	movabs	$0x0101010101010101, %r8
	movzx	%sil, %eax
	imul	%r8, %rax
	cmp	$256, %rdx
	jb	.quadset
	shr	$7, %rcx

	.align 16
.loop:
	movnti	%rax, (%rdi)
	movnti	%rax, 8(%rdi)
	movnti	%rax, 16(%rdi)
	movnti	%rax, 24(%rdi)
	movnti	%rax, 32(%rdi)
	movnti	%rax, 40(%rdi)
	movnti	%rax, 48(%rdi)
	movnti	%rax, 56(%rdi)
	movnti	%rax, 64(%rdi)
	movnti	%rax, 72(%rdi)
	movnti	%rax, 80(%rdi)
	movnti	%rax, 88(%rdi)
	movnti	%rax, 96(%rdi)
	movnti	%rax, 104(%rdi)
	movnti	%rax, 112(%rdi)
	movnti	%rax, 120(%rdi)
	lea	128(%rdi), %rdi
	dec	%rcx
	jnz	.loop

	sfence
	mov	%rdx, %rcx
	and	$127, %rcx
	rep
	stosb
	mov	%r9, %rax
	ret

.byteset:
	rep
	stosb
	mov	%r9, %rax
	ret

.quadset:
	shr	$3, %rcx

	.align 16
	rep
	stosq
	mov	%rdx, %rcx
	and	$7, %rcx
	rep
	stosb
	mov	%r9, %rax
	ret
ASM_FUNC_END (memset)
