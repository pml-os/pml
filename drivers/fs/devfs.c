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
#include <stdio.h>
#include <string.h>

const struct mount_ops devfs_mount_ops = {
  .mount = devfs_mount,
  .unmount = devfs_unmount,
  .check = devfs_check
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
  mp->root_vnode = vnode_alloc ();
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
devfs_check (struct vnode *vp)
{
  return 0;
}

int
devfs_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  struct vnode *vp = vnode_alloc ();
  if (UNLIKELY (!vp))
    return -1;
  if (!strcmp (name, "fd"))
    vp->ino = DEVFS_FD_INO;
  else
    {
      struct device *device = strmap_lookup (device_name_map, name);
      if (!device)
	RETV_ERROR (ENOENT, -1);
      vp->ino = makedev (device->major, device->minor);
    }
  vp->ops = &devfs_vnode_ops;
  if (vfs_fill (vp))
    {
      UNREF_OBJECT (vp);
      return -1;
    }
  *result = vp;
  return 0;
}

ssize_t
devfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  static lock_t lock;
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
      spinlock_acquire (&lock);
      for (i = 0; i < len; i++)
	{
	  unsigned char c;
	  switch (cdev->read (cdev, &c, block))
	    {
	    case 1:
	      *ptr++ = c;
	      break;
	    case 2:
	      *ptr++ = c;
	      spinlock_release (&lock);
	      return ++i;
	    case 0:
	      spinlock_release (&lock);
	      if (!i)
		RETV_ERROR (EAGAIN, -1);
	      else
		return i;
	    default:
	      spinlock_release (&lock);
	      return -1;
	    }
	}
      spinlock_release (&lock);
      return len;
    }
}

ssize_t
devfs_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  static lock_t lock;
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
      spinlock_acquire (&lock);
      for (i = 0; i < len; i++)
	{
	  switch (cdev->write (cdev, *ptr++, block))
	    {
	    case 1:
	      break;
	    case 0:
	      spinlock_release (&lock);
	      if (!i)
		RETV_ERROR (EAGAIN, -1);
	      else
		return i;
	    default:
	      spinlock_release (&lock);
	      return -1;
	    }
	}
      spinlock_release (&lock);
      return len;
    }
}

off_t
devfs_readdir (struct vnode *dir, struct dirent *dirent, off_t offset)
{
  switch (dir->ino)
    {
    case DEVFS_ROOT_INO:
      {
	struct device *device;
	struct hashmap_entry *temp;
	unsigned long key;
	size_t index;
	size_t i;
	switch (offset)
	  {
	  case DEVFS_SPECIAL_INO:
	    dirent->d_ino = DEVFS_FD_INO;
	    dirent->d_type = DT_DIR;
	    dirent->d_namlen = 2;
	    strcpy (dirent->d_name, "fd");
	    return DEVFS_SPECIAL_INO + 1;
	  case DEVFS_SPECIAL_INO + 1:
	    return 0;
	  case 0:
	    for (i = 0; i < device_num_map->bucket_count; i++)
	      {
		if (device_num_map->buckets[i])
		  {
		    offset = device_num_map->buckets[i]->key;
		    break;
		  }
	      }
	    if (!offset)
	      return DEVFS_SPECIAL_INO;
	    __fallthrough;
	  default:
	    device = hashmap_lookup (device_num_map, offset);
	    if (!device)
	      RETV_ERROR (EINVAL, -1);
	    dirent->d_ino = offset;
	    if (device->type == DEVICE_TYPE_BLOCK)
	      dirent->d_type = DT_BLK;
	    else
	      dirent->d_type = DT_CHR;
	    dirent->d_namlen = strlen (device->name);
	    strncpy (dirent->d_name, device->name, dirent->d_namlen);
	    dirent->d_name[dirent->d_namlen] = '\0';

	    key = offset;
	    index = siphash (&key, sizeof (unsigned long), 0) %
	      device_num_map->bucket_count;
	    for (temp = device_num_map->buckets[index]; temp != NULL;
		 temp = temp->next)
	      {
		if (temp->key == key)
		  break;
	      }
	    if (temp->next)
	      return temp->next->key;
	    for (index++; index < device_num_map->bucket_count; index++)
	      {
		if (device_num_map->buckets[index])
		  return device_num_map->buckets[index]->key;
	      }
	  }
	break;
      }
    case DEVFS_FD_INO:
      {
	struct fd_table *fds = &THIS_PROCESS->fds;
	size_t i;
	if (!offset)
	  {
	    for (; (size_t) offset < fds->size; offset++)
	      {
		if (fds->table[offset])
		  break;
	      }
	    if ((size_t) offset >= fds->size)
	      break;
	  }
	dirent->d_ino = fds->table[offset]->vnode->ino;
	dirent->d_ino = offset;
	dirent->d_type = IFTODT (fds->table[offset]->vnode->mode);
	sprintf (dirent->d_name, "%d", offset);
	dirent->d_namlen = strlen (dirent->d_name);
	for (i = offset + 1; i < fds->size; i++)
	  {
	    if (fds->table[i])
	      return i;
	  }
	break;
      }
    default:
      RETV_ERROR (ENOENT, -1);
    }
  return 0;
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
  vp->atime.tv_sec = vp->mtime.tv_sec = vp->ctime.tv_sec = real_time;
  vp->atime.tv_nsec = vp->mtime.tv_nsec = vp->ctime.tv_nsec = 0;
  switch (vp->ino)
    {
    case DEVFS_ROOT_INO:
      vp->mode = DEVFS_DIR_MODE;
      vp->nlink = 2;
      vp->rdev = 0;
      vp->size = 0;
      vp->blocks = 0;
      vp->blksize = PAGE_SIZE;
      break;
    case DEVFS_FD_INO:
      vp->mode = DEVFS_DIR_MODE;
      vp->nlink = 3;
      vp->rdev = 0;
      vp->size = 0;
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
