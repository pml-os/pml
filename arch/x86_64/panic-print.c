/* panic-print.c -- This file is part of PML.
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

#include <stdio.h>
#include <stdint.h>

uint64_t panic_registers[8];

void
panic_print_message (const char *__restrict__ fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  printf ("\n==========[ Kernel Panic ]==========\n");
  vprintf (fmt, args);
  va_end (args);
}

void
panic_print_registers (uint64_t rax, uint64_t rcx, uint64_t rdx, uint64_t rbx,
		       uint64_t rsp, uint64_t rbp, uint64_t rsi, uint64_t rdi)
{
  printf ("\n\nRegisters:\n"
	  "RAX %#016lx    RCX %#016lx\n"
	  "RDX %#016lx    RBX %#016lx\n"
	  "RSP %#016lx    RBP %#016lx\n"
	  "RSI %#016lx    RDI %#016lx\n"
	  "CS  %#04x    DS  %#04x    ES  %#04x\n"
	  "FS  %#04x    GS  %#04x    SS  %#04x\n",
	  rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi);
}
