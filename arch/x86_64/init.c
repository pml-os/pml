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

#include <pml/acpi.h>
#include <pml/alloc.h>
#include <pml/cmos.h>
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/panic.h>
#include <pml/pit.h>
#include <pml/process.h>
#include "syscall.h"

/*! Initializes the kernel heap. */

static void
init_kernel_heap (void)
{
  uintptr_t size = ALIGN_UP (total_phys_mem / 16, PAGE_SIZE);
  kh_init (next_phys_addr, size);
  next_phys_addr += size;
}

/*! Initializes the user mode trampoline. */

static void
user_mode_tp_init (void)
{
  uintptr_t page = alloc_page ();
  if (UNLIKELY (!page))
    panic ("Failed to initialize user mode trampoline");
  vm_map_page (THIS_THREAD->args.pml4t, page, (void *) USER_MODE_TP_BASE_VMA,
	       PAGE_FLAG_RW | PAGE_FLAG_USER);
}

/*!
 * Initializes architecture-specific services.
 */

void
arch_init (void)
{
  /* Initialize the heap first since it could be needed by other steps */
  init_kernel_heap ();

  /* Remap the 8259 PIC and disable it if using the APIC */
  pic_8259_remap ();
#ifdef USE_APIC
  pic_8259_disable ();
#endif

  /* Initialize ACPI and related services */
  pit_set_freq (0, 1000);
  acpi_init ();

  /* Initialize the real-time clock and enable interrupts */
  real_time = cmos_read_real_time ();
  cmos_enable_rtc_int ();

  /* Initialize the scheduler */
  sched_init ();

  /* Start interrupts, system calls, and user mode */
  int_start ();
  int_enable ();
  syscall_init ();
  user_mode_tp_init ();

  /* Start multiple cores if SMP is supported */
  smp_init ();
}
