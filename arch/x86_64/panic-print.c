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

/** @file */

#include <stdio.h>
#include <stdint.h>

/*!
 * Buffer to store registers on kernel panic. 
 */
uint64_t panic_registers[8];

/*!
 * Prints a kernel panic message. Called by the kernel panic function.
 *
 * @param fmt the printf-style format string to print
 */

void
panic_print_message (const char *__restrict__ fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  printf ("\n==========[ Kernel Panic ]==========\n");
  vprintf (fmt, args);
  va_end (args);
}

/*!
 * Prints the registers obtained from a kernel panic.
 *
 * @param rax the value of RAX
 * @param rcx the value of RCX
 * @param rdx the value of RDX
 * @param rbx the value of RBX
 * @param rsp the value of the stack pointer
 * @param rbp the value of the stack base pointer
 * @param rsi the value of RSI
 * @param rdi the value of RDI
 */

void
panic_print_registers (uint64_t rax, uint64_t rcx, uint64_t rdx, uint64_t rbx,
		       uint64_t rsp, uint64_t rbp, uint64_t rsi, uint64_t rdi)
{
  printf ("\n\nRegisters:\n"
	  "RAX %#016lx    RCX %#016lx\n"
	  "RDX %#016lx    RBX %#016lx\n"
	  "RSP %#016lx    RBP %#016lx\n"
	  "RSI %#016lx    RDI %#016lx\n",
	  rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi);
}
