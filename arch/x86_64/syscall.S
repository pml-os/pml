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
	push	%r12
	push	%r11

	/* Switch stacks */
	mov	%rsp, %rbp
	movabs	$SYSCALL_STACK_TOP_VMA, %rsp
	push	%rbp

	/* Clear error value */
	push	%rdi
	push	%rsi
	push	%rdx
	push	%r10
	push	%r8
	push	%r9
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

	/* Save return value and errno */
	mov	%rax, %rbx
	call	get_errno
	mov	(%rax), %r12d

	/* Check if a signal can be handled */
	call	load_signal
	test	%eax, %eax
	jnz	2f

	/* Disable task switching and switch stacks */
	movabs	$thread_switch_lock, %rax
	movl	$1, (%rax)
	mov	(%rsp), %rsp
	mov	%rbx, %rax
	pop	%r11

	/* Check if an error occurred */
	cmp	$-1, %rax
	jne	1f
	or	$0x0001, %r11
	mov	%r12d, %eax
1:
	pop	%r12
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdi
	pop	%rsi
	pop	%rbp
	pop	%rdx
	pop	%rcx
	pop	%rbx

	/* Prepare to execute signal handler */
	push	%rcx
	call	int_save_registers
	mov	%r11, %r12
	mov	$1, %edi
	call	poll_signal_handler
	mov	%rax, %rdi
	movabs	$SIGNAL_TRAMPOLINE_VMA, %rcx
	mov	%r12, %r11
	movabs	$thread_switch_lock, %rax
	movl	$0, (%rax)
	sysretq

2:
	/* Switch stacks and return to user mode */
	mov	(%rsp), %rsp
	mov	%rbx, %rax
	pop	%r11
	cmp	$-1, %rax
	jne	3f
	or	$0x0001, %r11
	mov	%r12d, %eax
3:
	pop	%r12
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
