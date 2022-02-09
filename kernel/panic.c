/* panic.c -- This file is part of PML.
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

/*! @file */

#include <pml/panic.h>
#include <stdio.h>

/*!
 * Prints a kernel panic message and halts execution of the CPU.
 *
 * @param fmt the printf-style format string to print
 */

void
panic (const char *__restrict__ fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  printf ("\n====================[ Kernel Panic ]====================\n");
  vprintf (fmt, args);
  va_end (args);
  printf ("\n\n\n");
  __asm__ volatile ("1: hlt; jmp 1b");
  __builtin_unreachable ();
}
