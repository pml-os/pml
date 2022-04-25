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
#include <pml/syscall.h>
#include <errno.h>
#include <string.h>

/*!
 * Locates a memory region with a base address before a requested address.
 *
 * @param base the address to search with
 * @return the index in the mmap table of the region
 */

static size_t
find_region_before (uintptr_t base)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  size_t first = 0;
  size_t last = mmaps->len - 1;
  while (first < last)
    {
      size_t mid = (first + last) / 2;
      if (mmaps->table[mid].base > base)
	last = mid - 1;
      else
	first = mid;
    }
  return first;
}

/*!
 * Locates a memory region with a base address after a requested address.
 *
 * @param base the address to search with
 * @return the index in the mmap table of the region
 */

static size_t
find_region_after (uintptr_t base)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  size_t first = 0;
  size_t last = mmaps->len - 1;
  while (first <= last)
    {
      size_t mid = (first + last) / 2;
      if (mmaps->table[mid].base > base)
	last = mid - 1;
      else
	first = mid + 1;
    }
  return last;
}

/*!
 * Removes all mappings or parts of mappings contained in an area in
 * virtual memory, optionally writing the contents of the memory to disk.
 *
 * @param addr the base address of the region to clear
 * @param len number of bytes to clear
 * @param sync whether to write the contents of the memory to disk
 * @return zero on success
 */

static int
clear_mappings (void *addr, size_t len, int sync)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  struct mmap *region;
  uintptr_t ptr = (uintptr_t) addr;
  uintptr_t i;
  size_t ri;

  if ((ptr & (PAGE_SIZE - 1)) || (len & (PAGE_SIZE - 1)))
    RETV_ERROR (EINVAL, -1);

  /* Find the last mapping under the requested address */
  ri = find_region_before (ptr);
  region = mmaps->table + ri;
  if (region->base + region->len > ptr)
    {
      /* The end of the region overlaps, truncate it */
      region->len = ptr - region->base;
      for (i = region->base + region->len; i < ptr; i += PAGE_SIZE)
	free_page (physical_addr ((void *) i));
    }

  for (i = ri + 1; i < mmaps->len; i++)
    {
      region = mmaps->table + i;
      if (ptr + len >= region->base + region->len)
	{
	  /* The region is entirely overlapped, remove it completely */
	  uintptr_t j;
	  if (sync && sys_msync ((void *) region->base, region->len, MS_SYNC))
	    return -1;
	  for (j = region->base; j < region->base + region->len;
	       j += PAGE_SIZE)
	    free_page (physical_addr ((void *) j));
	  mmaps->len--;
	  memmove (mmaps->table + i, mmaps->table + i + 1,
		   sizeof (struct mmap) * (mmaps->len - i));
	  i--;
	}
      else if (ptr + len >= region->base)
	{
	  /* The region's start overlaps, remove that portion */
	  uintptr_t j;
	  size_t diff = region->base - ptr - len;
	  if (sync && sys_msync ((void *) region->base, diff, MS_SYNC))
	    return -1;
	  for (j = region->base; j < ptr + len; j += PAGE_SIZE)
	    free_page (physical_addr ((void *) j));
	  mmaps->table[i].base = ptr + len;
	  mmaps->table[i].offset += diff;
	}
      else
	break;
    }
  return 0;
}

void *
sys_mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  struct mmap *temp;
  uintptr_t base;
  uintptr_t ptr;
  uintptr_t cptr;
  struct mmap *region;
  struct fd *file = NULL;
  struct vnode *vp = NULL;
  size_t next;
  size_t i;
  int pflags = prot != PROT_NONE ? PAGE_FLAG_USER : 0;
  size_t bytes = len;

  /* Check arguments */
  if (!(flags & MAP_ANONYMOUS))
    {
      file = file_fd (fd);
      if (!file)
	RETV_ERROR (EBADF, MAP_FAILED);
      vp = file->vnode;
      if (!S_ISREG (vp->mode))
	RETV_ERROR (EACCES, MAP_FAILED);
      if (offset < 0 || (size_t) offset > vp->size
	  || (offset & (PAGE_SIZE - 1)))
	RETV_ERROR (EINVAL, MAP_FAILED);
    }
  if (!len)
    RETV_ERROR (EINVAL, MAP_FAILED);

  /* Check file descriptor access mode, if present */
  if (vp)
    {
      if (!(prot & PROT_WRITE) && (file->flags & O_ACCMODE) == O_WRONLY)
	RETV_ERROR (EACCES, MAP_FAILED);
      if (!(prot & PROT_READ) && (file->flags & O_ACCMODE) == O_RDONLY)
	RETV_ERROR (EACCES, MAP_FAILED);
    }

  /* Check that only one of MAP_SHARED or MAP_PRIVATE was included in flags */
  if (!!(flags & MAP_SHARED) == !!(flags & MAP_PRIVATE))
    RETV_ERROR (EINVAL, MAP_FAILED);

  /* Determine the address to place the mapping */
  len = ((len - 1) | (PAGE_SIZE - 1)) + 1;
  if (addr)
    {
      base = (uintptr_t) addr;
      if (base & (PAGE_SIZE - 1))
	RETV_ERROR (EINVAL, MAP_FAILED);
    }
  else
    base = USER_MMAP_BASE_VMA;

  if ((flags & MAP_FIXED) && mmaps->len)
    {
      if (clear_mappings (addr, len, 1))
	return MAP_FAILED;
      goto map;
    }

  /* Search for the first region with a higher base address */
  if (!mmaps->len)
    goto map;
  next = find_region_after (base);
  if (next < mmaps->len - 1)
    {
      struct mmap *before = mmaps->table + next;
      ptr = before->base + before->len;
    }
  else
    ptr = base;
  for (i = next + 1; i < mmaps->len; ptr = region->base + region->len,
	 i++, next++)
    {
      region = mmaps->table + i;
      if (region->base - ptr >= len)
	break;
    }
  base = ptr;

 map:
  /* Map the region, first enabling write access so the file's contents
     can be copied over */
  if (base + len >= USER_MEM_TOP_VMA)
    RETV_ERROR (EINVAL, MAP_FAILED);
  if (prot & PROT_WRITE)
    pflags |= PAGE_FLAG_RW;
  for (ptr = base; ptr < base + len; ptr += PAGE_SIZE)
    {
      uintptr_t page = alloc_page ();
      if (UNLIKELY (!page))
	goto err0;
      memset ((void *) PHYS_REL (page), 0, PAGE_SIZE);
      if (vm_map_page (THIS_THREAD->args.pml4t, page, (void *) ptr,
		       PAGE_FLAG_RW))
	{
	  free_page (page);
	  goto err0;
	}
    }
  if (vp)
    {
      ssize_t ret = vfs_read (vp, (void *) base, bytes, offset);
      if (ret < 0)
	return MAP_FAILED;
    }

  /* Remap memory region with requested protection */
  for (cptr = base; cptr < base + len; cptr += PAGE_SIZE)
    {
      uintptr_t page = physical_addr ((void *) cptr);
      if (vm_map_page (THIS_THREAD->args.pml4t, page, (void *) cptr, pflags))
	goto err0;
      vm_clear_page ((void *) cptr);
    }

  temp = realloc (mmaps->table, sizeof (struct mmap) * ++mmaps->len);
  if (!temp)
    goto err1;
  mmaps->table = temp;
  memmove (mmaps->table + next + 1, mmaps->table + next,
	   sizeof (struct mmap) * (mmaps->len - next - 2));
  mmaps->table[next].base = base;
  mmaps->table[next].len = len;
  mmaps->table[next].file = file;
  REF_OBJECT (vp);
  mmaps->table[next].offset = offset;
  mmaps->table[next].flags = flags;
  return (void *) base;

 err1:
  mmaps->len--;
 err0:
  for (cptr = ALIGN_DOWN (base, PAGE_SIZE); cptr < ptr; cptr += PAGE_SIZE)
    free_page (physical_addr ((void *) cptr));
  return MAP_FAILED;
}

int
sys_munmap (void *addr, size_t len)
{
  if (((uintptr_t) addr & (PAGE_SIZE - 1)) || (len & (PAGE_SIZE - 1)))
    RETV_ERROR (EINVAL, -1);
  return clear_mappings (addr, len, 0);
}

int
sys_msync (void *addr, size_t len, int flags)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  struct mmap *region;
  uintptr_t base = (uintptr_t) addr;

  if (((uintptr_t) addr & (PAGE_SIZE - 1))
      || ((flags & MS_ASYNC) && (flags & MS_SYNC))
      || (flags & (MS_ASYNC | MS_SYNC | MS_INVALIDATE)) == 0)
    RETV_ERROR (EINVAL, -1);
  if (flags & MS_ASYNC)
    RETV_ERROR (ENOTSUP, -1);

  if (!mmaps->len)
    RETV_ERROR (ENOMEM, -1);
  region = mmaps->table + find_region_before (base);
  if (base >= region->base + region->len)
    RETV_ERROR (ENOMEM, -1);
  if (region->file)
    {
      if (vfs_write (region->file->vnode, addr, len,
		     region->offset + base - region->base) < 0)
        RETV_ERROR (EIO, -1);
    }
  return 0;
}
