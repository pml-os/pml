/* smp.c -- This file is part of PML.
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

#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/pit.h>
#include <string.h>

extern void *smp_ap_start;
extern void *smp_ap_size;

/*!
 * Initializes any additional processors using symmetric multiprocessing.
 * This function must be called with interrupts enabled since it relies on
 * a timer.
 */

void
smp_init (void)
{
#ifdef ENABLE_SMP
  size_t i;

  /* Copy AP startup code and kernel PML4T pointer to low memory */
  *((unsigned int *) PHYS32_REL (SMP_AP_KERNEL_PML4T)) =
    (uintptr_t) kernel_pml4t - KERNEL_VMA;
  memcpy ((void *) PHYS32_REL (SMP_AP_START_ADDR), &smp_ap_start,
	  (size_t) &smp_ap_size);

  for (i = 0; i < local_apic_count; i++)
    {
      /* Initialize each processor. Make sure to not initialize the processor
	 with the same ID as the BSP since it is already running. */
      apic_id_t id = local_apics[i];
      if (id != bsp_id)
	{
	  local_apic_clear_errors ();
	  local_apic_int (0, id, APIC_MODE_INIT, 0, 1);
	  local_apic_int (0, id, APIC_MODE_INIT, 1, 1);
	  pit_sleep (10);
	  local_apic_clear_errors ();
	  local_apic_int (8, id, APIC_MODE_STARTUP, 0, 0);
	  pit_sleep (10);
	  local_apic_clear_errors ();
	  local_apic_int (8, id, APIC_MODE_STARTUP, 0, 0);
	}
    }
#endif /* ENABLE_SMP */
}
