/* apic.c -- This file is part of PML.
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
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/panic.h>
#include <stdio.h>
#include <stdlib.h>

apic_id_t local_apics[MAX_CORES];   /*!< IDs of local APICs */
size_t local_apic_count;            /*!< Number of local APICs */
void *local_apic_addr;              /*!< Address of CPU local APIC */
apic_id_t ioapic_id;                /*!< ID of I/O APIC */
void *ioapic_addr;                  /*!< Address of I/O APIC */
unsigned int ioapic_gsi_base;       /*!< I/O APIC GSI base */

/*!
 * Registers a local APIC from a MADT processor local APIC entry.
 *
 * @param entry the MADT entry
 */

static void
add_local_apic (const struct acpi_madt_local_apic *entry)
{
  if (local_apic_count >= MAX_CORES)
    return;
  local_apics[local_apic_count++] = entry->local_apic_id;
  printf ("ACPI: found local APIC (%#x)\n", entry->local_apic_id);
}

/*!
 * Sets up information about the I/O APIC based on an I/O APIC MADT entry.
 *
 * @param entry the MADT entry
 */

static void
set_ioapic (const struct acpi_madt_ioapic *entry)
{
  ioapic_id = entry->ioapic_id;
  ioapic_addr = (void *) PHYS32_REL (entry->ioapic_addr);
  ioapic_gsi_base = entry->gsi_base;
  printf ("ACPI: found I/O APIC (%#x)\n", ioapic_id);
}

/*!
 * Overrides the address of the local APIC with the address specified
 * in a local APIC address override MADT entry.
 *
 * @param entry the MADT entry
 */

static void
set_local_apic_addr (const struct acpi_madt_local_apic_addr_ovr *entry)
{
  local_apic_addr = (void *) PHYS_REL (entry->local_apic_addr);
}

#ifdef USE_APIC

/*!
 * Starts the local APIC. This function is only called for the bootstrap
 * processor.
 */

void
int_start (void)
{
  LOCAL_APIC_REG (LOCAL_APIC_REG_SPURIOUS_INT_VEC) = 0x1ff;
}

#endif

/*!
 * Parses the ACPI MADT table to locate the address of the local APIC and
 * I/O APIC, and any interrupt source overrides.
 *
 * @param madt the address of the MADT
 */

void
acpi_parse_madt (const struct acpi_madt *madt)
{
#ifdef USE_APIC
  const unsigned char *ptr = madt->entries;
  const struct acpi_madt_entry *entry = (const struct acpi_madt_entry *) ptr;
  size_t i;
  for (i = offsetof (struct acpi_madt, entries); i < madt->header.len;
       i += entry->len, ptr += entry->len,
	 entry = (const struct acpi_madt_entry *) ptr)
    {
      switch (entry->type)
	{
	case ACPI_MADT_ENTRY_LOCAL_APIC:
	  add_local_apic ((const struct acpi_madt_local_apic *) entry);
	  break;
	case ACPI_MADT_ENTRY_IOAPIC:
	  set_ioapic ((const struct acpi_madt_ioapic *) entry);
	  break;
	case ACPI_MADT_ENTRY_LOCAL_APIC_ADDR_OVR:
	  set_local_apic_addr ((const struct acpi_madt_local_apic_addr_ovr *)
			       entry);
	  break;
	}
    }

  /* If no local APIC address was given, guess */
  if (!local_apic_addr)
    local_apic_addr = (void *) PHYS32_REL (LOCAL_APIC_DEFAULT_ADDR);

  /* Make sure an I/O APIC is present */
  if (UNLIKELY (!ioapic_addr))
    panic ("No I/O APIC found");
#endif
}
