/* sched.S -- This file is part of PML.
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

	.global load_signal
ASM_FUNC_BEGIN (load_signal):
	push	%rbp
	mov	%rsp, %rbp
	sub	$16, %rsp
	mov	%edi, -16(%rbp)

	/* If a signal is already waiting, don't check for another */
	call	signal_handler
	test	%rax, %rax
	jnz	.slow_check

	/* Check if a signal needs to be handled */
	call	poll_signal
	test	%eax, %eax
	jz	.cancel
	mov	%eax, %edi
	call	handle_signal

.slow_check:
	/* Check if we are in a slow system call. If we are, run the signal
	   handler immediately instead of doing the stack check. */
	/*call	slow_syscall
	test	%eax, %eax
	jz	.stack_check
	xor	%eax, %eax
	leave
	ret*/

.stack_check:
	/* Check the stack to determine the origin of the syscall.
	   This makes sure a signal is only executed when returning to user
	   mode. If EDI is nonzero check for a syscall entry, otherwise check
	   for an interrupt handler entry */
	mov	-16(%rbp), %edi
	test	%edi, %edi
	jz	.int_stack_check
	movabs	$SYSCALL_STACK_TOP_VMA, %rax
	cmp	%rax, %rsp
	ja	.cancel
	movabs	$PROCESS_STACK_BASE_VMA, %rax
	cmp	%rax, %rsp
	jbe	.cancel
	xor	%eax, %eax
	leave
	ret

.int_stack_check:
	movabs	$INTERRUPT_STACK_TOP_VMA, %rax
	cmp	%rax, %rsp
	ja	.cancel
	movabs	$SYSCALL_STACK_TOP_VMA, %rax
	cmp	%rax, %rsp
	jbe	.cancel
	xor	%eax, %eax
	leave
	ret

.cancel:
	mov	$1, %eax
	leave
	ret
ASM_FUNC_END (load_signal)

	.global run_signal
ASM_FUNC_BEGIN (run_signal):
	push	%rbp
	mov	%rsp, %rbp

	xor	%edi, %edi
	call	load_signal
	test	%eax, %eax
	jz	.handle
	leave
	ret

.handle:
	/* Get the original stack pointer and return address */
	movabs	$INTERRUPT_STACK_TOP_VMA, %rax
	mov	-16(%rax), %rbx
	mov	-40(%rax), %rax
	mov	%rax, -8(%rbx)

	/* TODO Support loading custom signal stacks,
	   save the signal mask before switching to trampoline */
	/* Load the signal return context on the thread's stack */
	call	reg_save_size
	mov	%eax, %edx
	neg	%rax
	lea	-8(%rbx,%rax), %rdi
	mov	%rdi, %rbx
	movabs	$INTERRUPT_STACK_TOP_VMA, %rsi
	lea	-40(%rsi,%rax), %rsi
	call	memcpy

	/* Jump to signal handler */
	sub	$16, %rsp
	mov	%rsp, %rdi
	call	poll_signal_handler
	mov	%rax, %rdi
	mov	(%rsp), %rsi
	pushq	$0x1b
	push	%rbx
	pushf
	pushq	$0x23
	movabs	$SIGNAL_TRAMPOLINE_VMA, %rax
	push	%rax
	iretq
ASM_FUNC_END (run_signal)
