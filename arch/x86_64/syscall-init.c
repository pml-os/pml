/* syscall-init.c -- This file is part of PML.
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

#include <pml/msr.h>
#include <pml/syscall.h>

/*!
 * Initializes system calls by setting the values of the appropriate MSRs.
 */

void
syscall_init (void)
{
  uintptr_t addr = (uintptr_t) syscall;
  msr_write (MSR_STAR, 0, 0x08 | (0x08 << 16));
  msr_write (MSR_LSTAR, addr & 0xffffffff, addr >> 32);
}
