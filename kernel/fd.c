/* fd.c -- This file is part of PML.
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

#include <pml/process.h>
#include <pml/syscall.h>
#include <errno.h>
#include <string.h>

/*! Index to start searching for free file descriptor */
static size_t fd_table_start;

/*! System file descriptor table. */
struct fd *system_fd_table;

/*!
 * Allocates a file descriptor from the system file descriptor table. The
 * file descriptor is not truly allocated until a vnode is associated with it,
 * so free_fd() need not be called until the file is actually used.
 *
 * @return an index into the system file descriptor table, or -1 if the table
 * is full
 */

int
alloc_fd (void)
{
  for (; fd_table_start < SYSTEM_FD_TABLE_SIZE; fd_table_start++)
    {
      if (!system_fd_table[fd_table_start].vnode)
	return fd_table_start;
    }
  return -1;
}

/*!
 * Removes a reference from a file descriptor.
 *
 * @param fd an index into the system file descriptor table
 */

void
free_fd (int fd)
{
  if (!--system_fd_table[fd].count)
    {
      UNREF_OBJECT (system_fd_table[fd].vnode);
      memset (system_fd_table + fd, 0, sizeof (struct fd));
      if ((size_t) fd < fd_table_start)
	fd_table_start = fd;
    }
}
