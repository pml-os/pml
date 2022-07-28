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

#include <pml/fcntl.h>
#include <pml/syscall.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

static void sync_recurse_vnode (struct vnode *vp);
static void unmark_sync_proc (struct vnode *vp);

static int
xaccess (struct vnode *vp, int mode, int real)
{
  if (mode == F_OK)
    return 0; /* The file exists at this point */
  else if (mode == (mode & (R_OK | W_OK | X_OK)))
    {
      if ((mode & R_OK) && !vfs_can_read (vp, real))
	RETV_ERROR (EACCES, -1);
      if ((mode & W_OK)
	  && ((vp->mount->flags & MS_RDONLY) || !vfs_can_write (vp, real)))
	RETV_ERROR (EACCES, -1);
      if ((mode & X_OK) && !vfs_can_exec (vp, real))
	RETV_ERROR (EACCES, -1);
    }
  else
    RETV_ERROR (EINVAL, -1);
  return 0;
}

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

static int
xrename (struct vnode *old_dir, const char *old_name, struct vnode *new_dir,
	 const char *new_name)
{
  struct vnode *vp;
  int ret = vfs_lookup (&vp, old_dir, old_name);
  if (ret)
    return ret;

  if (vp->mount != old_dir->mount)
    GOTO_ERROR (EBUSY, err0); /* Don't allow moving a mount point */
  if (old_dir->mount != new_dir->mount)
    GOTO_ERROR (EXDEV, err0); /* Don't allow renaming across filesystems */

  UNREF_OBJECT (vp);
  return vfs_rename (old_dir, old_name, new_dir, new_name);

 err0:
  UNREF_OBJECT (vp);
  return -1;
}

static int
xlink (struct vnode *vp, struct vnode *dir, const char *name)
{
  struct vnode *scratch;
  int ret;
  if (vp->mount != dir->mount)
    RETV_ERROR (EXDEV, -1);

  ret = vfs_lookup (&scratch, dir, name);
  if (!ret)
    {
      UNREF_OBJECT (scratch);
      RETV_ERROR (EEXIST, -1);
    }
  else if (errno != ENOENT)
    RETV_ERROR (EEXIST, -1);

  return vfs_link (dir, vp, name);
}

static int
xchmod (const char *path, mode_t mode, int follow_links)
{
  struct vnode *vp = vnode_namei (path, follow_links ? 0 : -1);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_chmod (vp, mode);
  UNREF_OBJECT (vp);
  return ret;
}

static int
xchown (const char *path, uid_t uid, gid_t gid, int follow_links)
{
  struct vnode *vp = vnode_namei (path, follow_links ? 0 : -1);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_chown (vp, uid, gid);
  UNREF_OBJECT (vp);
  return ret;
}

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
sys_mount (const char *source, const char *target, const char *fstype,
	   unsigned long flags, const void *data)
{
  struct vnode *vp;
  struct vnode *parent;
  const char *name;
  if (vnode_dir_name (target, &parent, &name))
    return -1;
  vp = vnode_namei (source, -1);
  if (!vp)
    goto err0;
  if (vp->mount != devfs || (!S_ISBLK (vp->mode) && !S_ISCHR (vp->mode)))
    goto err1;
  if (!mount_filesystem (fstype, vp->rdev, flags, parent, name, data))
    return 0;

 err1:
  UNREF_OBJECT (vp);
 err0:
  UNREF_OBJECT (parent);
  return -1;
}

int
sys_umount (const char *target)
{
  struct vnode *vp = vnode_namei (target, -1);
  int ret;
  if (!vp)
    return -1;
  ret = unmount_filesystem (vp->mount, 0);
  UNREF_OBJECT (vp);
  return ret;
}

int
sys_statvfs (const char *path, struct statvfs *st)
{
  struct vnode *vp = vnode_namei (path, 0);
  int ret;
  if (!vp)
    return -1;
  ret = vfs_statvfs (vp->mount, st);
  UNREF_OBJECT (vp);
  return ret;
}

int
sys_fstatvfs (int fd, struct statvfs *st)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_statvfs (file->vnode->mount, st);
}

int
sys_chroot (const char *path)
{
  RETV_ERROR (ENOTSUP, -1);
}

mode_t
sys_umask (mode_t mode)
{
  mode_t old = THIS_PROCESS->umask;
  THIS_PROCESS->umask = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
  return old;
}

int
sys_open (const char *path, int flags, ...)
{
  va_list args;
  struct vnode *vp;
  int fd;
  int sysfd = alloc_fd ();
  if (UNLIKELY (sysfd == -1))
    RETV_ERROR (ENFILE, -1);
  fd = alloc_procfd ();
  if (fd == -1)
    goto err0;
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
	    goto err1;
	  if (dir->mount->flags & MS_RDONLY)
	    {
	      UNREF_OBJECT (dir);
	      GOTO_ERROR (EROFS, err1);
	    }
	  if (!strcmp (name, ".") || !strcmp (name, ".."))
	    {
	      UNREF_OBJECT (dir);
	      GOTO_ERROR (ENOENT, err1);
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
	    goto err1;
	}
      else
	goto err1;
    }
  else if ((flags & O_CREAT) && (flags & O_EXCL))
    {
      UNREF_OBJECT (vp);
      GOTO_ERROR (EEXIST, err1);
    }
  if ((vp->mount->flags & MS_RDONLY) && (((flags & O_ACCMODE) == O_WRONLY)
					 || ((flags & O_ACCMODE) == O_RDWR)))
    {
      UNREF_OBJECT (vp);
      GOTO_ERROR (EROFS, err1);
    }
  fill_fd (fd, sysfd, vp, flags);
  return fd;

 err1:
  free_procfd (fd);
 err0:
  free_fd (sysfd);
  return -1;
}

int
sys_openat (int dirfd, const char *path, int flags, ...)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  va_list args;
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  va_start (args, flags);
  ret = sys_open (path, flags, va_arg (args, mode_t));
  va_end (args);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_close (int fd)
{
  if (!file_fd (fd))
    RETV_ERROR (EBADF, -1);
  free_procfd (fd);
  return 0;
}

int
sys_access (const char *path, int mode)
{
  struct vnode *vp = vnode_namei (path, 0);
  int ret;
  if (!vp)
    return -1;
  ret = xaccess (vp, mode, 1);
  UNREF_OBJECT (vp);
  return ret;
}

int
sys_faccessat (int dirfd, const char *path, int mode, int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  struct vnode *vp;
  int real = !(flags & AT_EACCESS);
  int links = -!!(flags & AT_SYMLINK_NOFOLLOW);
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  vp = vnode_namei (path, links);
  if (!vp)
    {
      ret = -1;
      goto end;
    }

  ret = xaccess (vp, mode, real);
  UNREF_OBJECT (vp);

 end:
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
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
sys_fstatat (int dirfd, const char *path, struct stat *st, int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int links = !(flags & AT_SYMLINK_NOFOLLOW);
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = xstat (path, st, links);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_mknod (const char *path, mode_t mode, dev_t dev)
{
  struct vnode *scratch;
  struct vnode *dir = NULL;
  const char *name;
  int ret;
  if (!S_ISREG (mode)
      && !S_ISBLK (mode)
      && !S_ISCHR (mode)
      && !S_ISSOCK (mode)
      && !S_ISFIFO (mode))
    RETV_ERROR (EINVAL, -1);
  else if ((S_ISBLK (mode) || S_ISCHR (mode)) && THIS_PROCESS->euid)
    RETV_ERROR (EPERM, -1);
  if (vnode_dir_name (path, &dir, &name))
    return -1;
  ret = vfs_lookup (&scratch, dir, name);
  if (!ret)
    {
      ret = -1;
      UNREF_OBJECT (scratch);
      GOTO_ERROR (EEXIST, end);
    }
  else if (errno != ENOENT)
    GOTO_ERROR (EEXIST, end);
  ret = vfs_create (NULL, dir, name, mode, dev);

 end:
  UNREF_OBJECT (dir);
  return ret;
}

int
sys_mknodat (int dirfd, const char *path, mode_t mode, dev_t dev)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = sys_mknod (path, mode, dev);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_mkdir (const char *path, mode_t mode)
{
  struct vnode *dir = NULL;
  const char *name;
  int ret;
  if (vnode_dir_name (path, &dir, &name))
    return -1;
  ret = vfs_mkdir (NULL, dir, name, mode);
  UNREF_OBJECT (dir);
  return ret;
}

int
sys_mkdirat (int dirfd, const char *path, mode_t mode)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = sys_mkdir (path, mode);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_rmdir (const char *path)
{
  struct vnode *dir;
  const char *name;
  int ret;
  if (vnode_dir_name (path, &dir, &name))
    return -1;
  if (!S_ISDIR (dir->mode))
    {
      UNREF_OBJECT (dir);
      RETV_ERROR (ENOTDIR, -1);
    }
  ret = vfs_unlink (dir, name);
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
  int ret;

  ret = vnode_dir_name (old_path, &old_dir, &old_name);
  if (ret)
    goto end;
  ret = vnode_dir_name (new_path, &new_dir, &new_name);
  if (ret)
    goto end;

  ret = xrename (old_dir, old_name, new_dir, new_name);
 end:
  UNREF_OBJECT (old_dir);
  UNREF_OBJECT (new_dir);
  return ret;
}

int
sys_renameat (int old_dirfd, const char *old_path, int new_dirfd,
	      const char *new_path)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  struct vnode *old_dir = NULL;
  const char *old_name;
  struct vnode *new_dir = NULL;
  const char *new_name;
  int unref = 0;
  int ret;
  if (old_dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (old_dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = vnode_dir_name (old_path, &old_dir, &old_name);
  if (ret)
    goto err0;
  if (unref)
    {
      unref = 0;
      UNREF_OBJECT (THIS_PROCESS->cwd);
    }

  if (new_dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (new_dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = vnode_dir_name (new_path, &new_dir, &new_name);
  if (ret)
    goto err1;

  ret = xrename (old_dir, old_name, new_dir, new_name);
 err1:
  UNREF_OBJECT (new_dir);
 err0:
  UNREF_OBJECT (old_dir);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_link (const char *old_path, const char *new_path)
{
  struct vnode *vp = vnode_namei (old_path, 0);
  struct vnode *dir;
  const char *name;
  int ret;
  if (!vp)
    return -1;
  if (vnode_dir_name (new_path, &dir, &name))
    {
      UNREF_OBJECT (vp);
      return -1;
    }
  ret = xlink (vp, dir, name);
  UNREF_OBJECT (dir);
  UNREF_OBJECT (vp);
  return ret;
}

int
sys_linkat (int old_dirfd, const char *old_path, int new_dirfd,
	    const char *new_path, int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  struct vnode *vp;
  struct vnode *dir;
  const char *name;
  int links = !!(flags & AT_SYMLINK_FOLLOW) - 1;
  int unref = 0;
  int ret;
  if (old_dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (old_dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  vp = vnode_namei (old_path, links);
  if (!vp)
    goto err0;
  if (unref)
    {
      unref = 0;
      UNREF_OBJECT (THIS_PROCESS->cwd);
    }

  if (new_dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (new_dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = vnode_dir_name (new_path, &dir, &name);
  if (ret)
    goto err1;

  ret = xlink (vp, dir, name);
  UNREF_OBJECT (dir);
 err1:
  UNREF_OBJECT (vp);
 err0:
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
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
sys_unlinkat (int dirfd, const char *path, int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int (*func) (const char *) = flags & AT_REMOVEDIR ? sys_rmdir : sys_unlink;
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = func (path);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
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

int
sys_symlinkat (const char *old_path, int new_dirfd, const char *new_path)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int unref = 0;
  int ret;
  if (new_dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (new_dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = sys_symlink (old_path, new_path);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
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

ssize_t
sys_readlinkat (int dirfd, const char *path, char *buffer, size_t len)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int unref = 0;
  ssize_t ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = sys_readlink (path, buffer, len);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
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

int
sys_ftruncate (int fd, off_t len)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_truncate (file->vnode, len);
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
sys_futimens (int fd, const struct timespec times[2])
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_utime (file->vnode, times, times + 1);
}

int
sys_utimensat (int dirfd, const char *path, const struct timespec times[2],
	       int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  struct vnode *vp;
  int links = -!(flags & AT_SYMLINK_NOFOLLOW);
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  vp = vnode_namei (path, links);
  if (!vp)
    goto end;

  ret = vfs_utime (vp, times, times + 1);
  UNREF_OBJECT (vp);
 end:
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_chmod (const char *path, mode_t mode)
{
  return xchmod (path, mode, 1);
}

int
sys_fchmod (int fd, mode_t mode)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_chmod (file->vnode, mode);
}

int
sys_fchmodat (int dirfd, const char *path, mode_t mode, int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int links = !(flags & AT_SYMLINK_NOFOLLOW);
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = xchmod (path, mode, links);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_chown (const char *path, uid_t uid, gid_t gid)
{
  return xchown (path, uid, gid, 1);
}

int
sys_fchown (int fd, uid_t uid, gid_t gid)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  return vfs_chown (file->vnode, uid, gid);
}

int
sys_lchown (const char *path, uid_t uid, gid_t gid)
{
  return xchown (path, uid, gid, 0);
}

int
sys_fchownat (int dirfd, const char *path, uid_t uid, gid_t gid, int flags)
{
  struct vnode *cwd = THIS_PROCESS->cwd;
  int links = !(flags & AT_SYMLINK_NOFOLLOW);
  int unref = 0;
  int ret;
  if (dirfd != AT_FDCWD)
    {
      struct fd *file = file_fd (dirfd);
      if (!file)
	return -1;
      REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
      unref = 1;
    }
  ret = xchown (path, uid, gid, links);
  if (unref)
    UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = cwd;
  return ret;
}

int
sys_chdir (const char *path)
{
  struct vnode *vp = vnode_namei (path, 0);
  if (!vp)
    return -1;
  UNREF_OBJECT (THIS_PROCESS->cwd);
  THIS_PROCESS->cwd = vp;
  return 0;
}

int
sys_fchdir (int fd)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return -1;
  UNREF_OBJECT (THIS_PROCESS->cwd);
  REF_ASSIGN (THIS_PROCESS->cwd, file->vnode);
  return 0;
}

ssize_t
sys_getdents (int fd, void *dirp, size_t len)
{
  struct fd *file = file_fd (fd);
  struct dirent *ptr = dirp;
  struct dirent dirent;
  size_t written = 0;
  off_t prev = file->offset;
  if (!file)
    return -1;
  while (written < len)
    {
      size_t rec_len;
      file->offset = vfs_readdir (file->vnode, &dirent, file->offset);
      if (file->offset == -1)
	{
	  file->offset = prev;
	  return -1;
	}
      else if (!file->offset)
	break;
      rec_len =
	offsetof (struct dirent, d_name) + ALIGN_UP (dirent.d_namlen + 1, 8);
      if (written + rec_len > len)
	{
	  file->offset = prev;
	  break;
	}
      ptr->d_ino = dirent.d_ino;
      ptr->d_reclen = rec_len;
      ptr->d_namlen = dirent.d_namlen;
      ptr->d_type = dirent.d_type;
      strncpy (ptr->d_name, dirent.d_name, ptr->d_namlen);
      ptr->d_name[ptr->d_namlen] = '\0';
      ptr = (struct dirent *) ((void *) ptr + ptr->d_reclen);
      prev = file->offset;
      written += ptr->d_reclen;
    }
  return written;
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
