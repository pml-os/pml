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

#include <pml/fcntl.h>
#include <pml/syscall.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

/*! Index to start searching for free file descriptor */
static size_t fd_table_start;

/*! System file descriptor table. */
struct fd *system_fd_table;

static int
dupfd (int fd, int fd2)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  struct fd *file = file_fd (fd);
  size_t i;
  if (!file)
    return -1;
  for (i = fd2; i < fds->size; i++)
    {
      if (!fds->table[i])
	goto found;
    }
  if (fds->size < fds->max_size)
    {
      /* Expand the file descriptor table up to the soft limit */
      size_t new_size = next64_p2 (fd2 + 1);
      struct fd **table;
      if (new_size > fds->max_size)
	new_size = fds->max_size;
      table = realloc (fds->table, sizeof (struct fd *) * new_size);
      if (UNLIKELY (!table))
	return -1;
      memset (table + fds->size, 0,
	      sizeof (struct fd *) * (new_size - fds->size));
      fds->table = table;
      fds->size = new_size;
      i = fd2;
    }
  else
    RETV_ERROR (EMFILE, -1);

 found:
  fds->table[i] = file;
  file->count++;
  return 0;
}

/*!
 * Allocates a file descriptor in the current process's file descriptor table.
 * The file is not considered as allocated until a vnode is written to it.
 *
 * @return the file descriptor, or -1 on failure
 */

int
alloc_procfd (void)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  for (; fds->curr < fds->size; fds->curr++)
    {
      if (!fds->table[fds->curr])
	return fds->curr++;
    }
  if (fds->size < fds->max_size)
    {
      /* Expand the file descriptor table up to the soft limit */
      size_t new_size =
	fds->size * 2 < fds->max_size ? fds->size * 2 : fds->max_size;
      struct fd **table = realloc (fds->table, sizeof (struct fd *) * new_size);
      if (UNLIKELY (!table))
	return -1;
      memset (table + fds->size, 0,
	      sizeof (struct fd *) * (new_size - fds->size));
      fds->table = table;
      fds->size = new_size;
      if (fds->curr >= fds->size)
	RETV_ERROR (EMFILE, -1);
      return fds->curr++;
    }
  else
    RETV_ERROR (EMFILE, -1);
}

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
 * Removes a reference from a file descriptor, removing it entirely from
 * a process's file descriptor table if necessary.
 *
 * @param process the process owning the file descriptor
 * @param fd a file descriptor
 */

void
free_fd (struct process *process, int fd)
{
  struct fd_table *fds = &process->fds;
  fd = fds->table[fd] - system_fd_table;
  if (!--system_fd_table[fd].count)
    {
      UNREF_OBJECT (system_fd_table[fd].vnode);
      memset (system_fd_table + fd, 0, sizeof (struct fd));
      if ((size_t) fd < fd_table_start)
	fd_table_start = fd;
    }
}

/*!
 * Obtains the file structure from a file descriptor in the current process.
 * If an error occurs, @ref errno will be set to @ref EBADF.
 *
 * @param fd the file descriptor
 * @return the file structure, or NULL if the file descriptor does not
 * reference an open file
 */

struct fd *
file_fd (int fd)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  if (fd < 0 || (size_t) fd >= fds->size || !fds->table[fd])
    RETV_ERROR (EBADF, NULL);
  return fds->table[fd];
}

int
sys_fcntl (int fd, int cmd, ...)
{
  va_list args;
  struct fd *file = file_fd (fd);
  if (!file)
    RETV_ERROR (EBADF, -1);

  va_start (args, cmd);
  switch (cmd)
    {
    case F_DUPFD:
      if (dupfd (fd, va_arg (args, int)))
	goto err;
      break;
    case F_GETFD:
    case F_GETFL: /* XXX What is the difference between these? */
      va_end (args);
      return file->flags;
    case F_SETFD:
    case F_SETFL:
      file->flags =
	(file->flags & O_ACCMODE) | (va_arg (args, int) & ~O_ACCMODE);
      break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:
      GOTO_ERROR (ENOSYS, err);
    default:
      GOTO_ERROR (EINVAL, err);
    }
  va_end (args);
  return 0;

 err:
  va_end (args);
  return -1;
}
