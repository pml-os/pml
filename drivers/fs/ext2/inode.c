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

#include <pml/alloc.h>
#include <pml/ext2fs.h>
#include <errno.h>
#include <string.h>

const struct vnode_ops ext2_vnode_ops = {
  .lookup = ext2_lookup,
  .read = ext2_read,
  .write = ext2_write,
  .sync = ext2_sync,
  .create = ext2_create,
  .mkdir = ext2_mkdir,
  .rename = ext2_rename,
  .link = ext2_link,
  .unlink = ext2_unlink,
  .symlink = ext2_symlink,
  .readlink = ext2_readlink,
  .readdir = ext2_readdir,
  .fill = ext2_fill,
  .dealloc = ext2_dealloc
};

/*!
 * Reads the on-disk inode structure from an ext2 filesystem.
 *
 * @param inode pointer to store inode data in
 * @param ino the inode number to read
 * @param fs the filesystem instance
 * @return zero on success
 */

int
ext2_read_inode (struct ext2_inode *inode, ino_t ino, struct ext2_fs *fs)
{
  ext2_bgrp_t group = ext2_inode_group_desc (ino, &fs->super);
  size_t index = (ino - 1) % fs->super.s_inodes_per_group;
  size_t curr = index * fs->inode_size / fs->block_size;
  block_t block = fs->group_descs[group].bg_inode_table + curr;
  index %= fs->block_size / fs->inode_size;
  if (block != fs->inode_table.block)
    {
      if (ext2_read_blocks (fs->inode_table.buffer, fs, block, 1))
	return -1;
      fs->inode_table.block = block;
    }
  memcpy (inode, fs->inode_table.buffer + index * fs->inode_size,
	  sizeof (struct ext2_inode));
  fs->inode_table.group = group;
  fs->inode_table.block = block;
  fs->inode_table.curr = curr;
  return 0;
}

/*!
 * Writes to an inode's on-disk inode structure.
 *
 * @param inode pointer containing inode data
 * @param ino the inode number to write
 * @param fs the filesystem instance
 * @return zero on success
 */

int
ext2_write_inode (const struct ext2_inode *inode, ino_t ino, struct ext2_fs *fs)
{
  ext2_bgrp_t group = ext2_inode_group_desc (ino, &fs->super);
  size_t index = (ino - 1) % fs->super.s_inodes_per_group;
  size_t curr = index * fs->inode_size / fs->block_size;
  block_t block = fs->group_descs[group].bg_inode_table + curr;
  index %= fs->block_size / fs->inode_size;
  if (block != fs->inode_table.block)
    {
      if (ext2_read_blocks (fs->inode_table.buffer, fs, block, 1))
	return -1;
      fs->inode_table.block = block;
    }
  memcpy (fs->inode_table.buffer + index * fs->inode_size, inode,
	  sizeof (struct ext2_inode));
  fs->inode_table.group = group;
  fs->inode_table.block = block;
  fs->inode_table.curr = curr;
  if (ext2_write_blocks (fs->inode_table.buffer, fs, fs->inode_table.block, 1))
    return -1;
  return 0;
}

/*!
 * Allocates a page-sized buffer for storing blocks of the filesystem while
 * performing I/O on unaligned offsets in inodes. If a buffer is already
 * allocated, no action is performed.
 *
 * @file the file instance
 * @return zero on success
 */

int
ext2_alloc_io_buffer (struct ext2_file *file)
{
  if (file->io_buffer)
    return 0;
  file->io_buffer = alloc_virtual_page ();
  if (UNLIKELY (!file->io_buffer))
    return -1;
  return 0;
}

/*!
 * Reads a block of an inode into its I/O buffer.
 *
 * @param vp the vnode
 * @param block the block number
 * @return zero on success
 */

int
ext2_read_io_buffer_block (struct vnode *vp, block_t block)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  block_t fs_block;
  if (ext2_alloc_io_buffer (file))
    RETV_ERROR (EIO, -1);
  if (ext2_read_bmap (vp, &fs_block, block, 1))
    RETV_ERROR (EIO, -1);
  if (fs_block != file->io_block)
    {
      file->io_block = fs_block;
      if (ext2_read_blocks (file->io_buffer, fs, fs_block, 1))
	RETV_ERROR (EIO, -1);
    }
  return 0;
}

/*!
 * Flushes the contents of a file's I/O buffer to disk.
 *
 * @param vp the vnode
 * @return zero on success
 */

int
ext2_flush_io_buffer_block (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  block_t fs_block;
  if (!file->io_buffer)
    return 0;
  if (ext2_write_blocks (file->io_buffer, fs, file->io_block, 1))
    RETV_ERROR (EIO, -1);
  return 0;
}

/*!
 * Reads a block of an inode into its indirect block buffer.
 *
 * @param vp the vnode
 * @param block the block number
 * @return zero on success
 */

int
ext2_read_ind_bmap (struct vnode *vp, block_t block)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  if (!file->ind_bmap)
    {
      file->ind_bmap = alloc_virtual_page ();
      if (UNLIKELY (!file->ind_bmap))
	return -1;
    }
  if (file->ind_block != block)
    {
      if (file->ind_block
	  && ext2_write_blocks (file->ind_bmap, fs, file->ind_block, 1))
	RETV_ERROR (EIO, -1);
      file->ind_block = block;
      if (ext2_read_blocks (file->ind_bmap, fs, block, 1))
	RETV_ERROR (EIO, -1);
    }
  return 0;
}

/*!
 * Reads a block of an inode into its doubly indirect block buffer.
 *
 * @param vp the vnode
 * @param block the block number
 * @return zero on success
 */

int
ext2_read_dind_bmap (struct vnode *vp, block_t block)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  if (!file->dind_bmap)
    {
      file->dind_bmap = alloc_virtual_page ();
      if (UNLIKELY (!file->dind_bmap))
	return -1;
    }
  if (file->dind_block != block)
    {
      if (file->dind_block
	  && ext2_write_blocks (file->dind_bmap, fs, file->dind_block, 1))
	RETV_ERROR (EIO, -1);
      file->dind_block = block;
      if (ext2_read_blocks (file->dind_bmap, fs, block, 1))
	RETV_ERROR (EIO, -1);
    }
  return 0;
}

/*!
 * Reads a block of an inode into its triply indirect block buffer.
 *
 * @param vp the vnode
 * @param block the block number
 * @return zero on success
 */

int
ext2_read_tind_bmap (struct vnode *vp, block_t block)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  if (!file->tind_bmap)
    {
      file->tind_bmap = alloc_virtual_page ();
      if (UNLIKELY (!file->tind_bmap))
	return -1;
    }
  if (file->tind_block != block)
    {
      if (file->tind_block
	  && ext2_write_blocks (file->tind_bmap, fs, file->tind_block, 1))
	RETV_ERROR (EIO, -1);
      file->tind_block = block;
      if (ext2_read_blocks (file->tind_bmap, fs, block, 1))
	RETV_ERROR (EIO, -1);
    }
  return 0;
}

/*!
 * Writes an inode's indirect block buffer to the on-disk structure.
 *
 * @param vp the vnode
 * @return zero on success
 */

int
ext2_write_ind_bmap (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  if (ext2_write_blocks (file->ind_bmap, fs, file->ind_block, 1))
    RETV_ERROR (EIO, -1);
  return 0;
}

/*!
 * Writes an inode's doubly indirect block buffer to the on-disk structure.
 *
 * @param vp the vnode
 * @return zero on success
 */

int
ext2_write_dind_bmap (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  if (ext2_write_blocks (file->dind_bmap, fs, file->dind_block, 1))
    RETV_ERROR (EIO, -1);
  return 0;
}

/*!
 * Writes an inode's triply indirect block buffer to the on-disk structure.
 *
 * @param vp the vnode
 * @return zero on success
 */

int
ext2_write_tind_bmap (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  if (ext2_write_blocks (file->tind_bmap, fs, file->tind_block, 1))
    RETV_ERROR (EIO, -1);
  return 0;
}

/*!
 * Determines the physical block numbers of one or more consecutive logical
 * blocks for a vnode.
 *
 * @param vp the vnode
 * @param blocks where to store physical block numbers. This must point to
 * a buffer capable of storing at least @p num @ref block_t values.
 * @param block first logical block number
 * @param num number of logical blocks past the first block to map
 * @return zero on success
 */

int
ext2_read_bmap (struct vnode *vp, block_t *blocks, block_t block, size_t num)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  size_t i;

  /* Read direct blocks */
  for (i = 0; block + i < EXT2_NDIR_BLOCKS; i++)
    {
      if (i >= num)
	return 0;
      blocks[i] = file->inode.i_block[block + i];
    }

  /* Read indirect blocks */
  if (ext2_read_ind_bmap (vp, file->inode.i_block[EXT2_IND_BLOCK]))
    return -1;
  for (; block + i < EXT2_IND_LIMIT; i++)
    {
      if (i >= num)
	return 0;
      blocks[i] = file->ind_bmap[block + i - EXT2_NDIR_BLOCKS];
    }

  /* Read doubly indirect blocks */
  if (ext2_read_dind_bmap (vp, file->inode.i_block[EXT2_DIND_BLOCK]))
    return -1;
  for (; block + i < EXT2_DIND_LIMIT; i++)
    {
      block_t dind_block;
      ext2_block_t ind_block;
      if (i >= num)
	return 0;
      dind_block = block + i - EXT2_IND_LIMIT;
      ind_block = file->dind_bmap[dind_block / fs->bmap_entries];
      if (ext2_read_ind_bmap (vp, ind_block))
	return -1;
      blocks[i] = file->ind_bmap[dind_block % fs->bmap_entries];
    }

  /* Read triply indirect blocks */
  if (ext2_read_tind_bmap (vp, file->inode.i_block[EXT2_TIND_BLOCK]))
    return -1;
  for (; block + i < EXT2_TIND_LIMIT; i++)
    {
      block_t tind_block;
      ext2_block_t dind_block;
      ext2_block_t ind_block;
      if (i >= num)
	return 0;
      tind_block = block + i - EXT2_DIND_LIMIT;
      dind_block =
	file->tind_bmap[tind_block / fs->bmap_entries / fs->bmap_entries];
      if (ext2_read_dind_bmap (vp, dind_block))
	return -1;
      ind_block =
	file->dind_bmap[tind_block / fs->bmap_entries % fs->bmap_entries];
      if (ext2_read_ind_bmap (vp, ind_block))
	return -1;
      blocks[i] = file->ind_bmap[tind_block % fs->bmap_entries];
    }

  /* File is too large at this point */
  RETV_ERROR (EFBIG, -1);
}

/*!
 * Sets the physical block numbers of one or more consecutive logical
 * blocks for a vnode. The vnode is expanded to include more blocks
 * if necessary.
 *
 * @param vp the vnode
 * @param blocks array of physical block numbers
 * @param block first logical block number
 * @param num number of logical blocks past the first block to map
 * @return zero on success
 */

int
ext2_write_bmap (struct vnode *vp, const block_t *blocks, block_t block,
		 size_t num)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  size_t i;

  /* Write direct blocks */
  for (i = 0; block + i < EXT2_NDIR_BLOCKS; i++)
    {
      if (i >= num)
	return 0;
      file->inode.i_block[block + i] = blocks[i];
    }

  /* Write indirect blocks */
  if (ext2_read_ind_bmap (vp, file->inode.i_block[EXT2_IND_BLOCK]))
    return -1;
  for (; block + i < EXT2_IND_LIMIT; i++)
    {
      if (i >= num)
	return 0;
      file->ind_bmap[block + i - EXT2_NDIR_BLOCKS] = blocks[i];
    }

  /* Write doubly indirect blocks */
  if (ext2_read_dind_bmap (vp, file->inode.i_block[EXT2_DIND_BLOCK]))
    return -1;
  for (; block + i < EXT2_DIND_LIMIT; i++)
    {
      block_t dind_block;
      ext2_block_t ind_block;
      if (i >= num)
	return 0;
      dind_block = block + i - EXT2_IND_LIMIT;
      ind_block = file->dind_bmap[dind_block / fs->bmap_entries];
      if (ext2_read_ind_bmap (vp, ind_block))
	return -1;
      file->ind_bmap[dind_block % fs->bmap_entries] = blocks[i];
    }

  /* Write triply indirect blocks */
  if (ext2_read_tind_bmap (vp, file->inode.i_block[EXT2_TIND_BLOCK]))
    return -1;
  for (; block + i < EXT2_TIND_LIMIT; i++)
    {
      block_t tind_block;
      ext2_block_t dind_block;
      ext2_block_t ind_block;
      if (i >= num)
	return 0;
      tind_block = block + i - EXT2_DIND_LIMIT;
      dind_block =
	file->tind_bmap[tind_block / fs->bmap_entries / fs->bmap_entries];
      if (ext2_read_dind_bmap (vp, dind_block))
	return -1;
      ind_block =
	file->dind_bmap[tind_block / fs->bmap_entries % fs->bmap_entries];
      if (ext2_read_ind_bmap (vp, ind_block))
	return -1;
      file->ind_bmap[tind_block % fs->bmap_entries] = blocks[i];
    }

  /* File is too large at this point */
  RETV_ERROR (EFBIG, -1);
}

ssize_t
ext2_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  size_t start_diff;
  size_t end_diff;
  block_t start_block;
  block_t mid_block;
  block_t end_block;
  size_t blocks;
  if (file->inode.i_flags & EXT4_INLINE_DATA_FL)
    RETV_ERROR (ENOTSUP, -1);

  start_block = offset / fs->block_size;
  mid_block = start_block + !!(offset % fs->block_size);
  end_block = (offset + len) / fs->block_size;
  blocks = end_block - mid_block;
  start_diff = mid_block * fs->block_size - offset;
  end_diff = offset + len - end_block * fs->block_size;

  if ((size_t) offset >= vp->size)
    RETV_ERROR (EINVAL, -1);
  if (offset + len > vp->size)
    len = vp->size - offset;

  /* Completely contained in a single block */
  if (mid_block > end_block)
    {
      if (ext2_read_io_buffer_block (vp, start_block))
	return -1;
      memcpy (buffer, file->io_buffer + fs->block_size - start_diff, len);
      return len;
    }

  /* Read full blocks */
  if (blocks)
    {
      block_t *bmap = malloc (sizeof (block_t) * blocks);
      size_t i;
      if (UNLIKELY (!bmap))
	RETV_ERROR (EIO, -1);
      if (ext2_read_bmap (vp, bmap, mid_block, blocks))
	{
	  free (bmap);
	  RETV_ERROR (EIO, -1);
	}
      for (i = 0; i < blocks; i++)
	{
	  if (ext2_read_blocks (buffer + start_diff + i * fs->block_size, fs,
				bmap[i], 1))
	    {
	      free (bmap);
	      RETV_ERROR (EIO, -1);
	    }
	}
      free (bmap);
    }

  /* Read unaligned starting bytes */
  if (start_diff)
    {
      if (ext2_read_io_buffer_block (vp, start_block))
	return -1;
      memcpy (buffer, file->io_buffer + fs->block_size - start_diff,
	      start_diff);
    }

  /* Read unaligned ending bytes */
  if (end_diff)
    {
      if (ext2_read_io_buffer_block (vp, end_block))
	return -1;
      memcpy (buffer + start_diff + blocks * fs->block_size, file->io_buffer,
	      end_diff);
    }
  return len;
}

ssize_t
ext2_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  RETV_ERROR (ENOSYS, -1);
}

void
ext2_sync (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  ext2_flush_io_buffer_block (vp);
  ext2_write_inode (&file->inode, vp->ino, fs);
  if (file->ind_block)
    ext2_write_ind_bmap (vp);
  if (file->dind_block)
    ext2_write_dind_bmap (vp);
  if (file->tind_block)
    ext2_write_tind_bmap (vp);
}

int
ext2_create (struct vnode **result, struct vnode *dir, const char *name,
	     mode_t mode, dev_t rdev)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_mkdir (struct vnode **result, struct vnode *dir, const char *name,
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
ext2_unlink (struct vnode *dir, const char *name)
{
  RETV_ERROR (ENOSYS, -1);
}

int
ext2_symlink (struct vnode *dir, const char *name, const char *target)
{
  RETV_ERROR (ENOSYS, -1);
}

ssize_t
ext2_readlink (struct vnode *vp, char *buffer, size_t len)
{
  size_t size = vp->size;
  struct ext2_inode *inode = vp->data;
  size_t i;
  if (size <= 60)
    {
      /* Link target is stored directly in block pointers */
      for (i = 0; i < len && i < size; i++)
	buffer[i] = ((char *) inode->i_block)[i];
      return 0;
    }
  else
    {
      if (size < len)
	len = size;
      return ext2_read (vp, buffer, len, 0);
    }
}

int
ext2_fill (struct vnode *vp)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = calloc (1, sizeof (struct ext2_file));
  if (UNLIKELY (!file))
    return -1;
  if (ext2_read_inode (&file->inode, vp->ino, fs))
    return -1;

  /* Fill vnode data */
  vp->mode = file->inode.i_mode;
  vp->nlink = file->inode.i_links_count;
  vp->uid = file->inode.i_uid;
  vp->gid = file->inode.i_gid;
  vp->rdev = 0;
  vp->atime.tv_sec = file->inode.i_atime;
  vp->mtime.tv_sec = file->inode.i_mtime;
  vp->ctime.tv_sec = file->inode.i_ctime;
  vp->atime.tv_nsec = vp->mtime.tv_nsec = vp->ctime.tv_nsec = 0;
  vp->blocks =
    (file->inode.i_blocks * 512 + fs->block_size - 1) / fs->block_size;
  vp->blksize = fs->block_size;

  /* Set size of file */
  vp->size = file->inode.i_size;
  if (fs->dynamic
      && (fs->super.s_feature_ro_compat & EXT2_FT_RO_COMPAT_LARGE_FILE))
    vp->size |= (size_t) file->inode.i_size_high << 32;

  vp->data = file;
  return 0;

 err0:
  free (file);
  return -1;
}

void
ext2_dealloc (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  free_virtual_page (file->io_buffer);
  free_virtual_page (file->ind_bmap);
  free_virtual_page (file->dind_bmap);
  free_virtual_page (file->tind_bmap);
  free (file);
}
