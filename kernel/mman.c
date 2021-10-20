/* mman.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/memory.h>
#include <pml/process.h>

/*!
 * Adds a new mapped memory region to the current process's table. Pages are
 * allocated and mapped to the virtual memory area. The mapped region will
 * be filled with zero bytes.
 *
 * @base the virtual address of the first byte of the memory region
 * @len the number of bytes in the region
 * @prot memory protection flags
 * @return zero on success
 */

int
add_mmap (void *base, size_t len, int prot)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  struct mmap *table =
    realloc (mmaps->table, sizeof (struct mmap) * ++mmaps->len);
  void *ptr;
  void *cptr;
  int flags;
  if (UNLIKELY (!table))
    goto err0;
  table[mmaps->len - 1].base = base;
  table[mmaps->len - 1].len = len;
  table[mmaps->len - 1].prot = prot;
  mmaps->table = table;

  flags = PAGE_FLAG_USER;
  if (prot & PROT_WRITE)
    flags |= PAGE_FLAG_RW;

  for (ptr = ALIGN_DOWN (base, PAGE_SIZE);
       ptr < ALIGN_UP (base + len, PAGE_SIZE); ptr += PAGE_SIZE)
    {
      uintptr_t page = alloc_page ();
      if (UNLIKELY (!page))
	goto err1;
      if (vm_map_page (THIS_THREAD->args.pml4t, page, ptr, flags))
	{
	  free_page (page);
	  goto err1;
	}
    }
  return 0;

 err1:
  for (cptr = ALIGN_DOWN (base, PAGE_SIZE); cptr < ptr; cptr += PAGE_SIZE)
    free_page (physical_addr (cptr));
 err0:
  mmaps->len--;
  return -1;
}
