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
#include <pml/hash.h>
#include <pml/memory.h>
#include <errno.h>

const struct mount_ops ext2_mount_ops = {
  .mount = ext2_mount,
  .unmount = ext2_unmount,
  .check = ext2_check,
  .flush = ext2_flush
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
  return -!(fs->device->read (fs->device, buffer, num * fs->blksize,
			      block * fs->blksize, 1) ==
	    (ssize_t) (num * fs->blksize));
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
  return -!(fs->device->write (fs->device, buffer, num * fs->blksize,
			       block * fs->blksize, 1) ==
	    (ssize_t) (num * fs->blksize));
}

/*!
 * Sets up private mount information for an ext2 filesystem.
 *
 * @param device the block device containing the filesystem
 * @param flags mount options
 * @return the filesystem instance
 */

static int
ext2_openfs (struct block_device *device, struct ext2_fs *fs)
{
  unsigned long first_meta_bg;
  unsigned long i;
  block_t group_block;
  block_t block;
  char *dest;
  size_t inosize;
  uint64_t ngroups;
  int group_zero_adjust = 0;
  int ret;

  /* Read and verify superblock */
  if (device->read (device, &fs->super, sizeof (struct ext2_super),
		    EXT2_SUPER_OFFSET, 1) != sizeof (struct ext2_super))
    RETV_ERROR (EIO, -1);
  if (!ext2_superblock_checksum_valid (fs)
      || fs->super.s_magic != EXT2_MAGIC
      || fs->super.s_rev_level > EXT2_DYNAMIC_REV)
    RETV_ERROR (EUCLEAN, -1);
  fs->device = device;

  /* Check for unsupported features */
  if ((fs->super.s_feature_incompat & ~EXT2_INCOMPAT_SUPPORT)
      || (!(fs->mflags & MS_RDONLY)
	  && (fs->super.s_feature_ro_compat & ~EXT2_RO_COMPAT_SUPPORT)))
    RETV_ERROR (ENOTSUP, -1);
  if (fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_JOURNAL_DEV)
    RETV_ERROR (ENOTSUP, -1); /* TODO Support ext3 journaling */

  /* Check for valid block and cluster sizes */
  if (fs->super.s_log_block_size >
      EXT2_MAX_BLOCK_LOG_SIZE - EXT2_MIN_BLOCK_LOG_SIZE)
    RETV_ERROR (EUCLEAN, -1);
  if ((fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_BIGALLOC)
      && fs->super.s_log_block_size != fs->super.s_log_cluster_size)
    RETV_ERROR (EUCLEAN, -1);
  fs->blksize = EXT2_BLOCK_SIZE (fs->super);

  /* Determine inode size */
  inosize = EXT2_INODE_SIZE (fs->super);
  if (inosize < EXT2_OLD_INODE_SIZE
      || inosize > (size_t) fs->blksize
      || !IS_P2 (inosize))
    RETV_ERROR (EUCLEAN, -1);

  if ((fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
      && fs->super.s_desc_size < EXT2_MIN_DESC_SIZE_64)
    RETV_ERROR (EUCLEAN, -1);

  fs->cluster_ratio_bits =
    fs->super.s_log_cluster_size - fs->super.s_log_block_size;
  if (fs->super.s_blocks_per_group !=
      fs->super.s_clusters_per_group << fs->cluster_ratio_bits)
    RETV_ERROR (EUCLEAN, -1);
  fs->inode_blocks_per_group =
    (fs->super.s_inodes_per_group * inosize + fs->blksize - 1) / fs->blksize;

  /* Don't read group descriptors for journal devices */
  if (fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_JOURNAL_DEV)
    {
      fs->group_desc_count = 0;
      return 0;
    }

  if (!fs->super.s_inodes_per_group)
    RETV_ERROR (EUCLEAN, -1);

  /* Initialize checksum seed */
  if (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_CSUM_SEED)
    fs->checksum_seed = fs->super.s_checksum_seed;
  else if ((fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM)
	   && (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_EA_INODE))
    fs->checksum_seed = crc32 (0xffffffff, fs->super.s_uuid, 16);

  /* Check for valid block count info */
  if (!fs->super.s_blocks_per_group
      || fs->super.s_blocks_per_group > EXT2_MAX_BLOCKS_PER_GROUP (fs->super)
      || fs->inode_blocks_per_group >= EXT2_MAX_INODES_PER_GROUP (fs->super)
      || !EXT2_DESC_PER_BLOCK (fs->super)
      || fs->super.s_first_data_block >= ext2_blocks_count (&fs->super))
    RETV_ERROR (EUCLEAN, -1);

  /* Determine number of block groups */
  ngroups = div64_ceil (ext2_blocks_count (&fs->super) -
			fs->super.s_first_data_block,
			fs->super.s_blocks_per_group);
  if (ngroups >> 32)
    RETV_ERROR (EUCLEAN, -1);
  fs->group_desc_count = ngroups;
  if (ngroups * fs->super.s_inodes_per_group != fs->super.s_inodes_count)
    RETV_ERROR (EUCLEAN, -1);
  fs->desc_blocks =
    div32_ceil (fs->group_desc_count, EXT2_DESC_PER_BLOCK (fs->super));

  /* Read block group descriptors */
  fs->group_desc = malloc (fs->desc_blocks * fs->blksize);
  if (UNLIKELY (!fs->group_desc))
    RETV_ERROR (ENOMEM, -1);

  group_block = fs->super.s_first_data_block;
  if (!group_block && fs->blksize == 1024)
    group_zero_adjust = 1;
  dest = (char *) fs->group_desc;

  if (fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_META_BG)
    {
      first_meta_bg = fs->super.s_first_meta_bg;
      if (first_meta_bg > fs->desc_blocks)
	first_meta_bg = fs->desc_blocks;
    }
  else
    first_meta_bg = fs->desc_blocks;
  if (first_meta_bg > 0)
    {
      ssize_t len = first_meta_bg * fs->blksize;
      ret =
	device->read (device, dest, len,
		      (group_block + group_zero_adjust + 1) * fs->blksize, 1);
      if (ret != len)
	{
	  free (fs->group_desc);
	  return ret;
	}
      dest += fs->blksize * first_meta_bg;
    }

  for (i = first_meta_bg; i < fs->desc_blocks; i++)
    {
      block = ext2_descriptor_block (fs, group_block, i);
      ret = ext2_read_blocks (dest, fs, block, 1);
      if (ret)
	{
	  free (fs->group_desc);
	  return ret;
	}
      dest += fs->blksize;
    }

  fs->stride = fs->super.s_raid_stride;
  if ((fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_MMP)
      && !(fs->mflags & MS_RDONLY))
    {
      ret = ext4_mmp_start (fs);
      if (ret)
	{
	  ext4_mmp_stop (fs);
	  free (fs->group_desc);
	  return ret;
	}
    }
  return 0;
}

int
ext2_mount (struct mount *mp, unsigned int flags)
{
  struct ext2_fs *fs;
  struct block_device *device;
  int ret;

  /* Determine block device */
  device = hashmap_lookup (device_num_map, mp->device);
  if (!device)
    RETV_ERROR (ENOENT, -1);
  if (device->device.type != DEVICE_TYPE_BLOCK)
    RETV_ERROR (ENOTBLK, -1);

  /* Fill filesystem data */
  fs = calloc (1, sizeof (struct ext2_fs));
  if (UNLIKELY (!fs))
    RETV_ERROR (ENOMEM, -1);
  fs->mflags = flags;
  ret = ext2_openfs (device, fs);
  if (ret)
    {
      free (fs);
      return ret;
    }

  if (!(fs->mflags & MS_RDONLY))
    {
      /* Update mount time and count */
      fs->super.s_mnt_count++;
      fs->super.s_mtime = time (NULL);
      fs->super.s_state &= ~EXT2_STATE_VALID;
      fs->flags |= EXT2_FLAG_CHANGED | EXT2_FLAG_DIRTY;
      ext2_flush_fs (fs, 0);
    }
  mp->data = fs;

  /* Allocate and fill root vnode */
  mp->root_vnode = vnode_alloc ();
  if (UNLIKELY (!mp->root_vnode))
    GOTO_ERROR (ENOMEM, err0);
  mp->ops = &ext2_mount_ops;
  mp->root_vnode->ino = EXT2_ROOT_INODE;
  mp->root_vnode->ops = &ext2_vnode_ops;
  REF_ASSIGN (mp->root_vnode->mount, mp);
  if (ext2_fill (mp->root_vnode))
    goto err1;
  return 0;

 err1:
  UNREF_OBJECT (mp->root_vnode);
 err0:
  free (fs->group_desc);
  free (fs);
  return -1;
}

int
ext2_unmount (struct mount *mp, unsigned int flags)
{
  struct ext2_fs *fs = mp->data;
  return ext2_flush_fs (fs, FLUSH_VALID);
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

void
ext2_flush (struct mount *mp)
{
  struct ext2_fs *fs = mp->data;
  ext2_flush_fs (fs, 0);
}
