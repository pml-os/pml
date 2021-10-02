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

#include <pml/acpi.h>
#include <pml/interrupt.h>
#include <stdio.h>

/*!
 * Parses the ACPI MADT table to locate the address of the local APIC and
 * I/O APIC, and any interrupt source overrides.
 *
 * @param madt the address of the MADT
 */

void
acpi_parse_madt (const struct acpi_madt *madt)
{
  const unsigned char *ptr = madt->entries;
  const struct acpi_madt_entry *entry = (const struct acpi_madt_entry *) ptr;
  size_t i;
  for (i = offsetof (struct acpi_madt, entries); i < madt->header.len;
       i += entry->len, ptr += entry->len,
	 entry = (const struct acpi_madt_entry *) ptr)
    printf ("ACPI: MADT entry (type %#x, length %u)\n", entry->type,
	    entry->len);
}
