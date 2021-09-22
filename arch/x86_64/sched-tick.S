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
	.global sched_tick
ASM_FUNC_BEGIN (sched_tick):
	/* If thread switching is disabled, don't switch */
	movabs	$thread_switch_lock, %rax
	mov	(%rax), %eax
	test	%eax, %eax
	jnz	.done

	/* Save thread stack pointer */
	mov	%rsp, %rdi
	call	thread_save_stack

	/* TODO Finish thread switching */
.done:
	ret
ASM_FUNC_END (sched_tick)
