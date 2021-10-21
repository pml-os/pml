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

/*!
 * @file
 * @brief x86-64 global descriptor tables
 */

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

/*! Type used as a segment to index into the GDT. */
typedef unsigned short segment_t;

/*!
 * Format of a GDT pointer.
 */

struct gdt_ptr
{
  uint16_t size;                /*!< Number of bytes in GDT minus one */
  uint64_t *addr;               /*!< Base virtual address of GDT */
} __packed;

/*!
 * Format of the long mode task state segment (TSS).
 */

struct tss
{
  uint32_t reserved;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved2;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint16_t reserved3[5];
  uint16_t iopb;
} __packed;

/*!
 * Loads a task state segment.
 *
 * @param desc the descriptor of the TSS in the GDT
 */

static inline void
load_tss (segment_t desc)
{
  __asm__ volatile ("ltr %0" :: "r" (desc));
}

__BEGIN_DECLS

extern struct tss kernel_tss;

void init_gdt (void);
void load_gdt (const struct gdt_ptr *ptr);

__END_DECLS

#endif
