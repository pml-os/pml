/* init.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/cmos.h>
#include <pml/memory.h>
#include <pml/thread.h>
#include <stdlib.h>

static void
init_kernel_heap (void)
{
  uintptr_t size = ALIGN_UP (total_phys_mem / 16, PAGE_SIZE);
  kh_init (next_phys_addr, size);
  next_phys_addr += size;
}

/*!
 * Initializes architecture-specific services.
 */

void
arch_init (void)
{
  init_kernel_heap ();
  sched_init ();
  real_time = cmos_read_real_time ();
  cmos_enable_rtc_int ();
}
