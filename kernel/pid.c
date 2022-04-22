/* pid.c -- This file is part of PML.
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

#include <pml/map.h>
#include <pml/panic.h>
#include <errno.h>
#include <string.h>

#define PID_BITMAP_INCREMENT    1024
#define PID_BITMAP_SIZE_LIMIT   32768

static void *pid_bitmap;        /* Bitmap of allocated PIDs */
static size_t next_pid;         /* PID to start searching from */
static size_t pid_bitmap_size;  /* Size of bitmap in bytes */
static lock_t pid_bitmap_lock;
static struct hashmap *pid_hashmap; /* Map of PIDs to process structures */

/*!
 * Initializes the PID allocator by marking PID 0 as used and allocating
 * the PID bitmap.
 */

void
init_pid_allocator (void)
{
  pid_bitmap = calloc (PID_BITMAP_INCREMENT, 1);
  if (UNLIKELY (!pid_bitmap))
    panic ("Failed to allocate PID bitmap");
  pid_bitmap_size = PID_BITMAP_INCREMENT;
  set_bit (pid_bitmap, 0);
  next_pid = 1;

  pid_hashmap = hashmap_create ();
  if (UNLIKELY (!pid_hashmap))
    panic ("Failed to create PID hashmap\n");
  map_pid_process (0, process_queue.queue[0]);
}

/*!
 * Returns a number suitable as a process or thread ID. The returned value
 * is guaranteed to be unique from all other values returned from this
 * function unless freed with free_pid().
 *
 * @return an unused ID, or -1 if no ID could be allocated
 */

pid_t
alloc_pid (void)
{
  void *temp;
  spinlock_acquire (&pid_bitmap_lock);
  while (pid_bitmap_size < PID_BITMAP_SIZE_LIMIT)
    {
      for (; next_pid < pid_bitmap_size * 8; next_pid++)
	{
	  if (!test_bit (pid_bitmap, next_pid))
	    {
	      pid_t pid = next_pid++;
	      set_bit (pid_bitmap, pid);
	      spinlock_release (&pid_bitmap_lock);
	      return pid;
	    }
	}

      /* No PID was found, enlarge the bitmap and try again */
      pid_bitmap_size += PID_BITMAP_INCREMENT;
      temp = realloc (pid_bitmap, pid_bitmap_size);
      if (UNLIKELY (!temp))
	{
	  spinlock_release (&pid_bitmap_lock);
	  return -1;
	}
      pid_bitmap = temp;
      memset (pid_bitmap + pid_bitmap_size - PID_BITMAP_INCREMENT, 0,
	      PID_BITMAP_INCREMENT);
    }
  spinlock_release (&pid_bitmap_lock);
  RETV_ERROR (ENOMEM, -1);
}

/*!
 * Unallocates a process or thread ID so it can be reused by other processes.
 *
 * @param pid the ID to unallocate
 */

void
free_pid (pid_t pid)
{
  if (UNLIKELY (pid < 0))
    return;
  spinlock_acquire (&pid_bitmap_lock);
  clear_bit (pid_bitmap, pid);
  if ((size_t) pid < next_pid)
    next_pid = pid;
  spinlock_release (&pid_bitmap_lock);
}

/*!
 * Inserts a mapping into the PID hashmap. This function panics on failure.
 *
 * @param pid the process ID
 * @param process the process structure corresponding to the PID
 */

void
map_pid_process (pid_t pid, struct process *process)
{
  hash_t key = siphash (&pid, sizeof (pid_t), 0);
  if (hashmap_insert (pid_hashmap, key, process))
    panic ("Failed to add into PID hashmap\n");
}

/*!
 * Removes a mapping from the PID hashmap, if one exists.
 *
 * @param pid the process ID
 */

void
unmap_pid (pid_t pid)
{
  hash_t key = siphash (&pid, sizeof (pid_t), 0);
  hashmap_remove (pid_hashmap, key);
}

struct process *
lookup_pid (pid_t pid)
{
  hash_t key = siphash (&pid, sizeof (pid_t), 0);
  return hashmap_lookup (pid_hashmap, key);
}
