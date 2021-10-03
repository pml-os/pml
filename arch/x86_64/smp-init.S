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
#include <pml/interrupt.h>

	/* Application processors start in real mode */
	.code16

	.section .text
	.global smp_ap_start
ASM_FUNC_BEGIN (smp_ap_start):
	ljmp	$0x00, $0x8005
.start:
	/* This is at address 0x8005 since this code is loaded at 0x8000 and
	   the preceding ljmp instruction is 5 bytes wide. */
	cli
	cld
1:
	hlt
	jmp	1b
ASM_FUNC_END (smp_ap_start)
smp_ap_end:

	.global smp_ap_size
	.set smp_ap_size, smp_ap_end - smp_ap_start
