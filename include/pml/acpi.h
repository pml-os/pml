/* acpi.h -- This file is part of PML.
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

#ifndef __PML_ACPI_H
#define __PML_ACPI_H

/*! @file */

#include <pml/cdefs.h>

/*!
 * Format of the RSDP before ACPI 2.0. This structure contains the signature,
 * checksum, revision, and a 32-bit pointer to only the RSDT.
 */

struct acpi_rsdp_old
{
  char signature[8];            /*!< RSDP signature (<tt>"RSD PTR "</tt>) */
  unsigned char checksum;
  char oem_id[6];
  unsigned char revision;       /*!< Nonzero if ACPI 2.0 or newer */
  uint32_t rsdt_addr;           /*!< Physical address of the RSDT */
};

/*!
 * Format of the RSDP after ACPI 2.0. This structure extends the old RSDP
 * (@ref acpi_rsdp_old) and includes the structure length and a 64-bit
 * pointer to the XSDT.
 */

struct acpi_rsdp
{
  struct acpi_rsdp_old old;     /*!< Old RSDP structure */
  uint32_t len;                 /*!< Length of RSDP in bytes */
  uint64_t xsdt_addr;           /*!< Physical address of the XSDT */
  unsigned char ext_checksum;
  unsigned char reserved[3];
};

/*!
 * Format of an ACPI table header. All ACPI tables begin with this structure
 * and can be identified with its @ref acpi_table_header.signature field.
 */

struct acpi_table_header
{
  char signature[4];            /*!< Table signature */
  uint32_t len;                 /*!< Length of table in bytes */
  unsigned char revision;
  unsigned char checksum;
  char oem_id[6];
  char oem_table_id[6];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
};

/*!
 * Format of the RSDT. Contains a header followed by an array of 32-bit
 * physical addresses to other ACPI tables.
 */

struct acpi_rsdt
{
  struct acpi_table_header header;
  uint32_t tables[];
};

/*!
 * Format of the XSDT in ACPI 2.0 and newer. Contains a header followed by
 * an array of 64-bit physical addresses to other ACPI tables.
 */

struct acpi_xsdt
{
  struct acpi_table_header header;
  uint64_t tables[];
};

__BEGIN_DECLS

extern struct acpi_rsdp *acpi_rsdp;

void acpi_init (void);
void acpi_parse_table (const struct acpi_table_header *header);
void acpi_parse_rsdt (const struct acpi_rsdt *rsdt);
void acpi_parse_xsdt (const struct acpi_xsdt *xsdt);
int acpi_rsdp_checksum (const struct acpi_rsdp *rsdp, int acpi2);
int acpi_table_checksum (const struct acpi_table_header *header);

__END_DECLS

#endif
