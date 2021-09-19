/* insn-set.S -- This file is part of PML.
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
#include <pml/cpuid.h>
#include <pml/ctlreg.h>

	.section .text
	.global enable_insn_sets
ASM_FUNC_BEGIN (enable_insn_sets):
	mov	$1, %eax
	cpuid

	/* Enable SSE */
	mov	%cr0, %rax
	and	$~CR0_EM, %rax
	or	$CR0_MP, %rax
	mov	%rax, %cr0
	mov	%cr4, %rax
	or	$(CR4_OSFXSR | CR4_OSXMMEXCPT), %rax

	/* Enable XSAVE if supported */
	test	$CPUID_XSAVE, %ecx
	jz	.no_xsave
	or	$CR4_OSXSAVE, %rax
	mov	%rax, %cr4

	/* Enable AVX if supported (requires XSAVE) */
	test	$CPUID_AVX, %ecx
	jz	.no_avx
	xor	%ecx, %ecx
	xgetbv
	or	$(XCR0_X87 | XCR0_SSE | XCR0_AVX), %eax
	xsetbv
.no_avx:
	ret
.no_xsave:
	mov	%rax, %cr4
	ret
ASM_FUNC_END (enable_insn_sets)
