/* vfs.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/ext2fs.h>
#include <pml/memory.h>
#include <errno.h>

const struct mount_ops ext2_mount_ops = {
  .mount = ext2_mount,
  .unmount = ext2_unmount,
  .check = ext2_check
};

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

struct ext2_fs *
ext2_openfs (struct block_device *device, unsigned int flags)
{
  struct ext2_fs *fs = malloc (sizeof (struct ext2_fs));
  ssize_t group_desc_size;
  if (UNLIKELY (!fs))
    return NULL;
  fs->device = device;

  /* Read superblock and check magic number */
  if (device->read (device, &fs->super, sizeof (struct ext2_super),
		    EXT2_SUPER_OFFSET, 1) != sizeof (struct ext2_super))
    GOTO_ERROR (EIO, err0);
  if (fs->super.s_magic != EXT2_MAGIC)
    GOTO_ERROR (EINVAL, err0);

  /* Set block and inode sizes */
  fs->block_size = 1 << (fs->super.s_log_block_size + 10);
  if (fs->super.s_rev_level >= EXT2_DYNAMIC)
    fs->inode_size = fs->super.s_inode_size;
  else
    fs->inode_size = sizeof (struct ext2_inode);
  if (fs->block_size > EXT2_MAX_BLOCK_SIZE)
    GOTO_ERROR (EUCLEAN, err0);

  /* Allocate and read block group descriptors */
  fs->group_desc_count = ext2_group_desc_count (&fs->super);
  group_desc_size = sizeof (struct ext2_group_desc) * fs->group_desc_count;
  fs->group_descs = malloc (group_desc_size);
  if (UNLIKELY (!fs->group_descs))
    goto err0;
  if (device->read (device, fs->group_descs, group_desc_size,
		    fs->block_size * (fs->super.s_first_data_block + 1), 1) !=
      group_desc_size)
    goto err1;

  /* Allocate inode table buffer */
  fs->inode_table.buffer = (unsigned char *) alloc_page ();
  if (UNLIKELY (!fs->inode_table.buffer))
    goto err1;
  fs->inode_table.buffer = PHYS_REL (fs->inode_table.buffer);
  return fs;

 err1:
  free (fs->group_descs);
 err0:
  free (fs);
  return NULL;
}

void
ext2_closefs (struct ext2_fs *fs)
{
  free_page (physical_addr (fs->inode_table.buffer));
  free (fs->group_descs);
  free (fs);
}

int
ext2_mount (struct mount *mp, unsigned int flags)
{
  struct ext2_fs *fs;
  struct block_device *device;

  /* Determine block device */
  device = hashmap_lookup (device_num_map, mp->device);
  if (!device)
    RETV_ERROR (ENOENT, -1);
  if (device->device.type != DEVICE_TYPE_BLOCK)
    RETV_ERROR (ENOTBLK, -1);
  fs = ext2_openfs (device, flags);
  if (!fs)
    return -1;

  /* Allocate and fill root vnode */
  mp->data = fs;
  mp->root_vnode = vnode_alloc ();
  if (UNLIKELY (!mp->root_vnode))
    goto err0;
  mp->ops = &ext2_mount_ops;
  mp->root_vnode->ino = EXT2_ROOT_INO;
  mp->root_vnode->ops = &ext2_vnode_ops;
  REF_ASSIGN (mp->root_vnode->mount, mp);
  if (ext2_fill (mp->root_vnode))
    goto err1;
  return 0;

 err1:
  UNREF_OBJECT (mp->root_vnode);
 err0:
  ext2_closefs (fs);
  return -1;
}

int
ext2_unmount (struct mount *mp, unsigned int flags)
{
  struct ext2_fs *fs = mp->data;
  UNREF_OBJECT (mp->root_vnode);
  ext2_closefs (fs);
  return 0;
}

int
ext2_check (struct vnode *vp)
{
  uint16_t magic;
  if (vfs_read (vp, &magic, 2, EXT2_SUPER_OFFSET +
		offsetof (struct ext2_super, s_magic)) != 2)
    return 0;
  return magic == EXT2_MAGIC;
}

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
