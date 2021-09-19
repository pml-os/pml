/* panic.S -- This file is part of PML.
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
	.global __panic
ASM_FUNC_BEGIN (__panic):
	cli
	call	panic_print_message
	movabs	$panic_registers, %rax
	pushq	56(%rax)
	pushq	48(%rax)
	mov	40(%rax), %r9
	mov	32(%rax), %r8
	mov	24(%rax), %rcx
	mov	16(%rax), %rdx
	mov	8(%rax), %rsi
	mov	(%rax), %rdi
	call	panic_print_registers
	jmp	abort
ASM_FUNC_END (__panic)
