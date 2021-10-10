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

#include <pml/ata.h>
#include <pml/devfs.h>
#include <pml/device.h>
#include <pml/memory.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

const struct mount_ops devfs_mount_ops = {
  .mount = devfs_mount,
  .unmount = devfs_unmount
};

const struct vnode_ops devfs_vnode_ops = {
  .lookup = devfs_lookup,
  .read = devfs_read,
  .write = devfs_write,
  .readdir = devfs_readdir,
  .readlink = devfs_readlink,
  .fill = devfs_fill
};

int
devfs_mount (struct mount *mp, unsigned int flags)
{
  ALLOC_OBJECT (mp->root_vnode, vfs_dealloc);
  if (UNLIKELY (!mp->root_vnode))
    return -1;
  mp->ops = &devfs_mount_ops;
  mp->root_vnode->ino = DEVFS_ROOT_INO;
  mp->root_vnode->ops = &devfs_vnode_ops;
  REF_ASSIGN (mp->root_vnode->mount, mp);
  devfs_fill (mp->root_vnode);
  return 0;
}

int
devfs_unmount (struct mount *mp, unsigned int flags)
{
  UNREF_OBJECT (mp->root_vnode);
  return 0;
}

int
devfs_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  struct device *device = strmap_lookup (device_name_map, name);
  struct vnode *vp;
  if (!device)
    RETV_ERROR (ENOENT, -1);
  ALLOC_OBJECT (vp, vfs_dealloc);
  if (UNLIKELY (!vp))
    return -1;
  *result = vp;
  return 0;
}

ssize_t
devfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  struct device *device = hashmap_lookup (device_num_map, vp->rdev);
  int block = !(vp->flags & VN_FLAG_NO_BLOCK);
  if (!device)
    RETV_ERROR (ENOENT, -1);
  if (device->type == DEVICE_TYPE_BLOCK)
    {
      struct block_device *bdev = (struct block_device *) device;
      return bdev->read (bdev, buffer, len, offset, block);
    }
  else
    {
      struct char_device *cdev = (struct char_device *) device;
      unsigned char *ptr = buffer;
      size_t i;
      for (i = 0; i < len; i++)
	{
	  unsigned char c;
	  switch (cdev->read (cdev, &c, block))
	    {
	    case 1:
	      *ptr++ = c;
	      break;
	    case 0:
	      if (!i)
		RETV_ERROR (EAGAIN, -1);
	      else
		return i;
	    default:
	      return -1;
	    }
	}
      return len;
    }
}

ssize_t
devfs_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  struct device *device = hashmap_lookup (device_num_map, vp->rdev);
  int block = !(vp->flags & VN_FLAG_NO_BLOCK);
  if (!device)
    RETV_ERROR (ENOENT, -1);
  if (device->type == DEVICE_TYPE_BLOCK)
    {
      struct block_device *bdev = (struct block_device *) device;
      return bdev->write (bdev, buffer, len, offset, block);
    }
  else
    {
      struct char_device *cdev = (struct char_device *) device;
      const unsigned char *ptr = buffer;
      size_t i;
      for (i = 0; i < len; i++)
	{
	  switch (cdev->write (cdev, *ptr++, block))
	    {
	    case 1:
	      break;
	    case 0:
	      if (!i)
		RETV_ERROR (EAGAIN, -1);
	      else
		return i;
	    default:
	      return -1;
	    }
	}
      return len;
    }
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

int
devfs_fill (struct vnode *vp)
{
  vp->uid = 0;
  vp->gid = 0;
  vp->atime.sec = vp->mtime.sec = vp->ctime.sec = real_time;
  vp->atime.nsec = vp->mtime.nsec = vp->ctime.nsec = 0;
  switch (vp->ino)
    {
    case DEVFS_ROOT_INO:
      vp->mode = DEVFS_DIR_MODE;
      vp->nlink = 2;
      vp->rdev = 0;
      vp->size = device_name_map->object_count;
      vp->blocks = 0;
      vp->blksize = PAGE_SIZE;
      break;
    default:
      if (!(vp->ino >> 32))
	{
	  /* Vnode is a device file */
	  struct device *device = hashmap_lookup (device_num_map, vp->ino);
	  if (!device)
	    RETV_ERROR (ENOENT, -1);
	  vp->nlink = 1;
	  vp->rdev = vp->ino;
	  if (device->type == DEVICE_TYPE_BLOCK)
	    {
	      struct block_device *bdev = (struct block_device *) device;
	      struct disk_device_data *data = device->data;
	      vp->mode = DEVFS_BLOCK_DEVICE_MODE;
	      vp->size = data->len;
	      vp->blocks = data->len / ATA_SECTOR_SIZE;
	      vp->blksize = bdev->block_size;
	    }
	  else
	    {
	      vp->mode = DEVFS_CHAR_DEVICE_MODE;
	      vp->size = 0;
	      vp->blocks = 0;
	      vp->blksize = PAGE_SIZE;
	    }
	}
      else
	RETV_ERROR (ENOENT, -1);
    }
  return 0;
}
