/* fork.S -- This file is part of PML.
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
#include <pml/errno.h>

	.section .text
ASM_FUNC_BEGIN (fork):
	push	%rbp
	mov	%rsp, %rbp
	sub	$40, %rsp

	/* -8(%rbp): Process structure
	   -16(%rbp): Thread structure
	   -24(%rbp): Thread PML4T
	   -32(%rbp): Stack pointer
	   -40(%rbp): Current PML4T */

	mov	%rdi, %rsi
	lea	-16(%rbp), %rdi
	call	process_fork
	test	%rax, %rax
	jz	.err0

	movabs	$thread_switch_lock, %rcx
	movl	$1, (%rcx)
	mov	%rax, -8(%rbp)
	mov	-8(%rbp), %rdi
	call	process_enqueue
	test	%eax, %eax
	jnz	.err1

	mov	-16(%rbp), %rdi
	mov	%rsp, %rsi
	call	thread_save_stack

	/* Setup fake interrupt stack frame */
	mov	%cr3, %rax
	mov	%rax, -40(%rbp)
	mov	-16(%rbp), %rdi
	lea	-24(%rbp), %rsi
	lea	-32(%rbp), %rdx
	call	thread_get_args
	mov	-16(%rbp), %rdi
	mov	-24(%rbp), %rax
	mov	%rsp, %rdx
	mov	-32(%rbp), %rsp
	mov	-40(%rbp), %rcx
	mov	%rax, %cr3
	mov	%rsp, %rax
	pushq	$0x10
	push	%rax
	pushf
	pushq	$0x08
	movabs	$.child, %rax
	push	%rax
	call	int_save_registers
	mov	%rsp, %rsi
	call	thread_save_stack
	mov	%rcx, %cr3
	mov	%rdx, %rsp

	/* Return PID of new process (as parent) */
	mov	-8(%rbp), %rdi
	call	process_get_pid
	jmp	.end

.child:
	/* Return 0 (as child) */
	xor	%eax, %eax
	jmp	.end

.err1:
	mov	-8(%rbp), %rdi
	call	process_free
.err0:
	mov	$-1, %eax
.end:
	movabs	$thread_switch_lock, %rcx
	movl	$0, (%rcx)
	leave
	ret
ASM_FUNC_END (fork)

	.global sys_fork
ASM_FUNC_BEGIN (sys_fork):
	mov	$1, %edi
	jmp	fork
ASM_FUNC_END (sys_fork)

	.global sys_vfork
ASM_FUNC_BEGIN (sys_vfork):
	xor	%edi, %edi
	jmp	fork
ASM_FUNC_END (sys_vfork)
