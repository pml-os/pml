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
 * Frees a file descriptor. A reference is released from its corresponding
 * vnode.
 *
 * @param fd an index into the system file descriptor table
 */

void
free_fd (int fd)
{
  UNREF_OBJECT (system_fd_table[fd].vnode);
  memset (system_fd_table + fd, 0, sizeof (struct fd));
  if ((size_t) fd < fd_table_start)
    fd_table_start = fd;
}

int
sys_open (const char *path, int flag, ...)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  struct vnode *vp;
  int fd;
  for (; fds->curr < fds->size; fds->curr++)
    {
      if (!fds->table[fds->curr])
	{
	  fd = fds->curr++;
	  goto found;
	}
    }
  RETV_ERROR (EMFILE, -1);

 found:
  vp = vnode_namei (path);
  if (!vp)
    return -1;
  return 0;
}
