/* acpi.c -- This file is part of PML.
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
#include <pml/panic.h>
#include <stdlib.h>
#include <string.h>

struct acpi_rsdp *acpi_rsdp;

/*!
 * Initializes the ACPI driver and panics if no RSDP was detected during
 * an earlier boot phase.
 */

void
acpi_init (void)
{
  if (UNLIKELY (!acpi_rsdp))
    panic ("No ACPI RSDP found");
  if (UNLIKELY (acpi_rsdp_checksum (acpi_rsdp)))
    panic ("Bad checksum or signature on ACPI RSDP");
}

/*!
 * Verifies the checksum and signature of the ACPI RSDP.
 *
 * @param rsdp the RSDP pointer to verify
 * @return zero if the RSDP is valid
 */

int
acpi_rsdp_checksum (const struct acpi_rsdp *rsdp)
{
  const unsigned char *ptr = (const unsigned char *) rsdp;
  unsigned char c = 0;
  size_t i;
  if (strncmp (rsdp->old.signature, "RSD PTR ", 8))
    return -1;
  for (i = 0; i < sizeof (struct acpi_rsdp_old); i++)
    c += *ptr++;
  if (c)
    return -1;
  for (; i < rsdp->len; i++)
    c += *ptr++;
  return c;
}

/*!
 * Verifies the checksum of an ACPI table.
 *
 * @param header the header of the table to verify
 * @return zero if the table is valid
 */

int
acpi_table_checksum (const struct acpi_table_header *header)
{
  const unsigned char *ptr = (const unsigned char *) header;
  unsigned char c = 0;
  size_t i;
  for (i = 0; i < header->len; i++)
    c += ptr[i];
  return c;
}
