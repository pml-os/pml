/* gdt.c -- This file is part of PML.
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

#include <pml/gdt.h>

static uint64_t gdt_table[5];
static struct gdt_ptr gdt_ptr;

static uint64_t
gdt_entry (uint32_t base, uint32_t limit, int rw, int dc, int can_exec,
	   int system_seg, unsigned int privilege)
{
  unsigned char access = GDT_ACC_PRESENT | GDT_ACC_PRIVILEGE (privilege);
  unsigned char flags;
  if (rw)
    access |= GDT_ACC_RW;
  if (dc)
    access |= GDT_ACC_DC;
  if (!system_seg)
    access |= GDT_ACC_TYPE;
  if (can_exec)
    {
      access |= GDT_ACC_EXECUTE;
      flags = GDT_FLAG_LONG_CODE;
    }
  else
    flags = GDT_FLAG_SIZE;
  return (limit & 0xffff)
    | ((uint64_t) (base & 0xffffff) << 16)
    | ((uint64_t) access            << 40)
    | ((uint64_t) (limit & 0xf0000) << 48)
    | ((uint64_t) (flags & 0xf)     << 52)
    | ((uint64_t) (base >> 24)      << 56);
}

void
init_gdt (void)
{
  gdt_table[0] = gdt_entry (0, 0, 0, 0, 0, 0, 0);
  gdt_table[1] = gdt_entry (0, 0xffffffff, 1, 0, 1, 0, 0);
  gdt_table[2] = gdt_entry (0, 0xffffffff, 1, 0, 0, 0, 0);
  gdt_table[3] = gdt_entry (0, 0xffffffff, 1, 0, 1, 0, 3);
  gdt_table[4] = gdt_entry (0, 0xffffffff, 1, 0, 0, 0, 3);
  gdt_ptr.size = sizeof (gdt_table) - 1;
  gdt_ptr.addr = gdt_table;
  load_gdt (&gdt_ptr);
}
