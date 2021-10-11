/* inode.c -- This file is part of PML.
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

#include <pml/ext2fs.h>
#include <errno.h>

const struct vnode_ops ext2_vnode_ops = {
  .lookup = ext2_lookup,
  .read = ext2_read,
  .write = ext2_write,
  .sync = ext2_sync,
  .create = ext2_create,
  .mkdir = ext2_mkdir,
  .rename = ext2_rename,
  .link = ext2_link,
  .symlink = ext2_symlink,
  .readdir = ext2_readdir,
  .bmap = ext2_bmap,
  .fill = ext2_fill
};

int
ext2_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
ext2_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
ext2_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

void
ext2_sync (struct vnode *vp)
{
}

int
ext2_create (struct vnode **result, struct vnode *dir, const char *name,
	     mode_t mode, dev_t rdev)
{
  RETV_ERROR (ENOSYS, -1);
}

int ext2_mkdir (struct vnode **result, struct vnode *dir, const char *name,
		mode_t mode)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_rename (struct vnode *vp, struct vnode *dir, const char *name)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_link (struct vnode *dir, struct vnode *vp, const char *name)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_symlink (struct vnode *dir, const char *name, const char *target)
{
  RETV_ERROR (ENOSYS, -1);
}

off_t
ext2_readdir (struct vnode *dir, struct pml_dirent *dirent, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
ext2_readlink (struct vnode *vp, char *buffer, size_t len)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_bmap (struct vnode *vp, block_t *result, block_t block, size_t num)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_fill (struct vnode *vp)
{
  RETV_ERROR (ENOSYS, -1);
}
