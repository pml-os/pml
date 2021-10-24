/* syscall.c -- This file is part of PML.
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

/*!
 * Obtains the file structure from a file descriptor in the current process.
 * If an error occurs, @ref errno will be set to @ref EBADF.
 *
 * @param fd the file descriptor
 * @return the file structure, or NULL if the file descriptor does not
 * reference an open file
 */

static struct fd *
file_fd (int fd)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  if (fd < 0 || (size_t) fd >= fds->size)
    RETV_ERROR (EBADF, NULL);
  return fds->table[fd];
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
  if (fds->size < fds->max_size)
    {
      /* Expand the file descriptor table up to the soft limit */
      size_t new_size =
	fds->size * 2 < fds->max_size ? fds->size * 2 : fds->max_size;
      struct fd **table = realloc (fds->table, sizeof (struct fd *) * new_size);
      if (UNLIKELY (!table))
	return -1;
      fds->table = table;
      fds->size = new_size;
      if (fds->curr >= fds->size)
	RETV_ERROR (EMFILE, -1);
      fd = fds->curr++;
    }
  else
    RETV_ERROR (EMFILE, -1);

 found:
  vp = vnode_namei (path, !(flags & __O_NOFOLLOW));
  if (!vp)
    {
      if (errno == ENOENT && (flags & __O_CREAT))
	{
	  struct vnode *dir;
	  const char *name;
	  int ret;
	  if (vnode_dir_name (path, &dir, &name))
	    return -1;
	  if (!strcmp (name, ".") || !strcmp (name, ".."))
	    {
	      UNREF_OBJECT (dir);
	      RETV_ERROR (ENOENT, -1);
	    }
	  if (flags & __O_DIRECTORY)
	    ret = vfs_mkdir (&vp, dir, name, FULL_PERM & ~THIS_PROCESS->umask);
	  else
	    ret = vfs_create (&vp, dir, name,
			      FULL_PERM & ~THIS_PROCESS->umask, 0);
	  UNREF_OBJECT (dir);
	  if (ret)
	    return -1;
	}
      return -1;
    }
  else if ((flags & __O_CREAT) && (flags & __O_EXCL))
    {
      UNREF_OBJECT (vp);
      RETV_ERROR (EEXIST, -1);
    }
  system_fd_table[sysfd].vnode = vp;
  system_fd_table[sysfd].flags = flags & (__O_ACCMODE | __O_NONBLOCK);
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

ssize_t
sys_read (int fd, void *buffer, size_t len)
{
  struct fd *file = file_fd (fd);
  ssize_t ret;
  if (!file)
    return -1;
  ret = vfs_read (file->vnode, buffer, len, file->offset);
  if (ret == -1)
    return -1;
  file->offset += ret;
  return ret;
}

ssize_t
sys_write (int fd, const void *buffer, size_t len)
{
  struct fd *file = file_fd (fd);
  ssize_t ret;
  if (!file)
    return -1;
  ret = vfs_write (file->vnode, buffer, len, file->offset);
  if (ret == -1)
    return -1;
  file->offset += ret;
  return ret;
}
