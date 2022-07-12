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
#include <pml/syscall.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

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
  struct vnode *vp = vnode_namei (path, follow_links ? 0 : -1);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_getattr (st, vp);
  UNREF_OBJECT (vp);
  return ret;
}

static void sync_recurse_vnode (struct vnode *vp);
static void unmark_sync_proc (struct vnode *vp);

static void
sync_children (const char *key, void *value, void *data)
{
  ino_t ino = (ino_t) value;
  struct mount *mp = data;
  struct vnode *vp = vnode_lookup_cache (mp, ino);
  if (LIKELY (vp) && !(vp->flags & VN_FLAG_SYNC_PROC))
    {
      sync_recurse_vnode (vp);
      vp->flags |= VN_FLAG_SYNC_PROC;
    }
}

static void
unmark_children (const char *key, void *value, void *data)
{
  ino_t ino = (ino_t) value;
  struct mount *mp = data;
  struct vnode *vp = vnode_lookup_cache (mp, ino);
  if (LIKELY (vp) && (vp->flags & VN_FLAG_SYNC_PROC))
    unmark_sync_proc (vp);
}

static void
sync_recurse_vnode (struct vnode *vp)
{
  vfs_sync (vp);
  vp->flags |= VN_FLAG_SYNC_PROC;
  strmap_iterate (vp->children, sync_children, vp->mount);
}

static void
unmark_sync_proc (struct vnode *vp)
{
  vp->flags &= ~VN_FLAG_SYNC_PROC;
  strmap_iterate (vp->children, unmark_children, vp->mount);
}

int
sys_open (const char *path, int flags, ...)
{
  va_list args;
  struct fd_table *fds = &THIS_PROCESS->fds;
  struct vnode *vp;
  int fd;
  int sysfd = alloc_fd ();
  if (UNLIKELY (sysfd == -1))
    RETV_ERROR (ENFILE, -1);
  fd = alloc_procfd ();
  if (fd == -1)
    return -1;
  vp = vnode_namei (path, flags & O_NOFOLLOW ? -1 : 0);
  if (!vp)
    {
      if (errno == ENOENT && (flags & O_CREAT))
	{
	  struct vnode *dir;
	  const char *name;
	  mode_t mode;
	  int ret;
	  if (vnode_dir_name (path, &dir, &name))
	    return -1;
	  if (!strcmp (name, ".") || !strcmp (name, ".."))
	    {
	      UNREF_OBJECT (dir);
	      RETV_ERROR (ENOENT, -1);
	    }
	  va_start (args, flags);
	  mode = va_arg (args, mode_t);
	  va_end (args);
	  if (flags & O_DIRECTORY)
	    ret = vfs_mkdir (&vp, dir, name, mode & ~THIS_PROCESS->umask);
	  else
	    ret = vfs_create (&vp, dir, name, mode & ~THIS_PROCESS->umask, 0);
	  UNREF_OBJECT (dir);
	  if (ret)
	    return -1;
	}
      else
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
  return fd;
}

int
sys_close (int fd)
{
  struct fd_table *fds = &THIS_PROCESS->fds;
  if (!fds->table[fd])
    RETV_ERROR (EBADF, -1);
  free_fd (THIS_PROCESS, fd);
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
  if (vfs_can_seek (file->vnode))
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
  if (vfs_can_seek (file->vnode))
    file->offset += ret;
  return ret;
}

off_t
sys_lseek (int fd, off_t offset, int whence)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  if (!vfs_can_seek (file->vnode))
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
sys_mkdir (const char *path, mode_t mode)
{
  struct vnode *dir;
  const char *name;
  int ret;
  if (vnode_dir_name (path, &dir, &name))
    return -1;
  ret = vfs_mkdir (NULL, dir, name, mode);
  UNREF_OBJECT (dir);
  return ret;
}

int
sys_rename (const char *old_path, const char *new_path)
{
  struct vnode *old_dir = NULL;
  const char *old_name;
  struct vnode *new_dir = NULL;
  const char *new_name;
  struct vnode *vp;
  int ret;

  ret = vnode_dir_name (old_path, &old_dir, &old_name);
  if (ret)
    goto end;
  ret = vnode_dir_name (new_path, &new_dir, &new_name);
  if (ret)
    goto end;

  ret = vfs_lookup (&vp, old_dir, old_name);
  if (ret)
    goto end;

  if (vp->mount != old_dir->mount)
    GOTO_ERROR (EBUSY, err0); /* Don't allow moving a mount point */
  if (old_dir->mount != new_dir->mount)
    GOTO_ERROR (EXDEV, err0); /* Don't allow renaming across filesystems */

  UNREF_OBJECT (vp);
  ret = vfs_rename (old_dir, old_name, new_dir, new_name);
  goto end;

 err0:
  UNREF_OBJECT (vp);
 end:
  UNREF_OBJECT (old_dir);
  UNREF_OBJECT (new_dir);
  return ret;
}

int
sys_link (const char *old_path, const char *new_path)
{
  struct vnode *vp = vnode_namei (old_path, 0);
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
sys_unlink (const char *path)
{
  struct vnode *dir;
  const char *name;
  int ret;
  if (vnode_dir_name (path, &dir, &name))
    return -1;
  ret = vfs_unlink (dir, name);
  UNREF_OBJECT (dir);
  return ret;
}

int
sys_symlink (const char *old_path, const char *new_path)
{
  struct vnode *dir;
  const char *name;
  int ret;
  if (vnode_dir_name (new_path, &dir, &name))
    return -1;
  ret = vfs_symlink (dir, name, old_path);
  UNREF_OBJECT (dir);
  return ret;
}

ssize_t
sys_readlink (const char *path, char *buffer, size_t len)
{
  struct vnode *vp = vnode_namei (path, -1);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_readlink (vp, buffer, len);
  UNREF_OBJECT (vp);
  return ret;
}

int
sys_truncate (const char *path, off_t len)
{
  struct vnode *vp = vnode_namei (path, 0);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_truncate (vp, len);
  UNREF_OBJECT (vp);
  return ret;
}

void
sys_sync (void)
{
  size_t i;
  for (i = 0; i < mount_count; i++)
    {
      vfs_flush (mount_table[i]);
      sync_recurse_vnode (mount_table[i]->root_vnode);
      unmark_sync_proc (mount_table[i]->root_vnode);
    }
}

int
sys_fsync (int fd)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_sync (file->vnode);
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
