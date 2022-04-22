/* syscall.S -- This file is part of PML.
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
#include <pml/memory.h>

	.section .text
	.global syscall
ASM_FUNC_BEGIN (syscall):
	push	%rbx
	push	%rcx
	push	%rdx
	push	%rbp
	push	%rsi
	push	%rdi
	push	%r8
	push	%r9
	push	%r10
	push	%r11

	/* Switch stacks */
	mov	%rsp, %rbp
	movabs	$SYSCALL_STACK_TOP_VMA, %rsp
	push	%rbp

	push	%rdi
	push	%rsi
	push	%rdx
	push	%r10
	push	%r8
	push	%r9

	/* Clear error value */
	mov	%eax, %ebx
	call	get_errno
	movl	$0, (%rax)

	pop	%r9
	pop	%r8
	pop	%r10
	pop	%rdx
	pop	%rsi
	pop	%rdi
	mov	%r10, %rcx

	/* Call function in syscall table */
	movabs	$syscall_table, %rax
	mov	(%rax,%rbx,8), %rax
	call	*%rax

	/* If an error occurred, return errno value and set carry flag */
	mov	%rax, %rbx
	call	get_errno
	mov	(%rax), %ecx
	test	%ecx, %ecx
	jz	1f
	mov	%ecx, %ebx
	mov	(%rsp), %rsp
	pop	%r11
	or	$0x0001, %r11
	jmp	2f

1:
	mov	(%rsp), %rsp
	pop	%r11
2:
	mov	%rbx, %rax
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdi
	pop	%rsi
	pop	%rbp
	pop	%rdx
	pop	%rcx
	pop	%rbx
	sysretq
ASM_FUNC_END (syscall)
