/* save-reg.S -- This file is part of PML.
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

	.global int_save_registers
ASM_FUNC_BEGIN (int_save_registers):
	/* Save return pointer and write RAX */
	xchg	%rax, (%rsp)

	/* Save other general-purpose registers */
	push	%rcx
	push	%rdx
	push	%rbx
	push	%rbp
	push	%rsi
	push	%rdi
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push	%r14
	push	%r15

#if defined AVX512F_SUPPORT
#error "saving AVX-512 registers is not supported"
#elif defined AVX_SUPPORT
	/* Align the stack pointer to 32 bytes */
	mov	%rsp, %rbp
	and	$-32, %rsp
	mov	%rbp, (%rsp)

	/* Save AVX registers */
	lea	-512(%rsp), %rsp
	vmovdqa	%ymm0, (%rsp)
	vmovdqa	%ymm1, 32(%rsp)
	vmovdqa	%ymm2, 64(%rsp)
	vmovdqa	%ymm3, 96(%rsp)
	vmovdqa	%ymm4, 128(%rsp)
	vmovdqa	%ymm5, 160(%rsp)
	vmovdqa	%ymm6, 192(%rsp)
	vmovdqa	%ymm7, 224(%rsp)
	vmovdqa	%ymm8, 256(%rsp)
	vmovdqa	%ymm9, 288(%rsp)
	vmovdqa	%ymm10, 320(%rsp)
	vmovdqa	%ymm11, 352(%rsp)
	vmovdqa	%ymm12, 384(%rsp)
	vmovdqa	%ymm13, 416(%rsp)
	vmovdqa	%ymm14, 448(%rsp)
	vmovdqa	%ymm15, 480(%rsp)
#else
	/* Save SSE registers */
	lea	-264(%rsp), %rsp
	movdqa	%xmm0, (%rsp)
	movdqa	%xmm1, 16(%rsp)
	movdqa	%xmm2, 32(%rsp)
	movdqa	%xmm3, 48(%rsp)
	movdqa	%xmm4, 64(%rsp)
	movdqa	%xmm5, 80(%rsp)
	movdqa	%xmm6, 96(%rsp)
	movdqa	%xmm7, 112(%rsp)
	movdqa	%xmm8, 128(%rsp)
	movdqa	%xmm9, 144(%rsp)
	movdqa	%xmm10, 160(%rsp)
	movdqa	%xmm11, 176(%rsp)
	movdqa	%xmm12, 192(%rsp)
	movdqa	%xmm13, 208(%rsp)
	movdqa	%xmm14, 224(%rsp)
	movdqa	%xmm15, 240(%rsp)
#endif

	/* Return to caller using saved return pointer */
	jmp	*%rax
ASM_FUNC_END (int_save_registers)

	.global int_restore_registers
ASM_FUNC_BEGIN (int_restore_registers):
	/* Save instruction pointer */
	pop	%rax

#if defined AVX512F_SUPPORT
#error "restoring AVX-512 registers is not supported"
#elif defined AVX_SUPPORT
	/* Restore AVX registers */
	vmovdqa	(%rsp), %ymm0
	vmovdqa	32(%rsp), %ymm1
	vmovdqa	64(%rsp), %ymm2
	vmovdqa	96(%rsp), %ymm3
	vmovdqa	128(%rsp), %ymm4
	vmovdqa	160(%rsp), %ymm5
	vmovdqa	192(%rsp), %ymm6
	vmovdqa	224(%rsp), %ymm7
	vmovdqa	256(%rsp), %ymm8
	vmovdqa	288(%rsp), %ymm9
	vmovdqa	320(%rsp), %ymm10
	vmovdqa	352(%rsp), %ymm11
	vmovdqa	384(%rsp), %ymm12
	vmovdqa	416(%rsp), %ymm13
	vmovdqa	448(%rsp), %ymm14
	vmovdqa	480(%rsp), %ymm15
	mov	512(%rsp), %rsp
#else
	/* Restore SSE registers */
	movdqa	(%rsp), %xmm0
	movdqa	16(%rsp), %xmm1
	movdqa	32(%rsp), %xmm2
	movdqa	48(%rsp), %xmm3
	movdqa	64(%rsp), %xmm4
	movdqa	80(%rsp), %xmm5
	movdqa	96(%rsp), %xmm6
	movdqa	112(%rsp), %xmm7
	movdqa	128(%rsp), %xmm8
	movdqa	144(%rsp), %xmm9
	movdqa	160(%rsp), %xmm10
	movdqa	176(%rsp), %xmm11
	movdqa	192(%rsp), %xmm12
	movdqa	208(%rsp), %xmm13
	movdqa	224(%rsp), %xmm14
	movdqa	240(%rsp), %xmm15
	lea	264(%rsp), %rsp
#endif

	/* Restore general-purpose registers */
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdi
	pop	%rsi
	pop	%rbp
	pop	%rbx
	pop	%rdx
	pop	%rcx

	/* Restore RAX and jump to saved pointer */
	xchg	%rax, (%rsp)
	ret
ASM_FUNC_END (int_restore_registers)
