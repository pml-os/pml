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

	.section .bss
	.global int_stack
	.align PAGE_SIZE
int_stack:
	.skip 16384
	.global int_stack_top
int_stack_top:

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
	mov	%r10, %rcx

	/* Switch stacks */
	mov	%rsp, %rbp
	movabs	$int_stack_top, %rsp
	push	%rbp

	/* Clear error value and call function in syscall table */
	movabs	$errno, %rbx
	movl	$0, (%rbx)
	movabs	$syscall_table, %rbx
	mov	(%rbx,%rax,8), %rax
	call	*%rax

	/* Error occurred, return errno value and set carry flag */
	movabs	$errno, %rbx
	mov	(%rbx), %ebx
	test	%ebx, %ebx
	jz	1f
	mov	%ebx, %eax
	mov	(%rsp), %rsp
	pop	%r11
	or	$0x0001, %r11

1:
	mov	(%rsp), %rsp
	pop	%r11
2:
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
