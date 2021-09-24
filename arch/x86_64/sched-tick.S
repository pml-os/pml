/* sched-tick.S -- This file is part of PML.
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
	.global int_rtc_tick
ASM_FUNC_BEGIN (int_rtc_tick):
	jmp	sched_tick
ASM_FUNC_END (int_rtc_tick)

	.global sched_tick
ASM_FUNC_BEGIN (sched_tick):
	call	int_save_registers

	/* If thread switching is disabled, don't switch */
	movabs	$thread_switch_lock, %rax
	mov	(%rax), %eax
	test	%eax, %eax
	jnz	.done

	/* Save this thread's stack pointer */
	mov	%rsp, %rdi
	call	thread_save_stack

	/* Switch threads */
	sub	$16, %rsp
	lea	(%rsp), %rdi
	lea	8(%rsp), %rsi
	call	thread_switch

	/* Set the new page directory */
	mov	8(%rsp), %rax
	mov	%cr3, %rcx
	cmp	%rax, %rcx
	je	.no_flush
	mov	%rax, %cr3

.no_flush:
	/* Set the new stack pointer */
	mov	(%rsp), %rsp

.done:
	call	int_restore_registers
	iretq
ASM_FUNC_END (sched_tick)

	.global sched_yield
ASM_FUNC_BEGIN (sched_yield):
	mov	%rsp, %rcx
	mov	%ss, %rax
	push	%rax
	push	%rcx
	pushf
	mov	%cs, %rax
	push	%rax
	lea	.after_ts(%rip), %rax
	push	%rax
	jmp	sched_tick

.after_ts:
	ret
ASM_FUNC_END (sched_yield)
