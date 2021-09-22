/* malloc.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/memory.h>
#include <stdlib.h>
#include <string.h>

void *
malloc (size_t size)
{
  return kh_alloc_aligned (size, KH_DEFAULT_ALIGN);
}

void *
calloc (size_t block, size_t size)
{
  void *ptr = malloc (block * size);
  if (LIKELY (ptr))
    memset (ptr, 0, block * size);
  return ptr;
}

void *
aligned_alloc (size_t align, size_t size)
{
  return kh_alloc_aligned (size, align);
}

void *
valloc (size_t size)
{
  return kh_alloc_aligned (size, PAGE_SIZE);
}

void *
realloc (void *ptr, size_t size)
{
  return kh_realloc (ptr, size);
}

void
free (void *ptr)
{
  return kh_free (ptr);
}
