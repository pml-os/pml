/* boot.S -- This file is part of PML.
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
#include <pml/memory.h>
#include <pml/msr.h>
#include <pml/multiboot.h>
#include <pml/thread.h>

	/* Boot code runs in protected mode */
	.code32

	.section .multiboot.data, "aw", @progbits
	.align MULTIBOOT_TAG_ALIGN
	.set multiboot_header_length, multiboot_header_end - multiboot_header
multiboot_header:
	.align MULTIBOOT_TAG_ALIGN
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long multiboot_header_length
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + \
		multiboot_header_length)
	.align MULTIBOOT_TAG_ALIGN
	.short MULTIBOOT_HEADER_TAG_END
	.short 0
	.long 8
multiboot_header_end:
boot_gdt:
	/* Null segment */
	.short 0xffff
	.short 0
	.byte 0
	.byte 0
	.byte 1
	.byte 0
	/* Code segment */
	.short 0
	.short 0
	.byte 0
	.byte 0x9a
	.byte 0xaf
	.byte 0
	/* Data segment */
	.short 0
	.short 0
	.byte 0
	.byte 0x92
	.byte 0
	.byte 0
boot_gdt_ptr:
	.short . - boot_gdt - 1
	.quad boot_gdt

	.section .multiboot.bss, "aw", @nobits
	.align PAGE_STRUCT_ALIGN
boot_pml4t:
	.skip PAGE_STRUCT_SIZE
boot_pdpt_low:
	.skip PAGE_STRUCT_SIZE
boot_pdpt_high:
	.skip PAGE_STRUCT_SIZE
	.set boot_pml4e_flags, PAGE_FLAG_PRESENT | PAGE_FLAG_RW
	.set boot_pdpe_flags, PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_SIZE

	.section .multiboot.text, "ax", @progbits
	.global _start
ASM_FUNC_BEGIN (_start):
	/* Ensure we were booted by a Multiboot2-compliant bootloader and
	   the info structure is 8-byte aligned. */
	cmp	$MULTIBOOT2_BOOTLOADER_MAGIC, %eax
	jne	boot_check_fail
	test	$7, %ebx
	jnz	boot_check_fail

	/* Setup boot page structures. The low 4 GiB of physical memory
	   is mapped to itself in virtual memory and at the higher half address
	   so we can run code there. */
	movl	$(boot_pdpt_low + boot_pml4e_flags), boot_pml4t
	movl	$(boot_pdpt_high + boot_pml4e_flags), boot_pml4t+4064
	movl	$boot_pdpe_flags, boot_pdpt_low
	movl	$(0x40000000 | boot_pdpe_flags), boot_pdpt_low+8
	movl	$(0x80000000 | boot_pdpe_flags), boot_pdpt_low+16
	movl	$(0xc0000000 | boot_pdpe_flags), boot_pdpt_low+24
	movl	$boot_pdpe_flags, boot_pdpt_high
	movl	$(0x40000000 | boot_pdpe_flags), boot_pdpt_high+8
	movl	$(0x80000000 | boot_pdpe_flags), boot_pdpt_high+16
	movl	$(0xc0000000 | boot_pdpe_flags), boot_pdpt_high+24
	mov	$boot_pml4t, %eax
	mov	%eax, %cr3

	/* Enable physical address extension, required by long mode */
	mov	%cr4, %eax
	or	$CR4_PAE, %eax
	mov	%eax, %cr4

	/* Enable long mode in the EFER register */
	mov	$MSR_EFER, %ecx
	rdmsr
	or	$EFER_LME, %eax
	wrmsr

	/* Enable paging and protected mode, required by long mode */
	mov	%cr0, %eax
	or	$(CR0_PE | CR0_PG), %eax
	mov	%eax, %cr0

	/* Enter 64-bit submode and jump to long mode code */
	mov	$boot_gdt_ptr, %eax
	lgdt	(%eax)
	ljmp	$0x08, $long_start
ASM_FUNC_END (_start)

	/* Remaining code is executed in long mode */
	.code64

ASM_FUNC_BEGIN (boot_check_fail):
	cli
1:
	hlt
	jmp	1b
ASM_FUNC_END (boot_check_fail)

ASM_FUNC_BEGIN (long_start):
	/* This is a temporary stub that is entered by the protected mode code.
	   It is required to be in low memory because high memory in the
	   64-bit address space cannot be referenced in protected mode. The
	   data segments are assigned to the boot GDT, the stack pointer is set,
	   and execution resumes at the entry point in high memory. */
	mov	$0x10, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss
	movabs	$boot_stack_top, %rsp
	movabs	$high_start, %rax
	jmp	*%rax
ASM_FUNC_END (long_start)

	.section .text
ASM_FUNC_BEGIN (high_start):
	/* Save the address to the Multiboot2 structure, since EBX will
	   be clobbered by CPUID */
	push	%rbx

	/* Enable additional instruction sets if supported */
	call	enable_insn_sets

	/* Initialize the main GDT */
	call	init_gdt

	/* Initialize VGA text mode display */
	call	vga_text_init

	/* Setup kernel state based on Multiboot2 structure */
	pop	%rdi
	call	multiboot_init

	/* Set FSBASE to thread-local storage address */
	mov	$MSR_FSBASE, %ecx
	mov	$(THREAD_LOCAL_BASE_VMA >> 32), %edx
	mov	$(THREAD_LOCAL_BASE_VMA & 0xffffffff), %eax
	wrmsr

	/* Initialize virtual memory layout */
	call	vm_init
	movabs	$DEFAULT_STACK_VMA, %rsp

	/* Initialize interrupts */
	call	init_idt

	/* Initialize platform-specific services */
	call	arch_init

	/* Call real kernel entry point */
	call	kentry

	/* Nothing left to do, just busy-wait */
	sti
.hang:
	hlt
	jmp	.hang
ASM_FUNC_END (high_start)

	.section .bss
	.align PAGE_SIZE
	.global boot_stack
boot_stack:
	.skip 16384
boot_stack_top:
