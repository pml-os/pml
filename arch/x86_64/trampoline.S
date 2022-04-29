/* trampoline.S -- This file is part of PML.
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

/* Routines included in a page mapped with user execution privileges */

#include <pml/asm.h>
#include <pml/memory.h>

	.align PAGE_SIZE
	.global signal_trampoline
ASM_FUNC_BEGIN (signal_trampoline):
	xor	%ebp, %ebp
	call	*%r12
	call	int_restore_registers
	ret
ASM_FUNC_END (signal_trampoline)

	.global int_save_registers
ASM_FUNC_BEGIN (int_save_registers):
	/* Save return pointer, RAX, and flags */
	xchg	%rax, (%rsp)
	pushf

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
	/* Save AVX registers */
	lea	-512(%rsp), %rsp
	vmovdqu	%ymm0, (%rsp)
	vmovdqu	%ymm1, 32(%rsp)
	vmovdqu	%ymm2, 64(%rsp)
	vmovdqu	%ymm3, 96(%rsp)
	vmovdqu	%ymm4, 128(%rsp)
	vmovdqu	%ymm5, 160(%rsp)
	vmovdqu	%ymm6, 192(%rsp)
	vmovdqu	%ymm7, 224(%rsp)
	vmovdqu	%ymm8, 256(%rsp)
	vmovdqu	%ymm9, 288(%rsp)
	vmovdqu	%ymm10, 320(%rsp)
	vmovdqu	%ymm11, 352(%rsp)
	vmovdqu	%ymm12, 384(%rsp)
	vmovdqu	%ymm13, 416(%rsp)
	vmovdqu	%ymm14, 448(%rsp)
	vmovdqu	%ymm15, 480(%rsp)
#else
	/* Save SSE registers */
	lea	-256(%rsp), %rsp
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
	vmovdqu	(%rsp), %ymm0
	vmovdqu	32(%rsp), %ymm1
	vmovdqu	64(%rsp), %ymm2
	vmovdqu	96(%rsp), %ymm3
	vmovdqu	128(%rsp), %ymm4
	vmovdqu	160(%rsp), %ymm5
	vmovdqu	192(%rsp), %ymm6
	vmovdqu	224(%rsp), %ymm7
	vmovdqu	256(%rsp), %ymm8
	vmovdqu	288(%rsp), %ymm9
	vmovdqu	320(%rsp), %ymm10
	vmovdqu	352(%rsp), %ymm11
	vmovdqu	384(%rsp), %ymm12
	vmovdqu	416(%rsp), %ymm13
	vmovdqu	448(%rsp), %ymm14
	vmovdqu	480(%rsp), %ymm15
	lea	512(%rsp), %rsp
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
	lea	256(%rsp), %rsp
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

	/* Restore RAX, flags, and jump to saved pointer */
	popf
	xchg	%rax, (%rsp)
	ret
ASM_FUNC_END (int_restore_registers)

	.global reg_save_size
ASM_FUNC_BEGIN (reg_save_size):
#if defined AVX512_SUPPORT || defined AVX_SUPPORT
#error "AVX-512 and AVX are not supported"
#else
	mov	$384, %eax
#endif
	ret
ASM_FUNC_END (reg_save_size)
	.align PAGE_SIZE
