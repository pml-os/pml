/* super.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/ext2fs.h>
#include <pml/memory.h>
#include <errno.h>

const struct mount_ops ext2_mount_ops = {
  .mount = ext2_mount,
  .unmount = ext2_unmount,
  .check = ext2_check
};

/*!
 * Reads one or more consecutive blocks from an ext2 filesystem.
 *
 * @param buffer buffer to store read data
 * @param fs the filesystem instance
 * @param block the starting block number to read
 * @param num the number of blocks to read
 * @return zero on success
 */

int
ext2_read_blocks (void *buffer, struct ext2_fs *fs, block_t block, size_t num)
{
  return -!(fs->device->read (fs->device, buffer, num * fs->block_size,
			      block * fs->block_size, 1) ==
	    (ssize_t) (num * fs->block_size));
}

/*!
 * Writes one or more consecutive blocks to an ext2 filesystem.
 *
 * @param buffer buffer containing the data to write
 * @param fs the filesystem instance
 * @param block the starting block number to write
 * @param num the number of blocks to write
 * @return zero on success
 */

int
ext2_write_blocks (const void *buffer, struct ext2_fs *fs, block_t block,
		   size_t num)
{
  return -!(fs->device->write (fs->device, buffer, num * fs->block_size,
			       block * fs->block_size, 1) ==
	    (ssize_t) (num * fs->block_size));
}

/*!
 * Sets up private mount information for an ext2 filesystem.
 *
 * @param device the block device containing the filesystem
 * @param flags mount options
 * @return the filesystem instance
 */

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
  fs->dynamic = fs->super.s_rev_level >= EXT2_DYNAMIC;
  if (fs->dynamic)
    fs->inode_size = fs->super.s_inode_size;
  else
    fs->inode_size = sizeof (struct ext2_inode);
  if (fs->block_size > EXT2_MAX_BLOCK_SIZE)
    GOTO_ERROR (EUCLEAN, err0);
  fs->bmap_entries = fs->block_size / 4;

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
  fs->inode_table.buffer = alloc_virtual_page ();
  if (UNLIKELY (!fs->inode_table.buffer))
    goto err1;
  return fs;

 err1:
  free (fs->group_descs);
 err0:
  free (fs);
  return NULL;
}

/*!
 * Deallocates the private ext2 filesystem data when closing a filesystem.
 *
 * @param fs the filesystem instance
 */

void
ext2_closefs (struct ext2_fs *fs)
{
  free_virtual_page (fs->inode_table.buffer);
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
