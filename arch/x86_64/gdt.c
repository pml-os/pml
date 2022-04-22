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

/*! @file */

#include <pml/gdt.h>
#include <pml/memory.h>

/*! 
 * The kernel task state segment, used for interrupts between privilege
 * levels.
 */

struct tss kernel_tss;

/*! The kernel GDT */
static uint64_t gdt_table[7];

/*! The kernel GDT pointer */
static struct gdt_ptr gdt_ptr;

/*!
 * Creates an entry in the global descriptor table.
 *
 * @param base the base address of the segment
 * @param limit the size of the segment
 * @param rw whether the segment allows write access
 * @param dc whether to set the direction or conforming bit
 * @param can_exec whether the segment allows executing code
 * @param tss whether the segment is a TSS
 * @param privilege the minimum privilege level required
 * @return a 64-bit integer suitable as a value in the GDT
 */

static uint64_t
gdt_entry (uint32_t base, uint32_t limit, int rw, int dc, int can_exec,
	   int tss, unsigned int privilege)
{
  unsigned char access = GDT_ACC_PRESENT | GDT_ACC_PRIVILEGE (privilege);
  unsigned char flags;
  if (rw)
    access |= GDT_ACC_RW;
  if (dc)
    access |= GDT_ACC_DC;
  if (tss)
    access |= GDT_ACC_ACCESS;
  else
    access |= GDT_ACC_TYPE;
  if (can_exec)
    {
      access |= GDT_ACC_EXECUTE;
      if (!tss)
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

/*
 * Initializes the kernel global descriptor table.
 */

void
init_gdt (void)
{
  uintptr_t tss = (uintptr_t) &kernel_tss;
  kernel_tss.rsp0 = INTERRUPT_STACK_TOP_VMA;

  gdt_table[0] = gdt_entry (0, 0, 0, 0, 0, 0, 0);
  gdt_table[1] = gdt_entry (0, 0xffffffff, 1, 0, 1, 0, 0);
  gdt_table[2] = gdt_entry (0, 0xffffffff, 1, 0, 0, 0, 0);
  gdt_table[3] = gdt_entry (0, 0xffffffff, 1, 0, 0, 0, 3);
  gdt_table[4] = gdt_entry (0, 0xffffffff, 1, 0, 1, 0, 3);
  gdt_table[5] = gdt_entry (tss & 0xffffffff, sizeof (struct tss),
			    0, 0, 1, 1, 0);
  gdt_table[6] = tss >> 32;

  gdt_ptr.size = sizeof (gdt_table) - 1;
  gdt_ptr.addr = gdt_table;
  load_gdt (&gdt_ptr);
  load_tss (0x28);
}
