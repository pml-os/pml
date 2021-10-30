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

#include <pml/fcntl.h>
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
  if (fd < 0 || (size_t) fd >= fds->size || !fds->table[fd])
    RETV_ERROR (EBADF, NULL);
  return fds->table[fd];
}

/*!
 * Allocates a file descriptor in the current process's file descriptor table.
 * The file is not considered as allocated until a vnode is written to it.
 *
 * @return the file descriptor, or -1 on failure
 */

static int
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
 * Unified function for stat(2) and lstat(2) with an additional argument
 * for whether to follow symbolic links.
 *
 * @param path the path to the file to stat
 * @param st buffer to store file information
 * @param follow_links whether to follow symbolic links
 * @return zero on success
 */

static int
xstat (const char *path, struct stat *st, int follow_links)
{
  struct vnode *vp = vnode_namei (path, follow_links);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_getattr (st, vp);
  UNREF_OBJECT (vp);
  return ret;
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
  fd = alloc_procfd ();
  if (fd == -1)
    return -1;
  vp = vnode_namei (path, !(flags & O_NOFOLLOW));
  if (!vp)
    {
      if (errno == ENOENT && (flags & O_CREAT))
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
	  if (flags & O_DIRECTORY)
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
  else if ((flags & O_CREAT) && (flags & O_EXCL))
    {
      UNREF_OBJECT (vp);
      RETV_ERROR (EEXIST, -1);
    }
  system_fd_table[sysfd].vnode = vp;
  system_fd_table[sysfd].flags = flags & (O_ACCMODE | O_NONBLOCK);
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

off_t
sys_lseek (int fd, off_t offset, int whence)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  if (S_ISFIFO (file->vnode->mode) || S_ISSOCK (file->vnode->mode))
    RETV_ERROR (ESPIPE, -1);
  switch (whence)
    {
    case SEEK_SET:
      break;
    case SEEK_CUR:
      offset += file->offset;
      break;
    case SEEK_END:
      offset += file->vnode->size;
      break;
    default:
      RETV_ERROR (EINVAL, -1);
    }
  if (offset < 0)
    RETV_ERROR (EINVAL, -1);
  file->offset = offset;
  return offset;
}

int
sys_stat (const char *path, struct stat *st)
{
  return xstat (path, st, 1);
}

int
sys_fstat (int fd, struct stat *st)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_getattr (st, file->vnode);
}

int
sys_lstat (const char *path, struct stat *st)
{
  return xstat (path, st, 0);
}

int
sys_link (const char *old_path, const char *new_path)
{
  struct vnode *vp = vnode_namei (old_path, 1);
  struct vnode *dir;
  const char *name;
  if (!vp)
    return -1;
  if (vnode_dir_name (new_path, &dir, &name))
    goto err0;
  if (vp->mount != dir->mount)
    GOTO_ERROR (EXDEV, err1);
  if (vfs_link (dir, vp, name))
    goto err1;
  UNREF_OBJECT (dir);
  UNREF_OBJECT (vp);
  return 0;

 err1:
  UNREF_OBJECT (dir);
 err0:
  UNREF_OBJECT (vp);
  return -1;
}

int
sys_dup (int fd)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  struct fd *file = file_fd (fd);
  int nfd;
  if (!file)
    return -1;
  nfd = alloc_procfd ();
  if (nfd == -1)
    return -1;
  fds->table[nfd] = file;
  file->count++;
  return nfd;
}
