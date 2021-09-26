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

#include <pml/panic.h>
#include <pml/thread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define PID_BITMAP_INCREMENT    1024
#define PID_BITMAP_SIZE_LIMIT   32768

static void *pid_bitmap;        /* Bitmap of allocated PIDs */
static size_t next_pid;         /* PID to start searching from */
static size_t pid_bitmap_size;  /* Size of bitmap in bytes */

/* Initializes the PID allocator by marking PIDs 0 and 1 as used and allocating
   the PID bitmap. */

void
init_pid_allocator (void)
{
  pid_bitmap = calloc (PID_BITMAP_INCREMENT, 1);
  if (UNLIKELY (!pid_bitmap))
    panic ("Failed to allocate PID bitmap");
  pid_bitmap_size = PID_BITMAP_INCREMENT;
  set_bit (pid_bitmap, 0);
  set_bit (pid_bitmap, 1);
  next_pid = 2;
}

/* Returns a number suitable as a process or thread ID. The returned value
   is guaranteed to be unique from all other values returned from this
   function unless freed with free_pid(). */

pid_t
alloc_pid (void)
{
  void *temp;
  while (pid_bitmap_size < PID_BITMAP_SIZE_LIMIT)
    {
      for (; next_pid < pid_bitmap_size * 8; next_pid++)
	{
	  if (!test_bit (pid_bitmap, next_pid))
	    {
	      set_bit (pid_bitmap, next_pid);
	      return next_pid++;
	    }
	}

      /* No PID was found, enlarge the bitmap and try again */
      pid_bitmap_size += PID_BITMAP_INCREMENT;
      temp = realloc (pid_bitmap, pid_bitmap_size);
      if (UNLIKELY (!temp))
	return -1;
      pid_bitmap = temp;
      memset (pid_bitmap + pid_bitmap_size - PID_BITMAP_INCREMENT, 0,
	      PID_BITMAP_INCREMENT);
    }
  errno = ENOMEM;
  return -1;
}

/* Unallocates the given PID so it can be reused by other processes. */

void
free_pid (pid_t pid)
{
  if (UNLIKELY (pid < 0))
    return;
  clear_bit (pid_bitmap, pid);
  if ((size_t) pid < next_pid)
    next_pid = pid;
}
