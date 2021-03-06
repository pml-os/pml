/* bmap.c -- This file is part of PML.
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

/* Copyright (C) 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#include <pml/ext2fs.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int
block_ind_bmap (struct ext2_fs *fs, int flags, uint32_t ind, char *blockbuf,
		int *blocks_alloc, block_t blockno, uint32_t *result)
{
  uint32_t b;
  int ret;
  if (!ind)
    {
      if (flags & BMAP_SET)
	RETV_ERROR (EINVAL, -1);
      *result = 0;
      return 0;
    }

  ret = ext2_read_blocks (blockbuf, fs, ind, 1);
  if (ret)
    return ret;

  if (flags & BMAP_SET)
    {
      b = *result;
      ((uint32_t *) blockbuf)[blockno] = b;
      return ext2_write_blocks (blockbuf, fs, ind, 1);
    }

  b = ((uint32_t *) blockbuf)[blockno];
  if (!b && (flags & BMAP_ALLOC))
    {
      block_t b64;
      b = blockno ? ((uint32_t *) blockbuf)[blockno - 1] : ind;
      b64 = b;
      ret = ext2_alloc_block (fs, b64, blockbuf + fs->blksize, &b64, NULL);
      b = 64;
      if (ret)
	return ret;
      ((uint32_t *) blockbuf)[blockno] = b;

      ret = ext2_write_blocks (blockbuf, fs, ind, 1);
      if (ret)
	return ret;
      blocks_alloc++;
    }

  *result = b;
  return 0;
}

static int
block_dind_bmap (struct ext2_fs *fs, int flags, uint32_t dind, char *blockbuf,
		 int *block_alloc, block_t blockno, uint32_t *result)
{
  uint32_t addr_per_block = fs->blksize >> 2;
  uint32_t b;
  int ret = block_ind_bmap (fs, flags & ~BMAP_SET, dind, blockbuf, block_alloc,
			    blockno / addr_per_block, &b);
  if (ret)
    return ret;
  return block_ind_bmap (fs, flags, b, blockbuf, block_alloc,
			 blockno % addr_per_block, result);
}

static int
block_tind_bmap (struct ext2_fs *fs, int flags, uint32_t tind, char *blockbuf,
		 int *block_alloc, block_t blockno, uint32_t *result)
{
  uint32_t addr_per_block = fs->blksize >> 2;
  uint32_t b;
  int ret = block_dind_bmap (fs, flags & ~BMAP_SET, tind, blockbuf, block_alloc,
			     blockno / addr_per_block, &b);
  if (ret)
    return ret;
  return block_ind_bmap (fs, flags, b, blockbuf, block_alloc,
			 blockno % addr_per_block, result);
}

int
ext2_bmap (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
	   char *blockbuf, int flags, block_t block, int *retflags,
	   block_t *physblock)
{
  struct ext2_inode inode_buf;
  struct ext3_extent_handle *handle = NULL;
  block_t addr_per_block;
  uint32_t b;
  uint32_t b32;
  block_t b64;
  char *buffer = NULL;
  int blocks_alloc = 0;
  int inode_dirty = 0;
  int ret = 0;
  struct ext2_balloc_ctx alloc_ctx = {ino, inode, 0, BLOCK_ALLOC_DATA};

  if (!(flags & BMAP_SET))
    *physblock = 0;
  if (retflags)
    *retflags = 0;

  if (!inode)
    {
      ret = ext2_read_inode (fs, ino, &inode_buf);
      if (ret)
	return ret;
      inode = &inode_buf;
    }
  addr_per_block = (block_t) fs->blksize >> 2;

  if (ext2_file_block_offset_too_big (fs, inode, block))
    RETV_ERROR (EFBIG, -1);
  if (inode->i_flags & EXT4_INLINE_DATA_FL)
    RETV_ERROR (EUCLEAN, -1);

  if (!blockbuf)
    {
      buffer = malloc (fs->blksize * 2);
      if (UNLIKELY (!buffer))
	RETV_ERROR (ENOMEM, -1);
      blockbuf = buffer;
    }

  if (inode->i_flags & EXT4_EXTENTS_FL)
    {
      ret = ext3_extent_open (fs, ino, inode, &handle);
      if (ret)
	goto end;
      ret = ext3_extent_bmap (fs, ino, inode, handle, blockbuf, flags, block,
			      retflags, &blocks_alloc, physblock);
      goto end;
    }

  if (block < EXT2_NDIR_BLOCKS)
    {
      if (flags & BMAP_SET)
	{
	  b = *physblock;
	  inode->i_block[block] = b;
	  inode_dirty++;
	  goto end;
	}

      *physblock = inode->i_block[block];
      b = block ? inode->i_block[block - 1] :
	ext2_find_inode_goal (fs, ino, inode, block);
      if (!*physblock && (flags & BMAP_ALLOC))
	{
	  b64 = b;
	  ret = ext2_alloc_block (fs, b64, blockbuf, &b64, &alloc_ctx);
	  b = b64;
	  if (ret)
	    goto end;
	  inode->i_block[block] = b;
	  blocks_alloc++;
	  *physblock = b;
	}
      goto end;
    }

  /* Indirect block */
  block -= EXT2_NDIR_BLOCKS;
  b32 = *physblock;
  if (block < addr_per_block)
    {
      b = inode->i_block[EXT2_IND_BLOCK];
      if (!b)
	{
	  if (!(flags & BMAP_ALLOC))
	    {
	      if (flags & BMAP_SET)
		{
		  ret = -1;
		  errno = EINVAL;
		}
	      goto end;
	    }
	  b = inode->i_block[EXT2_IND_BLOCK - 1];
	  b64 = b;
	  ret = ext2_alloc_block (fs, b64, blockbuf, &b64, &alloc_ctx);
	  b = b64;
	  if (ret)
	    goto end;
	  inode->i_block[EXT2_IND_BLOCK] = b;
	  blocks_alloc++;
	}
      ret = block_ind_bmap (fs, flags, b, blockbuf, &blocks_alloc, block, &b32);
      if (!ret)
	*physblock = b32;
      goto end;
    }

  /* Doubly indirect block */
  block -= addr_per_block;
  if (block < addr_per_block * addr_per_block)
    {
      b = inode->i_block[EXT2_DIND_BLOCK];
      if (!b)
	{
	  if (!(flags & BMAP_ALLOC))
	    {
	      if (flags & BMAP_SET)
		{
		  ret = -1;
		  errno = EINVAL;
		}
	      goto end;
	    }
	  b = inode->i_block[EXT2_IND_BLOCK];
	  b64 = b;
	  ret = ext2_alloc_block (fs, b64, blockbuf, &b64, &alloc_ctx);
	  b = b64;
	  if (ret)
	    goto end;
	  inode->i_block[EXT2_DIND_BLOCK] = b;
	  blocks_alloc++;
	}
      ret =
	block_dind_bmap (fs, flags, b, blockbuf, &blocks_alloc, block, &b32);
      if (!ret)
	*physblock = b32;
      goto end;
    }

  /* Triply indirect block */
  block -= addr_per_block * addr_per_block;
  b = inode->i_block[EXT2_TIND_BLOCK];
  if (!b)
    {
      if (!(flags & BMAP_ALLOC))
	{
	  if (flags & BMAP_SET)
	    {
	      ret = -1;
	      errno = EINVAL;
	    }
	  goto end;
	}
      b = inode->i_block[EXT2_DIND_BLOCK];
      b64 = b;
      ret = ext2_alloc_block (fs, b64, blockbuf, &b64, &alloc_ctx);
      b = b64;
      if (ret)
	goto end;
      inode->i_block[EXT2_TIND_BLOCK] = b;
      blocks_alloc++;
    }
  ret = block_tind_bmap (fs, flags, b, blockbuf, &blocks_alloc, block, &b32);
  if (!ret)
    *physblock = b32;

 end:
  if (*physblock && !ret && (flags & BMAP_ZERO))
    ret = ext2_zero_blocks (fs, *physblock, 1, NULL, NULL);
  if (buffer)
    free (buffer);
  if (handle)
    ext3_extent_free (handle);
  if (!ret && (blocks_alloc || inode_dirty))
    {
      ext2_iblk_add_blocks (fs, inode, blocks_alloc);
      ret = ext2_update_inode (fs, ino, inode, sizeof (struct ext2_inode));
    }
  return ret;
}
