/* bitmap.c -- This file is part of PML.
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

/* Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
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
ext2_reserve_super_bgd (struct ext2_fs *fs, unsigned int group,
			struct ext2_bitmap *bmap)
{
  block_t super;
  block_t old_desc;
  block_t new_desc;
  blkcnt_t used;
  int old_desc_nblocks;
  int nblocks;
  ext2_super_bgd_loc (fs, group, &super, &old_desc, &new_desc, &used);

  if (fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_META_BG)
    old_desc_nblocks = fs->super.s_first_meta_bg;
  else
    old_desc_nblocks = fs->desc_blocks + fs->super.s_reserved_gdt_blocks;

  if (super || !group)
    ext2_mark_bitmap (bmap, super);
  if (group == 0 && fs->blksize == 1024 && EXT2_CLUSTER_RATIO (fs) > 1)
    ext2_mark_bitmap (bmap, 0);

  if (old_desc)
    {
      nblocks = old_desc_nblocks;
      if (old_desc + nblocks >= (block_t) ext2_blocks_count (&fs->super))
	nblocks = ext2_blocks_count (&fs->super) - old_desc;
      ext2_mark_block_bitmap_range (bmap, old_desc, nblocks);
    }
  if (new_desc)
    ext2_mark_bitmap (bmap, new_desc);

  nblocks = ext2_group_blocks_count (fs, group);
  nblocks -= fs->inode_blocks_per_group + used + 2;
  return nblocks;
}

static int
ext2_mark_uninit_bg_group_blocks (struct ext2_fs *fs)
{
  struct ext2_bitmap *bmap = fs->block_bitmap;
  block_t block;
  unsigned int i;
  for (i = 0; i < fs->group_desc_count; i++)
    {
      if (!ext2_bg_test_flags (fs, i, EXT2_BG_BLOCK_UNINIT))
	continue;
      ext2_reserve_super_bgd (fs, i, bmap);
      block = ext2_inode_table_loc (fs, i);
      if (block)
	ext2_mark_block_bitmap_range (bmap, block, fs->inode_blocks_per_group);
      block = ext2_block_bitmap_loc (fs, i);
      if (block)
	ext2_mark_bitmap (bmap, block);
      block = ext2_inode_bitmap_loc (fs, i);
      if (block)
	ext2_mark_bitmap (bmap, block);
    }
  return 0;
}

static int
ext2_make_bitmap_32 (struct ext2_fs *fs, int magic, uint32_t start,
		     uint32_t end, uint32_t real_end, char *initmap,
		     struct ext2_bitmap **result)
{
  struct ext2_bitmap32 *bmap = malloc (sizeof (struct ext2_bitmap32));
  size_t size;
  if (UNLIKELY (!bmap))
    RETV_ERROR (ENOMEM, -1);
  bmap->magic = magic;
  bmap->start = start;
  bmap->end = end;
  bmap->real_end = real_end;

  size = (size_t) ((bmap->real_end - bmap->start) / 8 + 1);
  size = (size + 7) & ~3;
  bmap->bitmap = malloc (size);
  if (UNLIKELY (!bmap->bitmap))
    {
      free (bmap);
      RETV_ERROR (ENOMEM, -1);
    }

  if (initmap)
    memcpy (bmap->bitmap, initmap, size);
  else
    memset (bmap->bitmap, 0, size);
  *result = (struct ext2_bitmap *) bmap;
  return 0;
}

/* TODO Support making rbtree bitmaps */

static int
ext2_make_bitmap_64 (struct ext2_fs *fs, int magic, enum ext2_bitmap_type type,
		     uint64_t start, uint64_t end, uint64_t real_end,
		     struct ext2_bitmap **result)
{
  struct ext2_bitmap64 *bmap;
  struct ext2_bitmap_ops *ops;
  int ret;

  if (type == EXT2_BMAP64_BITARRAY)
    ops = &ext2_bitarray_ops;
  else
    RETV_ERROR (EUCLEAN, -1);

  bmap = malloc (sizeof (struct ext2_bitmap64));
  if (UNLIKELY (!bmap))
    RETV_ERROR (ENOMEM, -1);
  bmap->magic = magic;
  bmap->start = start;
  bmap->end = end;
  bmap->real_end = real_end;
  bmap->ops = ops;
  if (magic == EXT2_BMAP_MAGIC_BLOCK64)
    bmap->cluster_bits = fs->cluster_ratio_bits;
  else
    bmap->cluster_bits = 0;

  ret = bmap->ops->new_bmap (fs, bmap);
  if (ret)
    {
      free (bmap);
      return ret;
    }
  *result = (struct ext2_bitmap *) bmap;
  return 0;
}

static int
ext2_allocate_block_bitmap (struct ext2_fs *fs, struct ext2_bitmap **result)
{
  uint64_t start = EXT2_B2C (fs, fs->super.s_first_data_block);
  uint64_t end = EXT2_B2C (fs, ext2_blocks_count (&fs->super) - 1);
  uint64_t real_end = (uint64_t) fs->super.s_clusters_per_group *
    (uint64_t) fs->group_desc_count - 1 + start;
  if (fs->flags & EXT2_FLAG_64BIT)
    return ext2_make_bitmap_64 (fs, EXT2_BMAP_MAGIC_BLOCK64,
				EXT2_BMAP64_BITARRAY, start, end, real_end,
				result);
  if (end > 0xffffffff || real_end > 0xffffffff)
    RETV_ERROR (EUCLEAN, -1);
  return ext2_make_bitmap_32 (fs, EXT2_BMAP_MAGIC_BLOCK, start, end, real_end,
			      NULL, result);
}

static int
ext2_allocate_inode_bitmap (struct ext2_fs *fs, struct ext2_bitmap **result)
{
  uint64_t start = 1;
  uint64_t end = fs->super.s_inodes_count;
  uint64_t real_end =
    (uint64_t) fs->super.s_inodes_per_group * fs->group_desc_count;
  if (fs->flags & EXT2_FLAG_64BIT)
    return ext2_make_bitmap_64 (fs, EXT2_BMAP_MAGIC_INODE64,
				EXT2_BMAP64_BITARRAY, start, end, real_end,
				result);
  if (end > 0xffffffff || real_end > 0xffffffff)
    RETV_ERROR (EUCLEAN, -1);
  return ext2_make_bitmap_32 (fs, EXT2_BMAP_MAGIC_INODE, start, end, real_end,
			      NULL, result);
}

static void
ext2_free_bitmap (struct ext2_bitmap *bmap)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  if (!b)
    return;
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      if (!EXT2_BMAP_MAGIC_VALID (b32->magic))
	return;
      b32->magic = 0;
      if (b32->bitmap)
	{
	  free (b32->bitmap);
	  b32->bitmap = NULL;
	}
      free (b32);
      return;
    }
  if (!EXT2_BITMAP_IS_64 (b))
    return;
  b->ops->free_bmap (b);
  b->magic = 0;
  free (b);
}

static int
ext2_prepare_read_bitmap (struct ext2_fs *fs, int flags)
{
  int block_nbytes = fs->super.s_clusters_per_group / 8;
  int inode_nbytes = fs->super.s_inodes_per_group / 8;
  int ret;
  if (UNLIKELY (block_nbytes > fs->blksize || inode_nbytes > fs->blksize))
    RETV_ERROR (EUCLEAN, -1);

  if (flags & EXT2_BITMAP_BLOCK)
    {
      if (fs->block_bitmap)
	ext2_free_bitmap (fs->block_bitmap);
      ret = ext2_allocate_block_bitmap (fs, &fs->block_bitmap);
      if (ret)
	{
	  ext2_free_bitmap (fs->block_bitmap);
	  fs->block_bitmap = NULL;
	  return ret;
	}
    }

  if (flags & EXT2_BITMAP_INODE)
    {
      if (fs->inode_bitmap)
	ext2_free_bitmap (fs->inode_bitmap);
      ret = ext2_allocate_inode_bitmap (fs, &fs->inode_bitmap);
      if (ret)
	{
	  ext2_free_bitmap (fs->inode_bitmap);
	  fs->inode_bitmap = NULL;
	  return ret;
	}
    }
  return 0;
}

static int
ext2_read_bitmap_start (struct ext2_fs *fs, int flags, unsigned int start,
			unsigned int end)
{
  char *block_bitmap = NULL;
  char *inode_bitmap = NULL;
  int block_nbytes = fs->super.s_clusters_per_group / 8;
  int inode_nbytes = fs->super.s_inodes_per_group / 8;
  int csum_flag = ext2_has_group_desc_checksum (&fs->super);
  unsigned int count;
  unsigned int i;
  block_t block;
  block_t blkitr = EXT2_B2C (fs, fs->super.s_first_data_block);
  ino_t inoitr = 1;
  int ret;

  if (flags & EXT2_BITMAP_BLOCK)
    {
      block_bitmap = malloc (fs->blksize);
      if (UNLIKELY (!block_bitmap))
	GOTO_ERROR (ENOMEM, end);
    }
  else
    block_nbytes = 0;

  if (flags & EXT2_BITMAP_INODE)
    {
      inode_bitmap = malloc (fs->blksize);
      if (UNLIKELY (!inode_bitmap))
	GOTO_ERROR (ENOMEM, end);
    }
  else
    inode_nbytes = 0;

  blkitr += (block_t) start * (block_nbytes << 3);
  inoitr += (block_t) start * (inode_nbytes << 3);
  for (i = start; i <= end; i++)
    {
      if (block_bitmap)
	{
	  block = ext2_block_bitmap_loc (fs, i);
	  if ((csum_flag && ext2_bg_test_flags (fs, i, EXT2_BG_BLOCK_UNINIT)
	       && ext2_group_desc_checksum_valid (fs, i))
	      || block >= (block_t) ext2_blocks_count (&fs->super))
	    block = 0;
	  if (block)
	    {
	      ret = ext2_read_blocks (block_bitmap, fs, block, 1);
	      if (ret)
		GOTO_ERROR (EIO, end);
	      if (!ext2_block_bitmap_checksum_valid (fs, i, block_bitmap,
						     block_nbytes))
		GOTO_ERROR (EUCLEAN, end);
	    }
	  else
	    memset (block_bitmap, 0, block_nbytes);

	  count = block_nbytes << 3;
	  ret = ext2_set_bitmap_range (fs->block_bitmap, blkitr, count,
				       block_bitmap);
	  if (ret)
	    goto end;
	  blkitr += block_nbytes << 3;
	}

      if (inode_bitmap)
	{
	  block = ext2_inode_bitmap_loc (fs, i);
	  if ((csum_flag && ext2_bg_test_flags (fs, i, EXT2_BG_INODE_UNINIT)
	       && ext2_group_desc_checksum_valid (fs, i))
	      || block >= (block_t) ext2_blocks_count (&fs->super))
	    block = 0;
	  if (block)
	    {
	      ret = ext2_read_blocks (inode_bitmap, fs, block, 1);
	      if (ret)
		GOTO_ERROR (EIO, end);
	      if (!ext2_inode_bitmap_checksum_valid (fs, i, inode_bitmap,
						     inode_nbytes))
		GOTO_ERROR (EUCLEAN, end);
	    }
	  else
	    memset (inode_bitmap, 0, inode_nbytes);

	  count = inode_nbytes << 3;
	  ret = ext2_set_bitmap_range (fs->inode_bitmap, inoitr, count,
				       inode_bitmap);
	  if (ret)
	    goto end;
	  inoitr += inode_nbytes << 3;
	}
    }

 end:
  if (block_bitmap)
    {
      free (block_bitmap);
      block_bitmap = NULL;
    }
  if (inode_bitmap)
    {
      free (inode_bitmap);
      inode_bitmap = NULL;
    }
  return ret;
}

static int
ext2_read_bitmap_end (struct ext2_fs *fs, int flags)
{
  if (flags & EXT2_BITMAP_BLOCK)
    {
      int ret = ext2_mark_uninit_bg_group_blocks (fs);
      if (ret)
	return ret;
    }
  return 0;
}

static void
ext2_clean_bitmap (struct ext2_fs *fs, int flags)
{
  if (flags & EXT2_BITMAP_BLOCK)
    {
      ext2_free_bitmap (fs->block_bitmap);
      fs->block_bitmap = NULL;
    }
  if (flags & EXT2_BITMAP_INODE)
    {
      ext2_free_bitmap (fs->inode_bitmap);
      fs->inode_bitmap = NULL;
    }
}

int
ext2_read_bitmap (struct ext2_fs *fs, int flags, unsigned int start,
		  unsigned int end)
{
  int ret = ext2_prepare_read_bitmap (fs, flags);
  if (ret)
    return ret;
  ret = ext2_read_bitmap_start (fs, flags, start, end);
  if (!ret)
    ret = ext2_read_bitmap_end (fs, flags);
  if (ret)
    ext2_clean_bitmap (fs, flags);
  return ret;
}

int
ext2_read_bitmaps (struct ext2_fs *fs)
{
  int flags = 0;
  if (!fs->block_bitmap)
    flags |= EXT2_BITMAP_BLOCK;
  if (!fs->inode_bitmap)
    flags |= EXT2_BITMAP_INODE;
  if (flags == 0)
    return 0;
  return ext2_read_bitmap (fs, flags, 0, fs->group_desc_count - 1);
}

int
ext2_write_bitmaps (struct ext2_fs *fs)
{
  int do_block = fs->block_bitmap && (fs->flags & EXT2_FLAG_BB_DIRTY);
  int do_inode = fs->inode_bitmap && (fs->flags & EXT2_FLAG_IB_DIRTY);
  unsigned int i;
  unsigned int j;
  int block_nbytes = 0;
  int inode_nbytes = 0;
  unsigned int nbits;
  char *blockbuf = NULL;
  char *inodebuf = NULL;
  int csum_flag;
  block_t block;
  block_t blkitr = EXT2_B2C (fs, fs->super.s_first_data_block);
  ino_t inoitr = 1;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!do_block && !do_inode)
    return 0;
  csum_flag = ext2_has_group_desc_checksum (&fs->super);

  if (do_block)
    {
      block_nbytes = fs->super.s_clusters_per_group / 8;
      blockbuf = malloc (fs->blksize);
      if (UNLIKELY (!blockbuf))
	GOTO_ERROR (ENOMEM, err);
      memset (blockbuf, 0xff, fs->blksize);
    }
  if (do_inode)
    {
      inode_nbytes = (size_t) ((fs->super.s_inodes_per_group + 7) / 8);
      inodebuf = malloc (fs->blksize);
      if (UNLIKELY (!inodebuf))
	GOTO_ERROR (ENOMEM, err);
      memset (inodebuf, 0xff, fs->blksize);
    }

  for (i = 0; i < fs->group_desc_count; i++)
    {
      if (!do_block)
	goto skip_block;
      if (csum_flag && ext2_bg_test_flags (fs, i, EXT2_BG_BLOCK_UNINIT))
	goto skip_curr_block;

      ret = ext2_get_bitmap_range (fs->block_bitmap, blkitr, block_nbytes << 3,
				   blockbuf);
      if (ret)
	goto err;
      if (i == fs->group_desc_count - 1)
	{
	  nbits = EXT2_NUM_B2C (fs, (ext2_blocks_count (&fs->super) -
				     (block_t) fs->super.s_first_data_block) %
				(block_t) fs->super.s_blocks_per_group);
	  if (nbits)
	    {
	      for (j = nbits; j < fs->blksize * 8; j++)
		set_bit (blockbuf, j);
	    }
	}

      ext2_block_bitmap_checksum_update (fs, i, blockbuf, block_nbytes);
      ext2_group_desc_checksum_update (fs, i);
      fs->flags |= EXT2_FLAG_DIRTY;

      block = ext2_block_bitmap_loc (fs, i);
      if (block)
	{
	  ret = ext2_write_blocks (blockbuf, fs, block, 1);
	  if (ret)
	    GOTO_ERROR (EIO, err);
	}
    skip_curr_block:
      blkitr += block_nbytes << 3;

    skip_block:
      if (!do_inode)
	continue;
      if (csum_flag && ext2_bg_test_flags (fs, i, EXT2_BG_INODE_UNINIT))
	goto skip_curr_inode;

      ret = ext2_get_bitmap_range (fs->inode_bitmap, inoitr, inode_nbytes << 3,
				   inodebuf);
      if (ret)
	goto err;
      ext2_inode_bitmap_checksum_update (fs, i, inodebuf, inode_nbytes);
      ext2_group_desc_checksum_update (fs, i);
      fs->flags |= EXT2_FLAG_DIRTY;

      block = ext2_inode_bitmap_loc (fs, i);
      if (block)
	{
	  ret = ext2_write_blocks (inodebuf, fs, block, 1);
	  if (ret)
	    GOTO_ERROR (EIO, err);
	}
    skip_curr_inode:
      inoitr += inode_nbytes << 3;
    }

  if (do_block)
    {
      fs->flags &= ~EXT2_FLAG_BB_DIRTY;
      free (blockbuf);
    }
  if (do_inode)
    {
      fs->flags &= ~EXT2_FLAG_IB_DIRTY;
      free (inodebuf);
    }
  return 0;

 err:
  if (blockbuf)
    free (blockbuf);
  if (inodebuf)
    free (inodebuf);
  return ret;
}

void
ext2_mark_bitmap (struct ext2_bitmap *bmap, uint64_t arg)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  if (!b)
    return;
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      if (arg & ~0xffffffffUL)
	return;
      if (arg < b32->start || arg > b32->end)
	return;
      set_bit (b32->bitmap, arg - b32->start);
    }
  else
    {
      arg >>= b->cluster_bits;
      if (arg < b->start || arg > b->end)
	return;
      b->ops->mark_bmap (b, arg);
    }
}

void
ext2_unmark_bitmap (struct ext2_bitmap *bmap, uint64_t arg)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  if (!b)
    return;
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      if (arg & ~0xffffffffUL)
	return;
      if (arg < b32->start || arg > b32->end)
	return;
      clear_bit (b32->bitmap, arg - b32->start);
    }
  else
    {
      arg >>= b->cluster_bits;
      if (arg < b->start || arg > b->end)
	return;
      b->ops->unmark_bmap (b, arg);
    }
}

int
ext2_test_bitmap (struct ext2_bitmap *bmap, uint64_t arg)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  if (!b)
    return 0;
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      if (arg & ~0xffffffffUL)
	return 0;
      if (arg < b32->start || arg > b32->end)
	return 0;
      return test_bit (b32->bitmap, arg - b32->start);
    }
  else
    {
      arg >>= b->cluster_bits;
      if (arg < b->start || arg > b->end)
	return 0;
      return b->ops->test_bmap (b, arg);
    }
}

void
ext2_mark_block_bitmap_range (struct ext2_bitmap *bmap, block_t block,
			      blkcnt_t num)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  block_t end = block + num;
  if (!b)
    return;
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      int i;
      if ((block & ~0xffffffffUL) || ((block + num - 1) & ~0xffffffffUL))
	return;
      if (block < b32->start || block > b32->end || block + num - 1 > b32->end)
	return;
      for (i = 0; i < num; i++)
	set_bit (b32->bitmap, block + i - b32->start);
    }
  if (!EXT2_BITMAP_IS_64 (b))
    return;
  block >>= b->cluster_bits;
  end += (1 << b->cluster_bits) - 1;
  end >>= b->cluster_bits;
  num = end - block;
  if (block < b->start || block > b->end || block + num - 1 > b->end)
    return;
  b->ops->mark_bmap_extent (b, block, num);
}

int
ext2_set_bitmap_range (struct ext2_bitmap *bmap, uint64_t start,
		       unsigned int num, void *data)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  if (!b)
    RETV_ERROR (EINVAL, -1);
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      if ((start + num - 1) & ~0xffffffffUL)
	RETV_ERROR (EUCLEAN, -1);
      if (start < b32->start || start + num - 1 > b32->real_end)
	RETV_ERROR (EUCLEAN, -1);
      memcpy (b32->bitmap + ((start - b32->start) >> 3), data, (num + 7) >> 3);
      return 0;
    }
  if (!EXT2_BITMAP_IS_64 (b))
    RETV_ERROR (EUCLEAN, -1);
  b->ops->set_bmap_range (b, start, num, data);
  return 0;
}

int
ext2_get_bitmap_range (struct ext2_bitmap *bmap, uint64_t start,
		       unsigned int num, void *data)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  if (!b)
    RETV_ERROR (EINVAL, -1);
  if (EXT2_BITMAP_IS_32 (b))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      if ((start + num - 1) & ~0xffffffffUL)
	RETV_ERROR (EUCLEAN, -1);
      if (start < b32->start || start + num - 1 > b32->real_end)
	RETV_ERROR (EUCLEAN, -1);
      memcpy (data, b32->bitmap + ((start - b32->start) >> 3), (num + 7) >> 3);
      return 0;
    }
  if (!EXT2_BITMAP_IS_64 (b))
    RETV_ERROR (EUCLEAN, -1);
  b->ops->get_bmap_range (b, start, num, data);
  return 0;
}

int
ext2_find_first_zero_bitmap (struct ext2_bitmap *bmap, block_t start,
			     block_t end, block_t *result)
{
  struct ext2_bitmap64 *b = (struct ext2_bitmap64 *) bmap;
  uint64_t cstart;
  uint64_t cend;
  uint64_t cout;
  int ret;

  if (!bmap)
    RETV_ERROR (EINVAL, -1);
  if (EXT2_BITMAP_IS_32 (bmap))
    {
      struct ext2_bitmap32 *b32 = (struct ext2_bitmap32 *) bmap;
      uint32_t block;
      if ((start & ~0xffffffffUL) || (end & ~0xffffffffUL))
	RETV_ERROR (EUCLEAN, -1);
      if (start < b32->start || end > b32->end || start > end)
	RETV_ERROR (EUCLEAN, -1);
      while (start <= end)
	{
	  block = test_bit (b32->bitmap, start - b32->start);
	  if (!block)
	    {
	      *result = start;
	      return 0;
	    }
	  start++;
	}
      RETV_ERROR (ENOENT, -1);
    }
  if (!EXT2_BITMAP_IS_64 (bmap))
    RETV_ERROR (EUCLEAN, -1);

  cstart = start >> b->cluster_bits;
  cend = end >> b->cluster_bits;
  if (cstart < b->start || cend > b->end || start > end)
    RETV_ERROR (EUCLEAN, -1);
  if (b->ops->find_first_zero)
    {
      ret = b->ops->find_first_zero (b, cstart, cend, &cout);
      if (ret)
	return ret;
    found:
      cout <<= b->cluster_bits;
      *result = cout >= start ? cout : start;
      return 0;
    }

  for (cout = cstart; cout <= cend; cout++)
    {
      if (!b->ops->test_bmap (b, cout))
	goto found;
    }
  RETV_ERROR (ENOENT, -1);
}
