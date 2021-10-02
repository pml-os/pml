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

#include <pml/cdefs.h>

struct acpi_rsdp_old
{
  char signature[8];
  unsigned char checksum;
  char oem_id[6];
  unsigned char revision;
  uint32_t rsdt_addr;
};

struct acpi_rsdp
{
  struct acpi_rsdp_old old;
  uint32_t len;
  uint64_t xsdt_addr;
  unsigned char ext_checksum;
  unsigned char reserved[3];
};

struct acpi_table_header
{
  char signature[4];
  uint32_t len;
  unsigned char revision;
  unsigned char checksum;
  char oem_id[6];
  char oem_table_id[6];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
};

__BEGIN_DECLS

extern struct acpi_rsdp *acpi_rsdp;

void acpi_init (void);
int acpi_rsdp_checksum (const struct acpi_rsdp *rsdp);
int acpi_table_checksum (const struct acpi_table_header *header);

__END_DECLS

#endif
