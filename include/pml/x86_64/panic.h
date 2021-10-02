/* panic.h -- This file is part of PML.
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

#ifndef __PML_PANIC_H
#define __PML_PANIC_H

#include <pml/cdefs.h>

/* Kernel panic routine for x86_64 */

#define panic(fmt, ...) do						\
    {									\
      __asm__ volatile ("push %%rax\n"					\
			"push %%rcx\n"					\
			"push %%rdx\n"					\
			"push %%rbx\n"					\
			"lea -32(%%rsp), %%rax\n"			\
			"push %%rax\n"					\
			"push %%rbp\n"					\
			"push %%rsi\n"					\
			"push %%rdi\n"					\
			"movabs %0, %%rax\n"				\
			"popq 56(%%rax)\n"				\
			"popq 48(%%rax)\n"				\
			"popq 40(%%rax)\n"				\
			"popq 32(%%rax)\n"				\
			"popq 24(%%rax)\n"				\
			"popq 16(%%rax)\n"				\
			"popq 8(%%rax)\n"				\
			"popq (%%rax)\n"				\
			"add $8, %%rsp" ::				\
			"i" (panic_registers) : "rax");			\
      __panic (fmt, ## __VA_ARGS__);					\
    }									\
  while (0)

__BEGIN_DECLS

extern uint64_t panic_registers[8];

void __panic (const char *__restrict__ fmt, ...) __cold __noreturn;

__END_DECLS

#endif
