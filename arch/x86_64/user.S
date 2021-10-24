/* user.S -- This file is part of PML.
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
#include <pml/syscall.h>

	.section .text
	.global user_mode
ASM_FUNC_BEGIN (user_mode):
	/* Set segment selectors with ring 3 segments */
	mov	$0x1b, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs

	/* Do a fake interrupt into ring 3 */
	mov	%rsp, %rax
	pushq	$0x1b
	push	%rax
	pushf
	pushq	$0x23
	push	%rdi
	iretq
ASM_FUNC_END (user_mode)
