/* smp-init.S -- This file is part of PML.
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
#include <pml/ctlreg.h>
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/msr.h>

	/* Application processors start in real mode */
	.code16

	.section .text
	.global smp_ap_start
ASM_FUNC_BEGIN (smp_ap_start):
	ljmp	$0x00, $(SMP_AP_START_ADDR + 5)
	cli
	cld

	/* Load bootstrap PML4T */
	mov	SMP_AP_INIT_PML4T, %eax
	mov	%eax, %cr3

	/* Enable physical address extension, required by long mode */
	mov	%cr4, %eax
	or	$CR4_PAE, %eax
	mov	%eax, %cr4

	/* Enable long mode in the EFER register */
	mov	$MSR_EFER, %ecx
	rdmsr
	or	$(EFER_SCE | EFER_LME), %eax
	wrmsr

	/* Enable paging and protected mode, required by long mode */
	mov	%cr0, %eax
	or	$(CR0_PE | CR0_PG), %eax
	mov	%eax, %cr0

	/* Enter 64-bit submode */
	mov	SMP_AP_INIT_GDT, %eax
	lgdtl	(%eax)
	ljmp	$0x08, $SMP_AP_LONG_START_ADDR
ASM_FUNC_END (smp_ap_start)
	.global smp_ap_size
	.set smp_ap_size, . - smp_ap_start

	/* Remaining code runs in long mode */
	.code64

	.global smp_ap_long_start
ASM_FUNC_BEGIN (smp_ap_long_start):
	mov	$0x10, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss
	mov	SMP_AP_INIT_STACK, %rsp
	movabs	$smp_ap_high_start, %rax
	jmp	*%rax
ASM_FUNC_END (smp_ap_long_start)
	.global smp_ap_long_size
	.set smp_ap_long_size, . - smp_ap_long_start

ASM_FUNC_BEGIN (smp_ap_high_start):
	/* Notify the BSP that initialization is complete. */
	movabs	$smp_ap_setup_done, %rax
	lock
	incl	(%rax)

	/* Use the main virtual memory layout */
	movabs	$kernel_pml4t, %rax
	movabs	$KERNEL_VMA, %rbx
	sub	%rbx, %rax
	mov	%rax, %cr3

	/* Enable additional instruction sets */
	call	enable_insn_sets

	/* Set FSBASE to thread-local storage address */
	mov	$MSR_FSBASE, %ecx
	mov	$(THREAD_LOCAL_BASE_VMA >> 32), %edx
	mov	$(THREAD_LOCAL_BASE_VMA & 0xffffffff), %eax
	wrmsr

	/* Initialize syscalls */
	call	syscall_init

	/* Nothing left to do, just busy-wait */
.hang:
	hlt
	jmp	.hang
ASM_FUNC_END (smp_ap_high_start)
