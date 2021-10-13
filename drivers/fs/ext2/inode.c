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
  .symlink = ext2_symlink,
  .readdir = ext2_readdir,
  .bmap = ext2_bmap,
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
  block_t block = fs->group_descs[group].bg_inode_table +
    index * fs->inode_size / fs->block_size;
  index %= fs->block_size / fs->inode_size;
  if (block != fs->inode_table.block)
    {
      if (ext2_read_blocks (fs->inode_table.buffer, fs, block, 1))
	return -1;
      fs->inode_table.block = block;
    }
  memcpy (inode, fs->inode_table.buffer + index * fs->inode_size,
	  sizeof (struct ext2_inode));
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

int
ext2_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  RETV_ERROR (ENOSYS, -1);
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
      block_t block;
      if (ext2_alloc_io_buffer (file))
	RETV_ERROR (EIO, -1);
      if (vfs_bmap (vp, &block, start_block, 1))
	RETV_ERROR (EIO, -1);
      if (ext2_read_blocks (file->io_buffer, fs, block, 1))
	RETV_ERROR (EIO, -1);
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
      if (vfs_bmap (vp, bmap, mid_block, blocks))
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
      block_t block;
      if (ext2_alloc_io_buffer (file))
	RETV_ERROR (EIO, -1);
      if (vfs_bmap (vp, &block, start_block, 1))
	RETV_ERROR (EIO, -1);
      if (ext2_read_blocks (file->io_buffer, fs, block, 1))
	RETV_ERROR (EIO, -1);
      memcpy (buffer, file->io_buffer + fs->block_size - start_diff,
	      start_diff);
    }

  /* Read unaligned ending bytes */
  if (end_diff)
    {
      block_t block;
      if (ext2_alloc_io_buffer (file))
	RETV_ERROR (EIO, -1);
      if (vfs_bmap (vp, &block, end_block, 1))
	RETV_ERROR (EIO, -1);
      if (ext2_read_blocks (file->io_buffer, fs, block, 1))
	RETV_ERROR (EIO, -1);
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
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  size_t i;

  /* Read direct blocks */
  for (i = 0; block + i < EXT2_NDIR_BLOCKS; i++)
    {
      if (i >= num)
	return 0;
      result[i] = file->inode.i_block[block + i];
    }

  /* Allocate and read indirect block buffer */
  if (!file->ind_bmap)
    {
      file->ind_bmap = alloc_virtual_page ();
      if (UNLIKELY (!file->ind_bmap))
	return -1;
    }
  if (file->ind_block != file->inode.i_block[EXT2_IND_BLOCK])
    {
      file->ind_block = file->inode.i_block[EXT2_IND_BLOCK];
      if (ext2_read_blocks (file->ind_bmap, fs, file->ind_block, 1))
	RETV_ERROR (EIO, -1);
    }

  /* Read indirect blocks */
  for (; block + i < EXT2_IND_LIMIT; i++)
    {
      if (i >= num)
	return 0;
      result[i] = file->ind_bmap[block + i - EXT2_NDIR_BLOCKS];
    }

  /* Allocate and read doubly indirect block buffer */
  if (!file->dind_bmap)
    {
      file->dind_bmap = alloc_virtual_page ();
      if (UNLIKELY (!file->dind_bmap))
	return -1;
    }
  if (file->dind_block != file->inode.i_block[EXT2_DIND_BLOCK])
    {
      file->dind_block = file->inode.i_block[EXT2_DIND_BLOCK];
      if (ext2_read_blocks (file->dind_bmap, fs, file->dind_block, 1))
	RETV_ERROR (EIO, -1);
    }

  /* Read doubly indirect blocks */
  for (; block + i < EXT2_DIND_LIMIT; i++)
    {
      if (i >= num)
	return 0;
      result[i] = file->dind_bmap[block + i - EXT2_IND_LIMIT];
    }

  /* Allocate and read triply indirect block buffer */
  if (!file->tind_bmap)
    {
      file->tind_bmap = alloc_virtual_page ();
      if (UNLIKELY (!file->tind_bmap))
	return -1;
      if (ext2_read_blocks (file->tind_bmap, fs,
			    file->inode.i_block[EXT2_TIND_BLOCK], 1))
	RETV_ERROR (EIO, -1);
    }

  /* Read triply indirect blocks */
  for (; block + i < EXT2_TIND_LIMIT; i++)
    {
      if (i >= num)
	return 0;
      result[i] = file->tind_bmap[block + i - EXT2_DIND_LIMIT];
    }

  /* File is too large at this point */
  RETV_ERROR (EFBIG, -1);
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
  vp->atime.sec = file->inode.i_atime;
  vp->mtime.sec = file->inode.i_mtime;
  vp->ctime.sec = file->inode.i_ctime;
  vp->atime.nsec = vp->mtime.nsec = vp->ctime.nsec = 0;
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
