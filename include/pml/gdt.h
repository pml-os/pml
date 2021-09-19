/* gdt.h -- This file is part of PML.
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

#ifndef __PML_GDT_H
#define __PML_GDT_H

#include <pml/cdefs.h>

#define GDT_ACC_ACCESS         (1 << 0)
#define GDT_ACC_RW             (1 << 1)
#define GDT_ACC_DC             (1 << 2)
#define GDT_ACC_EXECUTE        (1 << 3)
#define GDT_ACC_TYPE           (1 << 4)
#define GDT_ACC_PRESENT        (1 << 7)
#define GDT_ACC_PRIVILEGE(r)   ((r & 3) << 5)

#define GDT_FLAG_LONG_CODE     (1 << 1)
#define GDT_FLAG_SIZE          (1 << 2)
#define GDT_FLAG_GRANULARITY   (1 << 3)

struct gdt_ptr
{
  uint16_t size;
  uint64_t *addr;
} __packed;

__BEGIN_DECLS

void init_gdt (void);
void load_gdt (const struct gdt_ptr *ptr);

__END_DECLS

#endif
