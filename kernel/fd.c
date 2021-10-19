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

int
sys_open (const char *path, int flags, ...)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  struct vnode *vp;
  int fd;
  int sysfd = alloc_fd ();
  if (UNLIKELY (sysfd == -1))
    RETV_ERROR (ENFILE, -1);
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
    {
      /* TODO Create file if O_CREAT given */
      return -1;
    }
  system_fd_table[sysfd].vnode = vp;
  system_fd_table[sysfd].flags = flags;
  system_fd_table[sysfd].count = 1;
  fds->table[fd] = system_fd_table + sysfd;
  return 0;
}

int
sys_close (int fd)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  if (!fds->table[fd])
    RETV_ERROR (EBADF, -1);
  free_fd (fds->table[fd] - system_fd_table);
  fds->table[fd] = NULL;
  return 0;
}
