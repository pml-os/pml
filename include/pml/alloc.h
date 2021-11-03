/* alloc.h -- This file is part of PML.
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

#ifndef __PML_ALLOC_H
#define __PML_ALLOC_H

/*!
 * @file
 * @brief Kernel memory allocation routines
 */

#include <pml/cdefs.h>

/* Kernel heap definitions */

/*! Must be in @ref kh_header.magic */
#define KH_HEADER_MAGIC         0x07242005
/*! Must be in @ref kh_tail.magic */
#define KH_TAIL_MAGIC           0xdeadc0de
/*! Default alignment of kernel heap objects */
#define KH_DEFAULT_ALIGN        16
/*! Minimum size of block to split during allocations */
#define KH_MIN_BLOCK_SPLIT_SIZE 32

#define KH_FLAG_ALLOC           (1 << 0)    /*!< Block is allocated */

/*!
 * Header for a block in the kernel heap. This structure is placed in front
 * of every allocated and free block.
 */

struct kh_header
{
  uint32_t magic;               /*!< Must be @ref KH_HEADER_MAGIC */
  uint32_t flags;               /*!< Block flags */
  uint64_t size;                /*!< Size of block data in bytes */
};

/*!
 * Tail for a block in the kernel heap. This structure is placed behind
 * every allocated and free block.
 */

struct kh_tail
{
  uint32_t magic;               /*!< Must be @ref KH_TAIL_MAGIC */
  uint32_t reserved;            /*!< Reserved, must be zero */
  struct kh_header *header;     /*!< Pointer to the corresponding header */
};

__BEGIN_DECLS

uintptr_t alloc_page (void);
void ref_page (uintptr_t addr);
void free_page (uintptr_t addr);
void *alloc_virtual_page (void);
void free_virtual_page (void *ptr);

void kh_init (uintptr_t base, size_t size);
void *kh_alloc_aligned (size_t size, size_t align);
void *kh_realloc (void *ptr, size_t size);
void kh_free (void *ptr);

__END_DECLS

#endif
