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
#include <pml/cmos.h>
#include <pml/interrupt.h>

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
	call	this_thread
	mov	%rax, %rdi
	mov	%rsp, %rsi
	call	thread_save_stack

	/* Switch threads */
	sub	$16, %rsp
	lea	(%rsp), %rdi
	lea	8(%rsp), %rsi
	call	thread_switch

	/* Set the new page directory and stack pointer */
	mov	(%rsp), %rdx
	mov	8(%rsp), %rax
	mov	%cr3, %rcx
	cmp	%rax, %rcx
	je	.no_flush
	mov	%rax, %cr3
.no_flush:
	mov	%rdx, %rsp

	/* Free an exited process */
	movabs	$exit_process, %rax
	mov	(%rax), %edi
	test	%edi, %edi
	jz	.done

	movl	$0, (%rax)
	movabs	$exit_status, %rax
	mov	(%rax), %esi
	movl	$0, (%rax)
	call	process_exit

.done:
	call	cmos_rtc_finish_irq
#ifdef USE_APIC
	call	local_apic_eoi
#else
	mov	$8, %edi
	call	pic_8259_eoi
#endif
	call	int_restore_registers
	iretq
ASM_FUNC_END (sched_tick)

	.global sched_exec
ASM_FUNC_BEGIN (sched_exec):
	/* Set segment selectors with ring 3 segments */
	mov	$0x1b, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs

	/* Do a fake interrupt into ring 3 */
	pushq	$0x1b
	push	%rsi
	pushf
	pushq	$0x23
	push	%rdi
	xor	%ebp, %ebp
	iretq
ASM_FUNC_END (sched_exec)

	.global sched_yield
ASM_FUNC_BEGIN (sched_yield):
	lea	.after_ts(%rip), %rdi
	call	sched_yield_to
.after_ts:
	ret
ASM_FUNC_END (sched_yield)

	.global sched_yield_to
ASM_FUNC_BEGIN (sched_yield_to):
	mov	%rsp, %rcx
	mov	%ss, %rax
	push	%rax
	push	%rcx
	pushf
	mov	%cs, %rax
	push	%rax
	push	%rdi
	jmp	sched_tick
ASM_FUNC_END (sched_yield_to)
