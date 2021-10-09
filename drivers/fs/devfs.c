/* devfs.c -- This file is part of PML.
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

#include <pml/devfs.h>
#include <errno.h>

struct vnode_ops devfs_vnode_ops = {
  .lookup = devfs_lookup,
  .getattr = devfs_getattr,
  .read = devfs_read,
  .write = devfs_write,
  .readdir = devfs_readdir,
  .readlink = devfs_readlink
};

int
devfs_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  RETV_ERROR (ENOSYS, -1);
}

int
devfs_getattr (struct pml_stat *stat, struct vnode *vp)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
devfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
devfs_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

off_t
devfs_readdir (struct vnode *dir, struct pml_dirent *dirent, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
devfs_readlink (struct vnode *vp, char *buffer, size_t len)
{
  RETV_ERROR (ENOSYS, -1);
}
