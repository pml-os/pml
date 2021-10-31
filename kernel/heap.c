/* heap.c -- This file is part of PML.
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
#include <pml/lock.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lock_t kh_lock;
static uintptr_t kh_base_addr;
static uintptr_t kh_end_addr;

/*!
 * Initializes the kernel heap.
 *
 * @param base the base address of the heap
 * @param size the reserved size of the heap, in bytes
 */

void
kh_init (uintptr_t base, size_t size)
{
  struct kh_header *header;
  struct kh_tail *tail;

  kh_base_addr = base;
  header = (struct kh_header *) base;
  header->magic = KH_HEADER_MAGIC;
  header->flags = 0;
  header->size = size - sizeof (struct kh_header) - sizeof (struct kh_tail);

  kh_end_addr = base + size;
  tail = (struct kh_tail *) kh_end_addr - 1;
  tail->magic = KH_TAIL_MAGIC;
  tail->reserved = 0;
  tail->header = header;
}

/*!
 * Allocates a block of memory on the kernel heap.
 *
 * @param size the minimum size of the block
 * @param align the required alignment of the returned pointer
 * @return a pointer to the new block, or NULL if the allocation failed
 */

void *
kh_alloc_aligned (size_t size, size_t align)
{
  struct kh_header *header = (struct kh_header *) kh_base_addr;
  struct kh_tail *tail;
  void *block;

  /* Check that the requested alignment is a power of two */
  if (UNLIKELY (!IS_P2 (align)))
    RETV_ERROR (EINVAL, NULL);

  /* Align the requested size to the default alignment so all memory
     accesses are aligned */
  size = ALIGN_UP (size, KH_DEFAULT_ALIGN);

  spinlock_acquire (&kh_lock);
  while (1)
    {
      if (UNLIKELY (header >= (struct kh_header *) kh_end_addr))
	{
	  /* Reached the end of the heap and no suitable block was found */
	  spinlock_release (&kh_lock);
	  RETV_ERROR (ENOMEM, NULL);
	}
      if (UNLIKELY (header->magic != KH_HEADER_MAGIC))
	{
	  spinlock_release (&kh_lock);
	  debug_printf ("bad magic number in header block\n");
	  RETV_ERROR (EUCLEAN, NULL);
	}

      /* Check that the tail block is valid */
      tail = (struct kh_tail *) ((uintptr_t) header + header->size +
				 sizeof (struct kh_header));
      if (UNLIKELY (tail->magic != KH_TAIL_MAGIC || tail->header != header))
	{
	  spinlock_release (&kh_lock);
	  debug_printf ("invalid tail block for header block\n");
	  RETV_ERROR (EUCLEAN, NULL);
	}

      /* Check if the header is free */
      block = (void *) ((uintptr_t) header + sizeof (struct kh_header));
      if (!(header->flags & KH_FLAG_ALLOC))
	{
	  /* Align the pointer to the requested alignment and check if
	     the block is large enough to fit the requested size */
	  block = ALIGN_UP (block, align);
	  if (block < (void *) tail
	      && (uintptr_t) tail - (uintptr_t) block >= size)
	    {
	      struct kh_header *aligned_header = (struct kh_header *) block - 1;
	      size_t diff = (uintptr_t) aligned_header - (uintptr_t) header;
	      if (diff > 0 && diff < sizeof (struct kh_header) +
		  sizeof (struct kh_tail) && header == (void *) kh_base_addr)
		{
		  /* No space to fit an empty block, move to the next
		     possible pointer */
		  aligned_header =
		    (struct kh_header *) ((uintptr_t) aligned_header + align);
		  diff += align;
		}
	      memcpy (aligned_header, header, sizeof (struct kh_header));
	      aligned_header->size -= diff;
	      if (diff > KH_MIN_BLOCK_SPLIT_SIZE + sizeof (struct kh_header) +
		  sizeof (struct kh_tail))
		{
		  /* Add a new free block in between the old header and
		     the new aligned header */
		  struct kh_tail *prev_tail;
		  header->size = diff - sizeof (struct kh_header) -
		    sizeof (struct kh_tail);
		  prev_tail = (struct kh_tail *) aligned_header - 1;
		  prev_tail->magic = KH_TAIL_MAGIC;
		  prev_tail->reserved = 0;
		  prev_tail->header = header;
		}
	      else if (diff > 0)
		{
		  struct kh_tail *prev_tail = (struct kh_tail *) header - 1;
		  if (prev_tail < (struct kh_tail *) kh_base_addr)
		    {
		      /* This is the first block of the heap, we must make
			 a new block even if its size is zero since we cannot
			 change the starting location of the heap */
		      struct kh_header *first_header =
			(struct kh_header *) kh_base_addr;
		      first_header->magic = KH_HEADER_MAGIC;
		      first_header->flags = 0;
		      first_header->size = diff - sizeof (struct kh_header) -
			sizeof (struct kh_tail);

		      prev_tail =
			(struct kh_tail *) ((uintptr_t) kh_base_addr +
					    sizeof (struct kh_header) +
					    first_header->size);
		      prev_tail->magic = KH_TAIL_MAGIC;
		      prev_tail->reserved = 0;
		      prev_tail->header = first_header;
		    }
		  else
		    {
		      /* Increase the size of the previous block up to
			 the new aligned block */
		      struct kh_tail *new_tail;
		      prev_tail->header->size += diff;
		      new_tail = (struct kh_tail *) aligned_header - 1;
		      memcpy (new_tail, prev_tail, sizeof (struct kh_tail));
		    }
		  tail->header = aligned_header;
		}

	      /* We found a suitable header, stop searching */
	      header = aligned_header;
	      break;
	    }
	}

      /* Move to the next header */
      header = (struct kh_header *) (tail + 1);
    }

  /* If the header is large enough that we can create another header using
     the free space, shrink the header and insert one after to fill the
     extra space */
  if (header->size >= size + sizeof (struct kh_header) +
      sizeof (struct kh_tail) + KH_MIN_BLOCK_SPLIT_SIZE)
    {
      struct kh_header *new_header;
      struct kh_tail *new_tail;

      /* Create a new tail to match the current header */
      new_tail = (struct kh_tail *) ((uintptr_t) header +
				     sizeof (struct kh_header) + size);
      new_tail->magic = KH_TAIL_MAGIC;
      new_tail->reserved = 0;
      new_tail->header = header;

      /* Create a new header for the remaining space in the old block */
      new_header = (struct kh_header *) (new_tail + 1);
      new_header->magic = KH_HEADER_MAGIC;
      new_header->flags = header->flags;
      new_header->size = header->size - size - sizeof (struct kh_header) -
	sizeof (struct kh_tail);
      tail->header = new_header;

      /* Shrink the header to the requested size */
      header->size = size;
    }

  /* Mark the header as allocated and return the pointer to its data */
  header->flags |= KH_FLAG_ALLOC;
  spinlock_release (&kh_lock);
  return block;
}

/*!
 * Changes the size of a memory block. If more memory is requested, the
 * returned pointer may be another memory block with the same contents as the
 * old one but at a different address.
 *
 * @param ptr the pointer to reallocate
 * @param size the new size of the block
 * @return a pointer to the new block, or NULL if the allocation failed
 */

void *
kh_realloc (void *ptr, size_t size)
{
  struct kh_header *header = (struct kh_header *) ptr - 1;
  if (!ptr)
    return kh_alloc_aligned (size, KH_DEFAULT_ALIGN);
  /* Align the requested size to the default alignment so all memory
     accesses are aligned */
  size = ALIGN_UP (size, KH_DEFAULT_ALIGN);

  spinlock_acquire (&kh_lock);
  if (UNLIKELY (header->magic != KH_HEADER_MAGIC))
    {
      spinlock_release (&kh_lock);
      debug_printf ("invalid pointer");
      RETV_ERROR (EFAULT, NULL);
    }
  if (!(header->flags & KH_FLAG_ALLOC))
    {
      spinlock_release (&kh_lock);
      return kh_alloc_aligned (size, KH_DEFAULT_ALIGN);
    }

  if (size > header->size)
    {
      /* We are requesting more memory than available in the current block.
	 If the next block is free and unifying it with the current block
	 would make it large enough, do it, otherwise allocate a new block
	 and copy the data over. */
      struct kh_header *next_header =
	(struct kh_header *) ((uintptr_t) header + sizeof (struct kh_header) +
			      sizeof (struct kh_tail) + header->size);
      if (next_header < (struct kh_header *) kh_end_addr
	  && !(next_header->flags & KH_FLAG_ALLOC) && header->size +
	  sizeof (struct kh_header) + sizeof (struct kh_tail) +
	  next_header->size >= size)
	{
	  struct kh_tail *next_tail =
	    (struct kh_tail *) ((uintptr_t) next_header +
				sizeof (struct kh_header) + next_header->size);
	  size_t extra = sizeof (struct kh_header) + sizeof (struct kh_tail) +
	    next_header->size;
	  size_t needed = size - header->size;
	  if (extra - needed >= sizeof (struct kh_header) +
	      sizeof (struct kh_tail) + KH_MIN_BLOCK_SPLIT_SIZE)
	    {
	      /* Create a new block to hold the remaining space instead of
		 using it all for the allocated block */
	      struct kh_header *new_header;
	      struct kh_tail *new_tail;
	      size_t next_size = next_header->size;

	      new_tail = (struct kh_tail *) ((uintptr_t) header +
					     sizeof (struct kh_header) + size);
	      new_tail->magic = KH_TAIL_MAGIC;
	      new_tail->reserved = 0;
	      new_tail->header = header;

	      new_header = (struct kh_header *) (new_tail + 1);
	      new_header->magic = KH_HEADER_MAGIC;
	      new_header->flags = header->flags & ~KH_FLAG_ALLOC;
	      new_header->size = header->size + next_size - size;
	      header->size = size;
	      next_tail->header = new_header;
	    }
	  else
	    {
	      next_tail->header = header;
	      header->size += extra;
	    }
	}
      else
	{
	  void *new_ptr;
	  spinlock_release (&kh_lock);
	  new_ptr = kh_alloc_aligned (size, KH_DEFAULT_ALIGN);
	  if (UNLIKELY (!new_ptr))
	    return NULL;
	  memcpy (new_ptr, ptr, header->size);
	  kh_free (ptr);
	  return new_ptr;
	}
    }
  /* We are requesting less memory than the block contains. If the
     freed memory is large enough to make a new free block, make one. */
  else if (header->size - size >= sizeof (struct kh_header) +
	   sizeof (struct kh_tail) + KH_MIN_BLOCK_SPLIT_SIZE)
    {
      struct kh_header *next_header;
      struct kh_tail *tail;
      struct kh_tail *next_tail;

      tail = (struct kh_tail *) ((uintptr_t) header +
				 sizeof (struct kh_header) + size);
      tail->magic = KH_TAIL_MAGIC;
      tail->reserved = 0;
      tail->header = header;

      next_header = (struct kh_header *) (tail + 1);
      next_header->magic = KH_HEADER_MAGIC;
      next_header->flags = header->flags & ~KH_FLAG_ALLOC;
      next_header->size = header->size - size - sizeof (struct kh_header) -
	sizeof (struct kh_tail);

      next_tail = (struct kh_tail *) ((uintptr_t) header +
				      sizeof (struct kh_header) + header->size);
      next_tail->header = next_header;
      header->size = size;
    }
  spinlock_release (&kh_lock);
  return ptr;
}

/*!
 * Unallocates the memory used by a memory block. If a null pointer is given,
 * no action is performed.
 *
 * @param ptr the pointer to free
 */

void
kh_free (void *ptr)
{
  struct kh_header *header;
  struct kh_header *next_header;
  struct kh_tail *tail;
  struct kh_tail *next_tail;
  if (!ptr)
    return;

  /* Validate the pointer's header and mark it as free */
  spinlock_acquire (&kh_lock);
  header = (struct kh_header *) ptr - 1;
  if (UNLIKELY (header->magic != KH_HEADER_MAGIC))
    {
      spinlock_release (&kh_lock);
      debug_printf ("invalid pointer");
      RET_ERROR (EFAULT);
    }
  header->flags &= ~KH_FLAG_ALLOC;

  /* Unify this block with a preceding free block */
  tail = (struct kh_tail *) header - 1;
  if (tail >= (struct kh_tail *) kh_base_addr
      && header->flags == tail->header->flags)
    {
      tail->header->size += header->size + sizeof (struct kh_header) +
	sizeof (struct kh_tail);
      next_tail = (struct kh_tail *) ((uintptr_t) header +
				      sizeof (struct kh_header) + header->size);
      next_tail->header = tail->header;
      header = tail->header;
    }

  /* Unify this block with a following free block */
  next_header =
    (struct kh_header *) ((uintptr_t) header + sizeof (struct kh_header) +
			  sizeof (struct kh_tail) + header->size);
  if (next_header < (struct kh_header *) kh_end_addr
      && header->flags == next_header->flags)
    {
      header->size += sizeof (struct kh_header) + sizeof (struct kh_tail) +
	next_header->size;
      next_tail = (struct kh_tail *) ((uintptr_t) next_header +
				      sizeof (struct kh_header) +
				      next_header->size);
      next_tail->header = header;
    }
  spinlock_release (&kh_lock);
}
