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

/*! @file */

#include <pml/acpi.h>
#include <pml/memory.h>
#include <pml/panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*! Nonzero if ACPI 2.0 or newer is available. */
int acpi2;

struct acpi_rsdp *acpi_rsdp;
struct acpi_fadt *acpi_fadt;
struct acpi_table_header *acpi_dsdt;

/*!
 * Initializes the ACPI driver and panics if no RSDP was detected during
 * an earlier boot phase.
 */

void
acpi_init (void)
{
  if (UNLIKELY (!acpi_rsdp))
    panic ("No ACPI RSDP found");
  acpi2 = acpi_rsdp->old.revision;
  if (UNLIKELY (acpi_rsdp_checksum (acpi_rsdp)))
    panic ("Bad checksum or signature on ACPI RSDP");
  if (acpi2 && acpi_rsdp->xsdt_addr)
    {
      const struct acpi_xsdt *xsdt =
	(const struct acpi_xsdt *) PHYS_REL (acpi_rsdp->xsdt_addr);
      printf ("ACPI: using XSDT at %p\n", xsdt);
      acpi_parse_xsdt (xsdt);
    }
  else
    {
      const struct acpi_rsdt *rsdt =
	(const struct acpi_rsdt *) PHYS32_REL (acpi_rsdp->old.rsdt_addr);
      printf ("ACPI: using RSDT at %p\n", rsdt);
      acpi_parse_rsdt (rsdt);
    }
}

void
acpi_parse_table (const struct acpi_table_header *header)
{
  char buffer[5];
  strncpy (buffer, header->signature, 4);
  buffer[4] = '\0';

  if (UNLIKELY (acpi_table_checksum (header)))
    {
      printf ("ACPI: ignoring %s table with bad checksum at %p\n", buffer,
	      header);
      return;
    }
  printf ("ACPI: found table with signature %s at %p\n", buffer, header);

  if (!strncmp (header->signature, "APIC", 4))
    acpi_parse_madt ((const struct acpi_madt *) header);
  else if (!strncmp (header->signature, "FACP", 4))
    acpi_parse_fadt ((const struct acpi_fadt *) header);
  else if (!strncmp (header->signature, "HPET", 4))
    acpi_parse_hpet ((const struct acpi_hpet *) header);
}

/*!
 * Parses all tables in the RSDT.
 *
 * @param rsdt the RSDT to use
 */

void
acpi_parse_rsdt (const struct acpi_rsdt *rsdt)
{
  size_t entries = (rsdt->header.len - sizeof (struct acpi_table_header)) / 4;
  size_t i;
  for (i = 0; i < entries; i++)
    {
      const struct acpi_table_header *header =
	(const struct acpi_table_header *) PHYS32_REL (rsdt->tables[i]);
      acpi_parse_table (header);
    }
}

/*!
 * Parses all tables in the XSDT.
 *
 * @param xsdt the XSDT to use
 */

void
acpi_parse_xsdt (const struct acpi_xsdt *xsdt)
{
  size_t entries = (xsdt->header.len - sizeof (struct acpi_table_header)) / 8;
  size_t i;
  for (i = 0; i < entries; i++)
    {
      const struct acpi_table_header *header =
	(const struct acpi_table_header *) PHYS_REL (xsdt->tables[i]);
      acpi_parse_table (header);
    }
}

/*!
 * Parses the Fixed ACPI Description Table (FADT). Saves a pointer to the FADT
 * and the DSDT.
 *
 * @param fadt the address of the FADT
 */

void
acpi_parse_fadt (const struct acpi_fadt *fadt)
{
  acpi_fadt = (struct acpi_fadt *) fadt;
  if (acpi2 && fadt->x_dsdt)
    acpi_dsdt = (struct acpi_table_header *) PHYS_REL (fadt->x_dsdt);
  else
    acpi_dsdt = (struct acpi_table_header *) PHYS32_REL (fadt->dsdt);
  if (UNLIKELY (acpi_table_checksum (acpi_dsdt)))
    panic ("ACPI: bad checksum on DSDT");
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
  if (acpi2)
    {
      if (c)
	return -1;
      for (; i < rsdp->len; i++)
	c += *ptr++;
    }
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
