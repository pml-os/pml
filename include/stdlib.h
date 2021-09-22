/* stdlib.h -- This file is part of PML.
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

#ifndef __STDLIB_H
#define __STDLIB_H

#include <pml/cdefs.h>

#define ALIGNED(x, s) (!((uintptr_t) (x) & ((s) - 1)))
#define LONG_NULL(x) (((x) - 0x0101010101010101) & ~(x) & 0x8080808080808080)
#define LONG_CHAR(x, c) LONG_NULL ((x) ^ (c))

#define ALIGN_DOWN(x, a) ((__typeof__ (x)) ((uintptr_t) (x) & ~((a) - 1)))
#define ALIGN_UP(x, a)						\
  ((__typeof__ (x)) ((((uintptr_t) (x) - 1) | ((a) - 1)) + 1))
#define IS_P2(x) ((x) != 0 && !((x) & ((x) - 1)))

#define LIKELY(x)               (__builtin_expect (!!(x), 1))
#define UNLIKELY(x)             (__builtin_expect (!!(x), 0))

__BEGIN_DECLS

unsigned long strtoul (const char *__restrict__ str, char **__restrict__ end,
		       int base) __pure;
void abort (void) __noreturn;

void *malloc (size_t size);
void *calloc (size_t block, size_t size);
void *aligned_alloc (size_t align, size_t size);
void *valloc (size_t size);
void *realloc (void *ptr, size_t size);
void free (void *ptr);

__END_DECLS

#endif
