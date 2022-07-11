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
 * Locates a memory region with a base address less than a requested address.
 *
 * @param base the address to search with
 * @return the index in the mmap table of the region, or -1 if there is no 
 * mapping satisfying the condition
 * @todo use binary search for performance
 */

static ssize_t
find_region_before (uintptr_t base)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  ssize_t i;
  for (i = 0; (size_t) i < mmaps->len; i++)
    {
      if (mmaps->table[i].base >= base)
	return i - 1;
    }
  return mmaps->len - 1;
}

/*!
 * Locates a memory region with a base address less than or equal to a 
 * requested address.
 *
 * @param base the address to search with
 * @return the index in the mmap table of the region, or -1 if there is no 
 * mapping satisfying the condition
 * @todo use binary search for performance
 */

static ssize_t
find_region_before_equal (uintptr_t base)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  ssize_t i;
  for (i = 0; (size_t) i < mmaps->len; i++)
    {
      if (mmaps->table[i].base > base)
	return i - 1;
    }
  return mmaps->len - 1;
}

/*!
 * Writes all data in mappings or parts of mappings contained in an area in
 * virtual memory to disk.
 *
 * @param addr the base address of the region to sync
 * @param len number of bytes to sync
 * @return zero on success
 */

static int
sync_mappings (void *addr, size_t len)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  struct mmap *region;
  uintptr_t ptr = (uintptr_t) addr;
  uintptr_t i;
  ssize_t ri;

  /* Find the last mapping under the requested address */
  ri = find_region_before (ptr);
  if (ri >= 0)
    {
      region = mmaps->table + ri;
      if (region->base + region->len > ptr)
	{
	  /* Write the data at the end of the base */
	  if (region->file
	      && vfs_write (region->file->vnode, (void *) ptr,
			    region->base + region->len - ptr,
			    region->offset + ptr - region->base) < 0)
	    return -1;
	}
    }

  for (i = ri + 1; i < mmaps->len; i++)
    {
      region = mmaps->table + i;
      if (ptr + len >= region->base + region->len)
	{
	  /* The region is entirely overlapped */
	  if (region->file
	      && vfs_write (region->file->vnode, (void *) region->base,
			    region->len, region->offset) < 0)
	    return -1;
	}
      else if (ptr + len > region->base)
	{
	  /* The region's start overlaps */
	  if (region->file
	      && vfs_write (region->file->vnode, (void *) region->base,
			    ptr + len - region->base, region->offset) < 0)
	    return -1;
	}
      else
	break;
    }
  return 0;
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
  ssize_t ri;

  if (sync)
    {
      if (sync_mappings (addr, len))
	return -1;
    }

  /* Find the last mapping under the requested address */
  ri = find_region_before (ptr);
  if (ri >= 0)
    {
      region = mmaps->table + ri;
      if (region->base + region->len > ptr)
	{
	  /* The end of the region overlaps, truncate it */
	  region->len = ptr - region->base;
	  for (i = region->base + region->len; i < ptr; i += PAGE_SIZE)
	    {
	      if (vm_unmap_page (THIS_THREAD->args.pml4t, (void *) i))
		return -1;
	      free_page (physical_addr ((void *) i));
	      vm_clear_page ((void *) i);
	    }
	}
    }

  for (i = ri + 1; i < mmaps->len; i++)
    {
      region = mmaps->table + i;
      if (ptr + len >= region->base + region->len)
	{
	  /* The region is entirely overlapped, remove it completely */
	  uintptr_t j;
	  for (j = region->base; j < region->base + region->len; j += PAGE_SIZE)
	    {
	      if (vm_unmap_page (THIS_THREAD->args.pml4t, (void *) j))
		return -1;
	      free_page (physical_addr ((void *) j));
	      vm_clear_page ((void *) j);
	    }
	  if (region->file)
	    free_fd (THIS_PROCESS, region->fd);
	  mmaps->len--;
	  memmove (mmaps->table + i, mmaps->table + i + 1,
		   sizeof (struct mmap) * (mmaps->len - i));
	  i--;
	}
      else if (ptr + len > region->base)
	{
	  /* The region's start overlaps, remove that portion */
	  uintptr_t j;
	  size_t diff = ptr + len - region->base;
	  for (j = region->base; j < ptr + len; j += PAGE_SIZE)
	    {
	      if (vm_unmap_page (THIS_THREAD->args.pml4t, (void *) j))
		return -1;
	      free_page (physical_addr ((void *) j));
	      vm_clear_page ((void *) j);
	    }
	  mmaps->table[i].len -= diff;
	  mmaps->table[i].base = ptr + len;
	  mmaps->table[i].offset += diff;
	}
      else
	break;
    }
  return 0;
}

/*!
 * Expands a memory mapping. This function does not check whether the
 * expanded space overlaps with mappings located afterward. The new length
 * must be greater than the current length. This function is intended to
 * be used when passing data from kernel to user space on process start.
 *
 * @param pml4t the PML4T to add the mapping to
 * @param addr an address located in the mapping to expand
 * @param len the new length of the mapping
 * @return zero on success
 */

int
expand_mmap (uintptr_t *pml4t, void *addr, size_t len)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  ssize_t ri = find_region_before_equal ((uintptr_t) addr);
  uintptr_t ptr;
  uintptr_t cptr;
  struct mmap *region;
  if (ri < 0)
    RETV_ERROR (ENOMEM, -1);
  region = mmaps->table + ri;
  len = ALIGN_UP (len, PAGE_SIZE);
  for (ptr = region->base + region->len; ptr < region->base + len;
       ptr += PAGE_SIZE)
    {
      uintptr_t page = alloc_page ();
      if (UNLIKELY (!page))
	goto err0;
      if (vm_map_page (pml4t, page, (void *) ptr,
		       PAGE_FLAG_USER | PAGE_FLAG_RW))
	{
	  free_page (page);
	  goto err0;
	}
    }
  region->len = len;
  return 0;

 err0:
  for (cptr = region->base; cptr < ptr; cptr += PAGE_SIZE)
    {
      free_page (physical_addr ((void *) cptr));
      vm_unmap_page (THIS_THREAD->args.pml4t, (void *) cptr);
      vm_clear_page ((void *) cptr);
    }
  RETV_ERROR (ENOMEM, -1);
}

void *
sys_mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  struct mmap_table *mmaps = &THIS_PROCESS->mmaps;
  struct mmap *temp;
  uintptr_t base;
  uintptr_t ptr;
  uintptr_t cptr;
  ssize_t ri;
  struct fd *file = NULL;
  struct vnode *vp = NULL;
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
  len = ALIGN_UP (len, PAGE_SIZE);
  if (addr)
    {
      base = (uintptr_t) addr;
      if (base & (PAGE_SIZE - 1))
	RETV_ERROR (EINVAL, MAP_FAILED);
    }
  else
    base = USER_MMAP_BASE_VMA;

  if (flags & MAP_FIXED)
    {
      /* Clear any existing mappings occupying the space that would be
	 overwritten by the new mapping */
      if (clear_mappings (addr, len, 1))
	return MAP_FAILED;
      ri = find_region_before (base) + 1;
    }
  else
    {
      /* Find a suitable location for the mapping, starting with the region
	 behind the requested address and moving forward until a gap in mappings
	 large enough is found */
      ri = find_region_before_equal (base);
      if (ri >= 0)
	{
	  struct mmap *region;
	  struct mmap *next = mmaps->table + ri;
	  for (; (size_t) ri < mmaps->len - 1; ri++)
	    {
	      region = mmaps->table + ri;
	      next = mmaps->table + ri + 1;
	      ptr = region->base + region->len;
	      if (next->base - ptr >= len)
		{
		  base = ptr;
		  goto found;
		}
	    }
	  base = next->base + next->len;
	}
    found:
      ri++;
    }

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

  /* Add another entry to the mmap table */
  temp = realloc (mmaps->table, sizeof (struct mmap) * ++mmaps->len);
  if (!temp)
    goto err1;
  mmaps->table = temp;
  memmove (mmaps->table + ri + 1, mmaps->table + ri,
	   sizeof (struct mmap) * (mmaps->len - ri - 1));
  mmaps->table[ri].base = base;
  mmaps->table[ri].len = len;
  mmaps->table[ri].prot = prot;
  mmaps->table[ri].file = file;
  if (file)
    file->count++;
  mmaps->table[ri].fd = fd;
  mmaps->table[ri].offset = offset;
  mmaps->table[ri].flags = flags;
  return (void *) base;

 err1:
  mmaps->len--;
 err0:
  for (cptr = ALIGN_DOWN (base, PAGE_SIZE); cptr < ptr; cptr += PAGE_SIZE)
    {
      free_page (physical_addr ((void *) cptr));
      vm_unmap_page (THIS_THREAD->args.pml4t, (void *) cptr);
      vm_clear_page ((void *) cptr);
    }
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
  if (((uintptr_t) addr & (PAGE_SIZE - 1))
      || ((flags & MS_ASYNC) && (flags & MS_SYNC))
      || (flags & (MS_ASYNC | MS_SYNC | MS_INVALIDATE)) == 0)
    RETV_ERROR (EINVAL, -1);
  if (flags & MS_ASYNC)
    RETV_ERROR (ENOTSUP, -1);
  return sync_mappings (addr, len);
}
