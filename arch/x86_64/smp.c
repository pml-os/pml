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

#include <pml/alloc.h>
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/pit.h>
#include <stdlib.h>
#include <string.h>

extern void *smp_ap_start;
extern void *smp_ap_size;

/*! Set to one when an application processor has been initialized. */
volatile int smp_ap_setup_done;

/*!
 * Initializes any additional processors using symmetric multiprocessing.
 * This function must be called with interrupts enabled since it relies on
 * the PIT timer for delays.
 */

void
smp_init (void)
{
#ifdef ENABLE_SMP
  size_t i;

  /* Copy AP startup code to low memory */
  memcpy ((void *) PHYS32_REL (SMP_AP_START_ADDR), &smp_ap_start,
	  (size_t) &smp_ap_size);

  for (i = 0; i < local_apic_count; i++)
    {
      /* Initialize each processor. Make sure to not initialize the processor
	 with the same ID as the BSP since it is already running. */
      apic_id_t id = local_apics[i];
      if (id != bsp_id)
	{
	  uintptr_t stack_page;
	  smp_ap_setup_done = 0;
	  stack_page = alloc_page ();
	  if (UNLIKELY (!stack_page))
	    continue; /* Couldn't allocate a stack for the new processor */
	  *((uintptr_t *) PHYS32_REL (SMP_AP_INIT_STACK)) =
	    PHYS_REL (stack_page) + 0xff8;
	  local_apic_clear_errors ();
	  local_apic_int (0, id, APIC_MODE_INIT, 0, 1);
	  local_apic_int (0, id, APIC_MODE_INIT, 1, 1);
	  pit_sleep (10);
	  local_apic_clear_errors ();
	  local_apic_int (8, id, APIC_MODE_STARTUP, 0, 0);
	  pit_sleep (10);
	  local_apic_clear_errors ();
	  local_apic_int (8, id, APIC_MODE_STARTUP, 0, 0);
	  while (!smp_ap_setup_done)
	    ;
	}
    }
#endif /* ENABLE_SMP */
}
