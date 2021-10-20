/* mman.h -- This file is part of PML.
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

#ifndef __PML_MMAN_H
#define __PML_MMAN_H

/*!
 * @file
 * @brief Memory mapping routines
 */

#include <pml/cdefs.h>

#define PROT_NONE               (1 << 0)    /*!< Memory is not accessible */
#define PROT_READ               (1 << 1)    /*!< Memory may be read */
#define PROT_WRITE              (1 << 2)    /*!< Memory may be written */
#define PROT_EXEC               (1 << 3)    /*!< Memory may be executed */

/*!
 * Stores info about an allocated region in the user-space half of an
 * address space.
 */

struct mmap
{
  void *base;                   /*!< Virtual address of memory region base */
  size_t len;                   /*!< Length of memory region */
  int prot;                     /*!< Memory protection of region */
};

/*!
 * Represents a table of memory regions allocated to a process.
 */

struct mmap_table
{
  struct mmap *table;           /*!< Array of memory region structures */
  size_t len;                   /*!< Number of memory regions */
};

__BEGIN_DECLS

int add_mmap (void *base, size_t len, int prot);

__END_DECLS

#endif
