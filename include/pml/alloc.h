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

#include <pml/cdefs.h>

/* Kernel heap definitions */

#define KH_HEADER_MAGIC         0x07242005
#define KH_TAIL_MAGIC           0xdeadc0de
#define KH_DEFAULT_ALIGN        16
#define KH_MIN_BLOCK_SPLIT_SIZE 32

#define KH_FLAG_ALLOC           (1 << 0)

struct kh_header
{
  uint32_t magic;
  uint32_t flags;
  uint64_t size;
};

struct kh_tail
{
  uint32_t magic;
  uint32_t reserved;
  struct kh_header *header;
};

__BEGIN_DECLS

uintptr_t alloc_page (void);
void free_page (uintptr_t addr);

void kh_init (uintptr_t base, size_t size);
void *kh_alloc_aligned (size_t size, size_t align);
void *kh_realloc (void *ptr, size_t size);
void kh_free (void *ptr);

__END_DECLS

#endif
