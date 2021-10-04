/* hpet.c -- This file is part of PML.
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
#include <pml/hpet.h>
#include <pml/interrupt.h>
#include <pml/memory.h>

/*! Address of memory-mapped HPET registers. */
uintptr_t hpet_addr;

/*! Whether the HPET main counter is running */
int hpet_active;

/*! Number of HPET ticks equivalent to one second */
unsigned long hpet_resolution;

/*!
 * Reads the value of the main counter of the HPET.
 *
 * @return the number of nanoseconds elapsed since an arbitrary point in time
 */

clock_t
hpet_nanotime (void)
{
  return HPET_REG (HPET_REG_COUNTER_VALUE) * 1000000000 / hpet_resolution;
}

/*!
 * Sets up the HPET from the information in the HPET ACPI table. The HPET
 * counter is reset to zero.
 *
 * @param hpet the address of the HPET ACPI table
 */

void
acpi_parse_hpet (const struct acpi_hpet *hpet)
{
  hpet_addr = PHYS_REL (hpet->addr.addr);
  hpet_active = 1;
  hpet_resolution = HPET_REG (HPET_REG_CAP_ID) >> 32;
  HPET_REG (HPET_REG_COUNTER_VALUE) = 0;
  HPET_REG (HPET_REG_CONFIG) |= HPET_CONFIG_ENABLE;
}
