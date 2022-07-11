/* util.c -- This file is part of PML.
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

#include <pml/ext2fs.h>
#include <pml/memory.h>
#include <errno.h>
#include <string.h>

static int
ext2_bg_super_test_root (unsigned int group, unsigned int x)
{
  while (1)
    {
      if (group < x)
	return 0;
      if (group == x)
	return 1;
      if (group % x)
	return 0;
      group /= x;
    }
}

static void
ext2_clear_block_uninit (struct ext2_fs *fs, unsigned int group)
{
  if (group >= fs->group_desc_count
      || !ext2_has_group_desc_checksum (&fs->super)
      || !ext2_bg_test_flags (fs, group, EXT2_BG_BLOCK_UNINIT))
    return;
  ext2_bg_clear_flags (fs, group, EXT2_BG_BLOCK_UNINIT);
  ext2_group_desc_checksum_update (fs, group);
  fs->flags |= EXT2_FLAG_CHANGED | EXT2_FLAG_DIRTY | EXT2_FLAG_BB_DIRTY;
}

static void
ext2_check_inode_uninit (struct ext2_fs *fs, struct ext2_bitmap *map,
			 unsigned int group)
{
  ino_t ino;
  ino_t i;
  if (group >= fs->group_desc_count
      || !ext2_has_group_desc_checksum (&fs->super)
      || !ext2_bg_test_flags (fs, group, EXT2_BG_INODE_UNINIT))
    return;
  ino = group * fs->super.s_inodes_per_group + 1;
  for (i = 0; i < fs->super.s_inodes_per_group; i++, ino++)
    ext2_unmark_bitmap (map, ino);
  ext2_bg_clear_flags (fs, group, EXT2_BG_INODE_UNINIT | EXT2_BG_BLOCK_UNINIT);
  ext2_group_desc_checksum_update (fs, group);
  fs->flags |= EXT2_FLAG_CHANGED | EXT2_FLAG_DIRTY | EXT2_FLAG_IB_DIRTY;
}

static int
ext2_file_zero_remainder (struct vnode *vp, off_t offset)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  blksize_t blksize = fs->blksize;
  off_t off = offset % blksize;
  char *b = NULL;
  block_t block;
  int retflags;
  int ret;
  if (!off)
    return 0;

  ret = ext2_sync_file_buffer_pos (vp);
  if (ret)
    return ret;

  ret = ext2_bmap (fs, file->ino, NULL, NULL, 0, offset / blksize, &retflags,
		   &block);
  if (ret)
    return ret;
  if (!block || (retflags & BMAP_RET_UNINIT))
    return 0;

  b = malloc (blksize);
  if (UNLIKELY (!b))
    RETV_ERROR (ENOMEM, -1);
  ret = ext2_read_blocks (b, fs, block, 1);
  if (ret)
    {
      free (b);
      RETV_ERROR (EIO, -1);
    }
  memset (b + off, 0, blksize - off);
  ret = ext2_write_blocks (b, fs, block, 1);
  free (b);
  return ret;
}

static int
ext2_check_zero_block (char *buffer, blksize_t blksize)
{
  blksize_t left = blksize;
  char *ptr = buffer;
  while (left > 0)
    {
      if (*ptr++)
	return 0;
      left--;
    }
  return 1;
}

static int
ext2_dealloc_indirect_block (struct ext2_fs *fs, struct ext2_inode *inode,
			     char *blockbuf, uint32_t *p, int level,
			     block_t start, block_t count, int max)
{
  block_t offset;
  block_t inc;
  uint32_t b;
  int freed = 0;
  int i;
  int ret;

  inc = 1UL << ((EXT2_BLOCK_SIZE_BITS (fs->super) - 2) * level);
  for (i = 0, offset = 0; i < max; i++, p++, offset += inc)
    {
      if (offset >= start + count)
	break;
      if (!*p || offset + inc <= start)
	continue;
      b = *p;
      if (level > 0)
	{
	  uint32_t s;
	  ret = ext2_read_blocks (blockbuf, fs, b, 1);
	  if (ret)
	    return ret;
	  s = start > offset ? start - offset : 0;
	  ret = ext2_dealloc_indirect_block (fs, inode, blockbuf + fs->blksize,
					     (uint32_t *) blockbuf, level - 1,
					     s, count - offset,
					     fs->blksize >> 2);
	  if (ret)
	    return ret;
	  ret = ext2_write_blocks (blockbuf, fs, b, 1);
	  if (ret)
	    return ret;
	  if (!ext2_check_zero_block (blockbuf, fs->blksize))
	    continue;
	}
      ext2_block_alloc_stats (fs, b, -1);
      *p = 0;
      freed++;
    }
  return ext2_iblk_sub_blocks (fs, inode, freed);
}

static int
ext2_dealloc_indirect (struct ext2_fs *fs, struct ext2_inode *inode,
		       char *blockbuf, block_t start, block_t end)
{
  char *buffer = NULL;
  int num = EXT2_NDIR_BLOCKS;
  uint32_t *bp = inode->i_block;
  uint32_t addr_per_block;
  block_t max = EXT2_NDIR_BLOCKS;
  block_t count;
  int level;
  int ret = 0;
  if (start > 0xffffffff)
    return 0;
  if (end >= 0xffffffff || end - start + 1 >= 0xffffffff)
    count = ~start;
  else
    count = end - start + 1;

  if (!blockbuf)
    {
      buffer = malloc (fs->blksize * 3);
      if (UNLIKELY (!buffer))
	RETV_ERROR (ENOMEM, -1);
      blockbuf = buffer;
    }
  addr_per_block = fs->blksize >> 2;

  for (level = 0; level < 4; level++, max *= addr_per_block)
    {
      if (start < max)
	{
	  ret = ext2_dealloc_indirect_block (fs, inode, blockbuf, bp, level,
					     start, count, num);
	  if (ret)
	    goto end;
	  if (count > max)
	    count -= max - start;
	  else
	    break;
	  start = 0;
	}
      else
	start -= max;
      bp += num;
      if (!level)
	{
	  num = 1;
	  max = 1;
	}
    }

 end:
  if (buffer)
    free (buffer);
  return ret;
}

static int
ext2_block_iterate_ind (uint32_t *ind_block, uint32_t ref_block, int ref_offset,
			struct ext2_block_ctx *ctx)
{
  struct ext2_fs *fs = ctx->fs;
  int changed = 0;
  int flags;
  int limit;
  int offset;
  uint32_t *blockno;
  block_t block;
  int ret = 0;
  int i;

  limit = fs->blksize;
  if (!(ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE)
      && !(ctx->flags & BLOCK_FLAG_DATA_ONLY))
    {
      block = *ind_block;
      ret = ctx->func (fs, &block, BLOCK_COUNT_IND, ref_block, ref_offset,
		       ctx->private);
      *ind_block = block;
    }
  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return ret | BLOCK_ABORT | BLOCK_ERROR;
    }

  if (!*ind_block || (ret & BLOCK_ABORT))
    {
      ctx->blkcnt += limit;
      return ret;
    }
  if (*ind_block >= ext2_blocks_count (&fs->super)
      || *ind_block < fs->super.s_first_data_block)
    {
      ctx->err = -1;
      errno = EUCLEAN;
      return ret | BLOCK_ERROR;
    }
  ctx->err = ext2_read_blocks (ctx->ind_buf, fs, *ind_block, 1);
  if (ctx->err)
    {
      errno = EIO;
      return ret | BLOCK_ERROR;
    }

  blockno = (uint32_t *) ctx->ind_buf;
  offset = 0;
  if (ctx->flags & BLOCK_FLAG_APPEND)
    {
      for (i = 0; i < limit; i++, ctx->blkcnt++, blockno++)
	{
	  block = *blockno;
	  flags = ctx->func (fs, &block, ctx->blkcnt, *ind_block, offset,
			     ctx->private);
	  *blockno = block;
	  changed |= flags;
	  if (flags & BLOCK_ABORT)
	    {
	      ret |= BLOCK_ABORT;
	      break;
	    }
	  offset += 4;
	}
    }
  else
    {
      for (i = 0; i < limit; i++, ctx->blkcnt++, blockno++)
	{
	  if (!*blockno)
	    goto skip_sparse;
	  block = *blockno;
	  flags = ctx->func (fs, &block, ctx->blkcnt, *ind_block, offset,
			     ctx->private);
	  *blockno = block;
	  changed |= flags;
	  if (flags & BLOCK_ABORT)
	    {
	      ret |= BLOCK_ABORT;
	      break;
	    }
	skip_sparse:
	  offset += 4;
	}
    }

  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (changed & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return changed | BLOCK_ABORT | BLOCK_ERROR;
    }
  if (changed & BLOCK_CHANGED)
    {
      ctx->err = ext2_write_blocks (ctx->ind_buf, fs, *ind_block, 1);
      if (ctx->err)
	ret |= BLOCK_ERROR | BLOCK_ABORT;
    }
  if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE)
      && !(ctx->flags & BLOCK_FLAG_DATA_ONLY) && !(ret & BLOCK_ABORT))
    {
      block = *ind_block;
      ret |= ctx->func (fs, &block, BLOCK_COUNT_IND, ref_block, ref_offset,
			ctx->private);
      *ind_block = block;
    }
  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return ret | BLOCK_ABORT | BLOCK_ERROR;
    }
  return ret;
}

static int
ext2_block_iterate_dind (uint32_t *dind_block, uint32_t ref_block,
			 int ref_offset, struct ext2_block_ctx *ctx)
{
  struct ext2_fs *fs = ctx->fs;
  int changed = 0;
  int flags;
  int limit;
  int offset;
  uint32_t *blockno;
  block_t block;
  int ret = 0;
  int i;

  limit = fs->blksize;
  if (!(ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE)
      && !(ctx->flags & BLOCK_FLAG_DATA_ONLY))
    {
      block = *dind_block;
      ret = ctx->func (fs, &block, BLOCK_COUNT_DIND, ref_block, ref_offset,
		       ctx->private);
      *dind_block = block;
    }
  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return ret | BLOCK_ABORT | BLOCK_ERROR;
    }

  if (!*dind_block || (ret & BLOCK_ABORT))
    {
      ctx->blkcnt += limit * limit;
      return ret;
    }
  if (*dind_block >= ext2_blocks_count (&fs->super)
      || *dind_block < fs->super.s_first_data_block)
    {
      ctx->err = -1;
      errno = EUCLEAN;
      return ret | BLOCK_ERROR;
    }
  ctx->err = ext2_read_blocks (ctx->dind_buf, fs, *dind_block, 1);
  if (ctx->err)
    {
      errno = EIO;
      return ret | BLOCK_ERROR;
    }

  blockno = (uint32_t *) ctx->dind_buf;
  offset = 0;
  if (ctx->flags & BLOCK_FLAG_APPEND)
    {
      for (i = 0; i < limit; i++, ctx->blkcnt++, blockno++)
	{
	  flags = ext2_block_iterate_ind (blockno, *dind_block, offset, ctx);
	  changed |= flags;
	  if (flags & (BLOCK_ABORT | BLOCK_ERROR))
	    {
	      ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
	      break;
	    }
	  offset += 4;
	}
    }
  else
    {
      for (i = 0; i < limit; i++, ctx->blkcnt++, blockno++)
	{
	  if (!*blockno)
	    {
	      ctx->blkcnt += limit;
	      continue;
	    }
	  flags = ext2_block_iterate_ind (blockno, *dind_block, offset, ctx);
	  changed |= flags;
	  if (flags & (BLOCK_ABORT | BLOCK_ERROR))
	    {
	      ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
	      break;
	    }
	  offset += 4;
	}
    }

  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (changed & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return changed | BLOCK_ABORT | BLOCK_ERROR;
    }
  if (changed & BLOCK_CHANGED)
    {
      ctx->err = ext2_write_blocks (ctx->dind_buf, fs, *dind_block, 1);
      if (ctx->err)
	ret |= BLOCK_ERROR | BLOCK_ABORT;
    }
  if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE)
      && !(ctx->flags & BLOCK_FLAG_DATA_ONLY) && !(ret & BLOCK_ABORT))
    {
      block = *dind_block;
      ret |= ctx->func (fs, &block, BLOCK_COUNT_DIND, ref_block, ref_offset,
			ctx->private);
      *dind_block = block;
    }
  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return ret | BLOCK_ABORT | BLOCK_ERROR;
    }
  return ret;
}

static int
ext2_block_iterate_tind (uint32_t *tind_block, uint32_t ref_block,
			 int ref_offset, struct ext2_block_ctx *ctx)
{
  struct ext2_fs *fs = ctx->fs;
  int changed = 0;
  int flags;
  int limit;
  int offset;
  uint32_t *blockno;
  block_t block;
  int ret = 0;
  int i;

  limit = fs->blksize;
  if (!(ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE)
      && !(ctx->flags & BLOCK_FLAG_DATA_ONLY))
    {
      block = *tind_block;
      ret = ctx->func (fs, &block, BLOCK_COUNT_TIND, ref_block, ref_offset,
		       ctx->private);
      *tind_block = block;
    }
  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return ret | BLOCK_ABORT | BLOCK_ERROR;
    }

  if (!*tind_block || (ret & BLOCK_ABORT))
    {
      ctx->blkcnt += (blkcnt_t) limit * limit * limit;
      return ret;
    }
  if (*tind_block >= ext2_blocks_count (&fs->super)
      || *tind_block < fs->super.s_first_data_block)
    {
      ctx->err = -1;
      errno = EUCLEAN;
      return ret | BLOCK_ERROR;
    }
  ctx->err = ext2_read_blocks (ctx->tind_buf, fs, *tind_block, 1);
  if (ctx->err)
    {
      errno = EIO;
      return ret | BLOCK_ERROR;
    }

  blockno = (uint32_t *) ctx->tind_buf;
  offset = 0;
  if (ctx->flags & BLOCK_FLAG_APPEND)
    {
      for (i = 0; i < limit; i++, ctx->blkcnt++, blockno++)
	{
	  flags = ext2_block_iterate_dind (blockno, *tind_block, offset, ctx);
	  changed |= flags;
	  if (flags & (BLOCK_ABORT | BLOCK_ERROR))
	    {
	      ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
	      break;
	    }
	  offset += 4;
	}
    }
  else
    {
      for (i = 0; i < limit; i++, ctx->blkcnt++, blockno++)
	{
	  if (!*blockno)
	    {
	      ctx->blkcnt += limit * limit;
	      continue;
	    }
	  flags = ext2_block_iterate_ind (blockno, *tind_block, offset, ctx);
	  changed |= flags;
	  if (flags & (BLOCK_ABORT | BLOCK_ERROR))
	    {
	      ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
	      break;
	    }
	  offset += 4;
	}
    }

  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (changed & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return changed | BLOCK_ABORT | BLOCK_ERROR;
    }
  if (changed & BLOCK_CHANGED)
    {
      ctx->err = ext2_write_blocks (ctx->tind_buf, fs, *tind_block, 1);
      if (ctx->err)
	ret |= BLOCK_ERROR | BLOCK_ABORT;
    }
  if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE)
      && !(ctx->flags & BLOCK_FLAG_DATA_ONLY) && !(ret & BLOCK_ABORT))
    {
      block = *tind_block;
      ret |= ctx->func (fs, &block, BLOCK_COUNT_TIND, ref_block, ref_offset,
			ctx->private);
      *tind_block = block;
    }
  if ((ctx->flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx->err = -1;
      errno = EROFS;
      return ret | BLOCK_ABORT | BLOCK_ERROR;
    }
  return ret;
}

static int
ext2_read_dir_block (struct ext2_fs *fs, block_t block, void *buffer, int flags,
		     struct vnode *vp)
{
  int corrupt = 0;
  int ret = ext2_read_blocks (buffer, fs, block, 1);
  if (ret)
    return ret;
  if (!ext2_dir_block_checksum_valid (fs, vp, buffer))
    corrupt = 1;
  if (!ret && corrupt)
    RETV_ERROR (EUCLEAN, -1);
  else
    return ret;
}

static int
ext2_dirent_valid (struct ext2_fs *fs, char *buffer, unsigned int offset,
		   unsigned int final_offset)
{
  struct ext2_dirent *dirent;
  unsigned int rec_len;
  while (offset < final_offset && offset <= fs->blksize - 12)
    {
      dirent = (struct ext2_dirent *) (buffer + offset);
      if (ext2_get_rec_len (fs, dirent, &rec_len))
	return 0;
      offset += rec_len;
      if (rec_len < 8 || rec_len % 4 == 0
	  || (unsigned int) (dirent->d_name_len & 0xff) + 8 > rec_len)
	return 0;
    }
  return offset == final_offset;
}

static int
ext2_process_dir_block (struct ext2_fs *fs, block_t *blockno, blkcnt_t blkcnt,
			block_t ref_block, int ref_offset, void *private)
{
  struct ext2_dir_ctx *ctx = private;
  struct ext2_dirent *dirent;
  unsigned int next_real_entry = 0;
  unsigned int offset = 0;
  unsigned int bufsize;
  unsigned int size;
  unsigned int rec_len;
  int changed = 0;
  int csum_size = 0;
  int do_abort = 0;
  int entry;
  int inline_data;
  int ret = 0;

  entry = blkcnt ? DIRENT_OTHER_FILE : DIRENT_DOT_FILE;
  inline_data = ctx->flags & DIRENT_FLAG_INLINE ? 1 : 0;
  if (!inline_data)
    {
      ctx->err =
	ext2_read_dir_block (fs, *blockno, ctx->buffer, 0, ctx->dir);
      if (ctx->err)
	return BLOCK_ABORT;
      bufsize = fs->blksize;
    }
  else
    bufsize = ctx->bufsize;

  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM)
    csum_size = sizeof (struct ext2_dirent_tail);

  while (offset < bufsize - 8)
    {
      dirent = (struct ext2_dirent *) (ctx->buffer + offset);
      if (ext2_get_rec_len (fs, dirent, &rec_len))
	return BLOCK_ABORT;
      if (offset + rec_len > bufsize || rec_len < 8 || rec_len % 4
	  || (unsigned int) (dirent->d_name_len & 0xff) + 8 > rec_len)
	{
	  ctx->err = -1;
	  errno = EUCLEAN;
	  return BLOCK_ABORT;
	}
      if (!dirent->d_inode)
	{
	  if (!inline_data && offset == bufsize - csum_size
	      && dirent->d_rec_len == csum_size &&
	      dirent->d_name_len == EXT2_DIR_NAME_CHECKSUM)
	    {
	      if (!(ctx->flags & DIRENT_FLAG_CHECKSUM))
		goto next;
	      entry = DIRENT_CHECKSUM;
	    }
	  else if (!(ctx->flags & DIRENT_FLAG_EMPTY))
	    goto next;
	}

      ret = ctx->func (ctx->dir, next_real_entry > offset ?
		       DIRENT_DELETED_FILE : entry, dirent, offset, bufsize,
		       ctx->buffer, ctx->private);
      if (entry < DIRENT_OTHER_FILE)
	entry++;

      if (ret & DIRENT_CHANGED)
	{
	  if (ext2_get_rec_len (fs, dirent, &rec_len))
	    return BLOCK_ABORT;
	  changed++;
	}
      if (ret & DIRENT_ABORT)
	{
	  do_abort++;
	  break;
	}

    next:
      if (next_real_entry == offset)
	next_real_entry += rec_len;
      if (ctx->flags & DIRENT_FLAG_REMOVED)
	{
	  size = ((dirent->d_name_len & 0xff) + 11) & ~3;
	  if (rec_len != size)
	    {
	      unsigned int final_offset = offset + rec_len;
	      offset += size;
	      while (offset < final_offset
		     && !ext2_dirent_valid (fs, ctx->buffer, offset,
					    final_offset))
		offset += 4;
	      continue;
	    }
	}
      offset += rec_len;
    }

  if (changed)
    {
      if (!inline_data)
	{
	  ctx->err = ext2_write_dir_block (fs, *blockno, ctx->buffer, 0,
					   ctx->dir);
	  if (ctx->err)
	    return BLOCK_ABORT;
	}
      else
	ret = BLOCK_INLINE_CHANGED;
    }
  return ret | (do_abort ? BLOCK_ABORT : 0);
}

static int
ext2_process_lookup (struct vnode *dir, int entry, struct ext2_dirent *dirent,
		     int offset, blksize_t blksize, char *buffer, void *private)
{
  struct ext2_lookup_ctx *l = private;
  if (l->namelen != (dirent->d_name_len & 0xff))
    return 0;
  if (strncmp (l->name, dirent->d_name, dirent->d_name_len & 0xff))
    return 0;
  *l->inode = dirent->d_inode;
  l->found = 1;
  return DIRENT_ABORT;
}

static int
ext2_process_dir_expand (struct ext2_fs *fs, block_t *blockno, blkcnt_t blkcnt,
			 block_t ref_block, int ref_offset, void *private)
{
  struct ext2_dir_expand_ctx *e = private;
  block_t newblock;
  char *block;
  int ret;
  if (*blockno)
    {
      if (blkcnt >= 0)
	e->goal = *blockno;
      return 0;
    }

  if (blkcnt && EXT2_B2C (fs, e->goal) == EXT2_B2C (fs, e->goal + 1))
    newblock = e->goal + 1;
  else
    {
      e->goal &= ~EXT2_CLUSTER_MASK (fs);
      ret = ext2_new_block (fs, e->goal, NULL, &newblock, NULL);
      if (ret)
	{
	  e->err = ret;
	  return BLOCK_ABORT;
	}
      e->newblocks++;
      ext2_block_alloc_stats (fs, newblock, 1);
    }

  if (blkcnt > 0)
    {
      ret = ext2_new_dir_block (fs, 0, 0, &block);
      if (ret)
	{
	  e->err = ret;
	  return BLOCK_ABORT;
	}
      e->done = 1;
      ret = ext2_write_dir_block (fs, newblock, block, 0, e->dir);
      free (block);
    }
  else
    ret = ext2_zero_blocks (fs, newblock, 1, NULL, NULL);
  if (blkcnt >= 0)
    e->goal = newblock;
  if (ret)
    {
      e->err = ret;
      return BLOCK_ABORT;
    }
  *blockno = newblock;
  return e->done ? BLOCK_CHANGED | BLOCK_ABORT : BLOCK_CHANGED;
}

int
ext2_bg_has_super (struct ext2_fs *fs, unsigned int group)
{
  if (!group)
    return 1;
  if (fs->super.s_feature_ro_compat & EXT2_FT_RO_COMPAT_SPARSE_SUPER)
    return group == fs->super.s_backup_bgs[0]
      || group == fs->super.s_backup_bgs[1];
  if (group <= 1)
    return 1;
  if (!(group & 1))
    return 0;
  if (ext2_bg_super_test_root (group, 3)
      || ext2_bg_super_test_root (group, 5)
      || ext2_bg_super_test_root (group, 7))
    return 1;
  return 0;
}

int
ext2_bg_test_flags (struct ext2_fs *fs, unsigned int group, uint16_t flags)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return gdp->bg_flags & flags;
}

void
ext2_bg_clear_flags (struct ext2_fs *fs, unsigned int group, uint16_t flags)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  gdp->bg_flags &= ~flags;
}

void
ext2_update_super_revision (struct ext2_super *s)
{
  if (s->s_rev_level > EXT2_OLD_REV)
    return;
  s->s_rev_level = EXT2_DYNAMIC_REV;
  s->s_first_ino = EXT2_OLD_FIRST_INODE;
  s->s_inode_size = EXT2_OLD_INODE_SIZE;
}

struct ext2_group_desc *
ext2_group_desc (struct ext2_fs *fs, struct ext2_group_desc *gdp,
		 unsigned int group)
{
  static char *buffer;
  static size_t bufsize;
  block_t block;
  int desc_size = EXT2_DESC_SIZE (fs->super);
  int desc_per_block = EXT2_DESC_PER_BLOCK (fs->super);
  int ret;

  if (group > fs->group_desc_count)
    return NULL;
  if (gdp)
    return (struct ext2_group_desc *) ((char *) gdp + group * desc_size);

  if (bufsize < (size_t) fs->blksize)
    {
      free (buffer);
      buffer = NULL;
    }
  if (!buffer)
    {
      buffer = malloc (fs->blksize);
      if (UNLIKELY (!buffer))
	RETV_ERROR (ENOMEM, NULL);
      bufsize = fs->blksize;
    }
  block = ext2_descriptor_block (fs, fs->super.s_first_data_block,
				 group / desc_per_block);
  ret = ext2_read_blocks (buffer, fs, block, 1);
  if (ret)
    RETV_ERROR (EIO, NULL);
  return (struct ext2_group_desc *) (buffer + group %
				     desc_per_block * desc_size);
}

struct ext4_group_desc *
ext4_group_desc (struct ext2_fs *fs, struct ext2_group_desc *gdp,
		 unsigned int group)
{
  return (struct ext4_group_desc *) ext2_group_desc (fs, gdp, group);
}

void
ext2_super_bgd_loc (struct ext2_fs *fs, unsigned int group, block_t *super,
		    block_t *old_desc, block_t *new_desc, blkcnt_t *used)
{
  block_t group_block;
  block_t super_block = 0;
  block_t old_desc_block = 0;
  block_t new_desc_block = 0;
  unsigned int meta_bg;
  size_t meta_bg_size;
  blkcnt_t nblocks = 0;
  block_t old_desc_blocks;
  int has_super;

  group_block = ext2_group_first_block (fs, group);
  if (!group_block && fs->blksize == 1024)
    group_block = 1;

  if (fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_META_BG)
    old_desc_blocks = fs->super.s_first_meta_bg;
  else
    old_desc_blocks = fs->desc_blocks + fs->super.s_reserved_gdt_blocks;

  has_super = ext2_bg_has_super (fs, group);
  if (has_super)
    {
      super_block = group_block;
      nblocks++;
    }
  meta_bg_size = EXT2_DESC_PER_BLOCK (fs->super);
  meta_bg = group / meta_bg_size;

  if (!(fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_META_BG)
      || meta_bg < fs->super.s_first_meta_bg)
    {
      if (has_super)
	{
	  old_desc_block = group_block + 1;
	  nblocks += old_desc_blocks;
	}
    }
  else
    {
      if (group % meta_bg_size == 0
	  || group % meta_bg_size == 1
	  || group % meta_bg_size == meta_bg_size - 1)
	{
	  new_desc_block = group_block + (has_super ? 1 : 0);
	  nblocks++;
	}
    }

  if (super)
    *super = super_block;
  if (old_desc)
    *old_desc = old_desc_block;
  if (new_desc)
    *new_desc = new_desc_block;
  if (used)
    *used = nblocks;
}

block_t
ext2_block_bitmap_loc (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return (block_t) gdp->bg_block_bitmap |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (block_t) gdp->bg_block_bitmap_hi << 32 : 0);
}

block_t
ext2_inode_bitmap_loc (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return (block_t) gdp->bg_inode_bitmap |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (block_t) gdp->bg_inode_bitmap_hi << 32 : 0);
}

block_t
ext2_inode_table_loc (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return (block_t) gdp->bg_inode_table |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (block_t) gdp->bg_inode_table_hi << 32 : 0);
}

block_t
ext2_descriptor_block (struct ext2_fs *fs, block_t group_block, unsigned int i)
{
  int has_super = 0;
  int group_zero_adjust = 0;
  int bg;
  block_t block;

  if (!i && fs->blksize == 1024 && EXT2_CLUSTER_RATIO (fs) > 1)
    group_zero_adjust = 1;

  if (!(fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_META_BG)
      || i < fs->super.s_first_meta_bg)
    return group_block + group_zero_adjust + i + 1;

  bg = EXT2_DESC_PER_BLOCK (fs->super) * i;
  if (ext2_bg_has_super (fs, bg))
    has_super = 1;
  block = ext2_group_first_block (fs, bg);

  if (group_block != fs->super.s_first_data_block
      && block + has_super + fs->super.s_blocks_per_group <
      (block_t) ext2_blocks_count (&fs->super))
    {
      block += fs->super.s_blocks_per_group;
      if (ext2_bg_has_super (fs, bg + 1))
	has_super = 1;
      else
	has_super = 0;
    }
  return block + has_super + group_zero_adjust;
}

uint32_t
ext2_bg_free_blocks_count (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return gdp->bg_free_blocks_count |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (uint32_t) gdp->bg_free_blocks_count_hi << 16 : 0);
}

void
ext2_bg_free_blocks_count_set (struct ext2_fs *fs, unsigned int group,
			       uint32_t blocks)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  gdp->bg_free_blocks_count = blocks;
  if (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    gdp->bg_free_blocks_count_hi = blocks >> 16;
}

uint32_t
ext2_bg_free_inodes_count (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return gdp->bg_free_inodes_count |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (uint32_t) gdp->bg_free_inodes_count_hi << 16 : 0);
}

void
ext2_bg_free_inodes_count_set (struct ext2_fs *fs, unsigned int group,
			       uint32_t inodes)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  gdp->bg_free_inodes_count = inodes;
  if (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    gdp->bg_free_inodes_count_hi = inodes >> 16;
}

uint32_t
ext2_bg_used_dirs_count (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return gdp->bg_used_dirs_count |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (uint32_t) gdp->bg_used_dirs_count_hi << 16 : 0);
}

void
ext2_bg_used_dirs_count_set (struct ext2_fs *fs, unsigned int group,
			     uint32_t dirs)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  gdp->bg_used_dirs_count = dirs;
  if (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    gdp->bg_used_dirs_count_hi = dirs >> 16;
}

uint32_t
ext2_bg_itable_unused (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return gdp->bg_itable_unused |
    (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (uint32_t) gdp->bg_itable_unused_hi << 16 : 0);
}

void
ext2_bg_itable_unused_set (struct ext2_fs *fs, unsigned int group,
			   uint32_t unused)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  gdp->bg_itable_unused = unused;
  if (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    gdp->bg_itable_unused_hi = unused >> 16;
}

void
ext2_cluster_alloc (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		    struct ext3_extent_handle *handle, block_t block,
		    block_t *physblock)
{
  block_t pblock = 0;
  block_t base;
  int i;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_BIGALLOC))
    return;

  base = block & ~EXT2_CLUSTER_MASK (fs);
  for (i = 0; i < EXT2_CLUSTER_RATIO (fs); i++)
    {
      if (base + i == block)
	continue;
      ext3_extent_bmap (fs, ino, inode, handle, 0, 0, base + i, 0, 0, &pblock);
      if (pblock)
	break;
    }
  if (pblock)
    *physblock = pblock - i + block - base;
}

int
ext2_map_cluster_block (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
			block_t block, block_t *physblock)
{
  struct ext3_extent_handle *handle;
  int ret;
  *physblock = 0;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_BIGALLOC)
      || !(inode->i_flags & EXT4_EXTENTS_FL))
    return 0;

  ret = ext3_extent_open (fs, ino, inode, &handle);
  if (ret)
    return ret;

  ext2_cluster_alloc (fs, ino, inode, handle, block, physblock);
  ext3_extent_free (handle);
  return 0;
}

void
ext2_block_alloc_stats (struct ext2_fs *fs, block_t block, int inuse)
{
  unsigned int group = ext2_group_of_block (fs, block);
  if (block > (block_t) ext2_blocks_count (&fs->super))
    return;
  if (inuse > 0)
    ext2_mark_bitmap (fs->block_bitmap, block);
  else
    ext2_unmark_bitmap (fs->block_bitmap, block);
  ext2_bg_free_blocks_count_set (fs, group,
				 ext2_bg_free_blocks_count (fs, group) - inuse);
  ext2_bg_clear_flags (fs, group, EXT2_BG_BLOCK_UNINIT);
  ext2_group_desc_checksum_update (fs, group);
  ext2_free_blocks_count_add (&fs->super,
			      -inuse * (blkcnt_t) EXT2_CLUSTER_RATIO (fs));
  fs->flags |= EXT2_FLAG_CHANGED | EXT2_FLAG_DIRTY | EXT2_FLAG_BB_DIRTY;
}

void
ext2_inode_alloc_stats (struct ext2_fs *fs, ino_t ino, int inuse, int isdir)
{
  unsigned int group = ext2_group_of_inode (fs, ino);
  if (ino > fs->super.s_inodes_count)
    return;
  if (inuse > 0)
    ext2_mark_bitmap (fs->inode_bitmap, ino);
  else
    ext2_unmark_bitmap (fs->inode_bitmap, ino);
  ext2_bg_free_inodes_count_set (fs, group,
				 ext2_bg_free_inodes_count (fs, group) - inuse);
  if (isdir)
    ext2_bg_used_dirs_count_set (fs, group,
				 ext2_bg_used_dirs_count (fs, group) + inuse);
  ext2_bg_clear_flags (fs, group, EXT2_BG_INODE_UNINIT);
  if (ext2_has_group_desc_checksum (&fs->super))
    {
      ino_t first_unused_inode =
	fs->super.s_inodes_per_group - ext2_bg_itable_unused (fs, group) +
	group * fs->super.s_inodes_per_group + 1;
      if (ino >= first_unused_inode)
	ext2_bg_itable_unused_set (fs, group,
				   group * fs->super.s_inodes_per_group +
				   fs->super.s_inodes_per_group - ino);
      ext2_group_desc_checksum_update (fs, group);
    }
  fs->super.s_free_inodes_count -= inuse;
  fs->flags |= EXT2_FLAG_CHANGED | EXT2_FLAG_DIRTY | EXT2_FLAG_IB_DIRTY;
}

int
ext2_write_backup_superblock (struct ext2_fs *fs, unsigned int group,
			      block_t group_block, struct ext2_super *s)
{
  unsigned int sgroup = group;
  if (sgroup > 65535)
    sgroup = 65535;
  s->s_block_group_nr = sgroup;
  ext2_superblock_checksum_update (fs, s);
  if (fs->device->write (fs->device, s, sizeof (struct ext2_super),
			 group_block * fs->blksize, 1)
      == sizeof (struct ext2_super))
    return 0;
  else
    RETV_ERROR (EIO, -1);
}

int
ext2_write_primary_superblock (struct ext2_fs *fs, struct ext2_super *s)
{
  if (fs->device->write (fs->device, s, sizeof (struct ext2_super),
			 EXT2_SUPER_OFFSET, 1) == sizeof (struct ext2_super))
    return 0;
  else
    RETV_ERROR (EIO, -1);
}

int
ext2_flush_fs (struct ext2_fs *fs, int flags)
{
  unsigned int state;
  unsigned int i;
  struct ext2_super *super_shadow = NULL;
  struct ext2_group_desc *group_shadow = NULL;
  char *group_ptr;
  block_t old_desc_blocks;
  int ret;
  if (fs->super.s_magic != EXT2_MAGIC)
    RETV_ERROR (EUCLEAN, -1);
  if (!(fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_JOURNAL_DEV)
      && !fs->group_desc)
    RETV_ERROR (EUCLEAN, -1);

  state = fs->super.s_state;
  fs->super.s_wtime = time (NULL);
  fs->super.s_block_group_nr = 0;
  fs->super.s_state &= ~EXT2_STATE_VALID;
  fs->super.s_feature_incompat &= ~EXT3_FT_INCOMPAT_RECOVER;

  ret = ext2_write_bitmaps (fs);
  if (ret)
    return ret;

  super_shadow = &fs->super;
  group_shadow = fs->group_desc;

  if (fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_JOURNAL_DEV)
    goto write_super;

  group_ptr = (char *) group_shadow;
  if (fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_META_BG)
    {
      old_desc_blocks = fs->super.s_first_meta_bg;
      if (old_desc_blocks > fs->desc_blocks)
	old_desc_blocks = fs->desc_blocks;
    }
  else
    old_desc_blocks = fs->desc_blocks;

  for (i = 0; i < fs->group_desc_count; i++)
    {
      block_t super_block;
      block_t old_desc_block;
      block_t new_desc_block;
      ext2_super_bgd_loc (fs, i, &super_block, &old_desc_block, &new_desc_block,
			  NULL);
      if (i > 0 && super_block)
	{
	  ret = ext2_write_backup_superblock (fs, i, super_block, super_shadow);
	  if (ret)
	    return ret;
	}
      if (old_desc_block)
	{
	  ret = ext2_write_blocks (group_ptr, fs, old_desc_block,
				   old_desc_blocks);
	  if (ret)
	    return ret;
	}
      if (new_desc_block)
	{
	  int meta_bg = i / EXT2_DESC_PER_BLOCK (fs->super);
	  ret = ext2_write_blocks (group_ptr + meta_bg * fs->blksize, fs,
				   new_desc_block, 1);
	  if (ret)
	    return ret;
	}
    }

 write_super:
  fs->super.s_block_group_nr = 0;
  fs->super.s_state = state;
  if (flags & FLUSH_VALID)
    fs->super.s_state |= EXT2_STATE_VALID;
  ext2_superblock_checksum_update (fs, super_shadow);
  ret = ext2_write_primary_superblock (fs, super_shadow);
  if (ret)
    return ret;
  fs->flags &= ~EXT2_FLAG_DIRTY;
  return 0;
}

int
ext2_open_file (struct ext2_fs *fs, ino_t inode, struct ext2_file *file)
{
  int ret = ext2_read_inode (fs, inode, &file->inode);
  if (ret)
    return ret;
  file->ino = inode;
  file->flags = 0;
  file->buffer = malloc (fs->blksize * 3);
  if (UNLIKELY (!file->buffer))
    RETV_ERROR (ENOMEM, -1);
  return 0;
}

int
ext2_file_block_offset_too_big (struct ext2_fs *fs, struct ext2_inode *inode,
				block_t offset)
{
  block_t addr_per_block;
  block_t max_map_block;

  if (offset >= (1UL << 32) - 1)
    return 1;
  if (inode->i_flags & EXT4_EXTENTS_FL)
    return 0;

  addr_per_block = fs->blksize >> 2;
  max_map_block = addr_per_block;
  max_map_block += addr_per_block * addr_per_block;
  max_map_block += addr_per_block * addr_per_block * addr_per_block;
  max_map_block += EXT2_NDIR_BLOCKS;
  return offset >= max_map_block;
}

int
ext2_file_set_size (struct vnode *vp, off_t size)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  blksize_t blksize = fs->blksize;
  block_t old_truncate;
  block_t truncate_block;
  off_t old_size;
  int ret;
  if (size > 0 && ext2_file_block_offset_too_big (fs, &file->inode,
						  (size - 1) / blksize))
    RETV_ERROR (EFBIG, -1);

  truncate_block = (size + blksize - 1) >> EXT2_BLOCK_SIZE_BITS (fs->super);
  old_size = EXT2_I_SIZE (file->inode);
  old_truncate = (old_size + blksize - 1) >> EXT2_BLOCK_SIZE_BITS (fs->super);

  ret = ext2_inode_set_size (fs, &file->inode, size);
  if (ret)
    return ret;

  if (file->ino)
    {
      ret = ext2_update_inode (fs, file->ino, &file->inode,
			       sizeof (struct ext2_inode));
      if (ret)
	return ret;
    }

  ret = ext2_file_zero_remainder (vp, size);
  if (ret)
    return ret;

  if (truncate_block >= old_truncate)
    return 0;
  return ext2_dealloc_blocks (fs, file->ino, &file->inode, 0,
			      truncate_block, ~0UL);
}

int
ext2_read_inode (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode)
{
  int len = EXT2_INODE_SIZE (fs->super);
  size_t bufsize = sizeof (struct ext2_inode);
  struct ext2_large_inode *iptr;
  char *ptr;
  block_t blockno;
  unsigned int group;
  unsigned long block;
  unsigned long offset;
  unsigned int i;
  int cache_slot;
  int csum_valid;
  int clen;
  int ret;

  if (UNLIKELY (!ino || ino > fs->super.s_inodes_count))
    RETV_ERROR (EINVAL, -1);

  /* Try lookup in inode cache */
  if (!fs->icache)
    {
      ret = ext2_create_inode_cache (fs, 4);
      if (ret)
	return ret;
    }
  for (i = 0; i < fs->icache->cache_size; i++)
    {
      if (fs->icache->cache[i].ino == ino)
	{
	  memcpy (inode, fs->icache->cache[i].inode,
		  bufsize > (size_t) len ? (size_t) len : bufsize);
	  return 0;
	}
    }

  group = (ino - 1) / fs->super.s_inodes_per_group;
  if (group > fs->group_desc_count)
    RETV_ERROR (EINVAL, -1);
  offset = (ino - 1) % fs->super.s_inodes_per_group *
    EXT2_INODE_SIZE (fs->super);
  block = offset >> EXT2_BLOCK_SIZE_BITS (fs->super);
  blockno = ext2_inode_table_loc (fs, group);
  if (!blockno || blockno < fs->super.s_first_data_block
      || blockno + fs->inode_blocks_per_group - 1 >=
      (block_t) ext2_blocks_count (&fs->super))
    RETV_ERROR (EUCLEAN, -1);
  blockno += block;
  offset &= EXT2_BLOCK_SIZE (fs->super) - 1;

  cache_slot = (fs->icache->cache_last + 1) % fs->icache->cache_size;
  iptr = (struct ext2_large_inode *) fs->icache->cache[cache_slot].inode;
  ptr = (char *) iptr;
  while (len)
    {
      clen = len;
      if (offset + len > (unsigned long) fs->blksize)
	clen = fs->blksize - offset;
      if (blockno != fs->icache->block)
	{
	  ret = ext2_read_blocks (fs->icache->buffer, fs, blockno, 1);
	  if (ret)
	    return ret;
	  fs->icache->block = blockno;
	}
      memcpy (ptr, fs->icache->buffer + (unsigned int) offset, clen);
      offset = 0;
      len -= clen;
      ptr += clen;
      blockno++;
    }
  len = EXT2_INODE_SIZE (fs->super);

  csum_valid = ext2_inode_checksum_valid (fs, ino, iptr);
  if (csum_valid)
    {
      fs->icache->cache_last = cache_slot;
      fs->icache->cache[cache_slot].ino = ino;
    }
  memcpy (inode, iptr, bufsize > (size_t) len ? (size_t) len : bufsize);
  if (csum_valid)
    return 0;
  else
    RETV_ERROR (EUCLEAN, -1);
}

int
ext2_update_inode (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		   size_t bufsize)
{
  int len = EXT2_INODE_SIZE (fs->super);
  struct ext2_large_inode *winode;
  block_t blockno;
  unsigned long block;
  unsigned long offset;
  unsigned int group;
  char *ptr;
  unsigned int i;
  int clen;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);

  winode = calloc (len, 1);
  if (UNLIKELY (!winode))
    RETV_ERROR (ENOMEM, -1);
  if (bufsize < (size_t) len)
    {
      ret = ext2_read_inode (fs, ino, (struct ext2_inode *) winode);
      if (ret)
	{
	  free (winode);
	  return ret;
	}
    }

  /* Update inode cache if necessary */
  if (fs->icache)
    {
      for (i = 0; i < fs->icache->cache_size; i++)
	{
	  if (fs->icache->cache[i].ino == ino)
	    {
	      memcpy (fs->icache->cache[i].inode, inode,
		      bufsize > (size_t) len ? (size_t) len : bufsize);
	      break;
	    }
	}
    }
  else
    {
      ret = ext2_create_inode_cache (fs, 4);
      if (ret)
	{
	  free (winode);
	  return ret;
	}
    }
  memcpy (winode, inode, bufsize > (size_t) len ? (size_t) len : bufsize);
  ext2_inode_checksum_update (fs, ino, winode);

  group = (ino - 1) / fs->super.s_inodes_per_group;
  offset = (ino - 1) % fs->super.s_inodes_per_group *
    EXT2_INODE_SIZE (fs->super);
  block = offset >> EXT2_BLOCK_SIZE_BITS (fs->super);
  blockno = ext2_inode_table_loc (fs, group);
  if (!blockno || blockno < fs->super.s_first_data_block
      || blockno + fs->inode_blocks_per_group - 1 >=
      (block_t) ext2_blocks_count (&fs->super))
    {
      free (winode);
      RETV_ERROR (EUCLEAN, -1);
    }
  blockno += block;
  offset &= EXT2_BLOCK_SIZE (fs->super) - 1;

  ptr = (char *) winode;
  while (len)
    {
      clen = len;
      if (offset + len > (unsigned long) fs->blksize)
	clen = fs->blksize - offset;
      if (fs->icache->block != blockno)
	{
	  ret = ext2_read_blocks (fs->icache->buffer, fs, blockno, 1);
	  if (ret)
	    {
	      free (winode);
	      return ret;
	    }
	  fs->icache->block = blockno;
	}
      memcpy (fs->icache->buffer + (unsigned int) offset, ptr, clen);
      ret = ext2_write_blocks (fs->icache->buffer, fs, blockno, 1);
      if (ret)
	{
	  free (winode);
	  return ret;
	}
      offset = 0;
      ptr += clen;
      len -= clen;
      blockno++;
    }
  fs->flags |= EXT2_FLAG_CHANGED;
  free (winode);
  return 0;
}

void
ext2_update_vfs_inode (struct vnode *vp)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  vp->mode = file->inode.i_mode & S_IFMT;
  vp->nlink = file->inode.i_links_count;
  vp->uid = file->inode.i_uid;
  vp->gid = file->inode.i_gid;
  if (S_ISBLK (vp->mode) || S_ISCHR (vp->mode))
    vp->rdev = *((dev_t *) file->inode.i_block);
  else
    vp->rdev = 0;
  vp->atime.tv_sec = file->inode.i_atime;
  vp->mtime.tv_sec = file->inode.i_mtime;
  vp->ctime.tv_sec = file->inode.i_ctime;
  vp->atime.tv_nsec = vp->mtime.tv_nsec = vp->ctime.tv_nsec = 0;
  vp->blocks = (file->inode.i_blocks * 512 + fs->blksize - 1) / fs->blksize;
  vp->blksize = fs->blksize;
  vp->size = file->inode.i_size;
  if (fs->super.s_feature_ro_compat & EXT2_FT_RO_COMPAT_LARGE_FILE)
    vp->size |= (size_t) file->inode.i_size_high << 32;
}

int
ext2_inode_set_size (struct ext2_fs *fs, struct ext2_inode *inode, off_t size)
{
  if (size < 0)
    RETV_ERROR (EINVAL, -1);
  if (ext2_needs_large_file (size))
    {
      int dirty_sb = 0;
      if (S_ISREG (inode->i_mode))
	{
	  if (!(fs->super.s_feature_ro_compat & EXT2_FT_RO_COMPAT_LARGE_FILE))
	    {
	      fs->super.s_feature_ro_compat |= EXT2_FT_RO_COMPAT_LARGE_FILE;
	      dirty_sb = 1;
	    }
	}
      else if (S_ISDIR (inode->i_mode))
	{
	  if (!(fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_LARGEDIR))
	    {
	      fs->super.s_feature_incompat |= EXT4_FT_INCOMPAT_LARGEDIR;
	      dirty_sb = 1;
	    }
	}
      else
	RETV_ERROR (EFBIG, -1);

      if (dirty_sb)
	{
	  if (fs->super.s_rev_level == EXT2_OLD_REV)
	    ext2_update_super_revision (&fs->super);
	  fs->flags |= EXT2_FLAG_DIRTY | EXT2_FLAG_CHANGED;
	}
    }

  inode->i_size = size & 0xffffffff;
  inode->i_size_high = size >> 32;
  return 0;
}

block_t
ext2_find_inode_goal (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		      block_t block)
{
  unsigned int group;
  unsigned char log_flex;
  struct ext3_generic_extent extent;
  struct ext3_extent_handle *handle = NULL;
  int ret;

  if (!inode || ext2_is_inline_symlink (inode)
      || (inode->i_flags & EXT4_INLINE_DATA_FL))
    goto noblocks;
  if (inode->i_flags & EXT4_EXTENTS_FL)
    {
      ret = ext3_extent_open (fs, ino, inode, &handle);
      if (ret)
	goto noblocks;
      ret = ext3_extent_goto (handle, 0, block);
      if (ret)
	goto noblocks;
      ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
      if (ret)
	goto noblocks;
      ext3_extent_free (handle);
      return extent.e_pblk + block - extent.e_lblk;
    }

  if (inode->i_block[0])
    return inode->i_block[0];

 noblocks:
  ext3_extent_free (handle);
  log_flex = fs->super.s_log_groups_per_flex;
  group = ext2_group_of_inode (fs, ino);
  if (log_flex)
    group &= ~((1 << log_flex) - 1);
  return ext2_group_first_block (fs, group);
}

int
ext2_create_inode_cache (struct ext2_fs *fs, unsigned int cache_size)
{
  unsigned int i;
  if (fs->icache)
    return 0;

  fs->icache = malloc (sizeof (struct ext2_inode_cache));
  if (UNLIKELY (!fs->icache))
    RETV_ERROR (ENOMEM, -1);
  fs->icache->buffer = malloc (fs->blksize);
  if (UNLIKELY (!fs->icache->buffer))
    goto err;

  fs->icache->block = 0;
  fs->icache->cache_last = -1;
  fs->icache->cache_size = cache_size;
  fs->icache->refcnt = 1;

  fs->icache->cache =
    malloc (sizeof (struct ext2_inode_cache_entry) * cache_size);
  if (UNLIKELY (!fs->icache->cache))
    goto err;

  for (i = 0; i < cache_size; i++)
    {
      fs->icache->cache[i].inode = malloc (EXT2_INODE_SIZE (fs->super));
      if (UNLIKELY (!fs->icache->cache[i].inode))
	goto err;
    }
  ext2_flush_inode_cache (fs->icache);
  return 0;

 err:
  ext2_free_inode_cache (fs->icache);
  fs->icache = NULL;
  RETV_ERROR (ENOMEM, -1);
}

void
ext2_free_inode_cache (struct ext2_inode_cache *cache)
{
  unsigned int i;
  if (--cache->refcnt > 0)
    return;
  if (cache->buffer)
    free (cache->buffer);
  for (i = 0; i < cache->cache_size; i++)
    free (cache->cache[i].inode);
  if (cache->cache)
    free (cache->cache);
  cache->block = 0;
  free (cache);
}

int
ext2_flush_inode_cache (struct ext2_inode_cache *cache)
{
  unsigned int i;
  if (!cache)
    return 0;
  for (i = 0; i < cache->cache_size; i++)
    cache->cache[i].ino = 0;
  cache->block = 0;
  return 0;
}

int
ext2_file_flush (struct vnode *vp)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  block_t ignore;
  int retflags;
  int ret;
  if (!(file->flags & (EXT2_FILE_BUFFER_VALID | EXT2_FILE_BUFFER_DIRTY)))
    return 0;
  if (file->physblock && (file->inode.i_flags & EXT4_EXTENTS_FL))
    {
      ret = ext2_bmap (fs, file->ino, &file->inode, file->buffer + fs->blksize,
		       0, file->block, &retflags, &ignore);
      if (ret)
	return ret;
      if (retflags & BMAP_RET_UNINIT)
	{
	  ret = ext2_bmap (fs, file->ino, &file->inode,
			   file->buffer + fs->blksize, BMAP_SET, file->block,
			   0, &file->physblock);
	  if (ret)
	    return ret;
	}
    }

  if (!file->physblock)
    {
      ret = ext2_bmap (fs, file->ino, &file->inode,
		       file->buffer + fs->blksize, file->ino ? BMAP_ALLOC : 0,
		       file->block, 0, &file->physblock);
      if (ret)
	return ret;
    }

  ret = ext2_write_blocks (file->buffer, fs, file->physblock, 1);
  if (ret)
    return ret;
  file->flags &= ~EXT2_FILE_BUFFER_DIRTY;
  return ret;
}

int
ext2_sync_file_buffer_pos (struct vnode *vp)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  block_t block = file->pos / fs->blksize;
  int ret;
  if (block != file->block)
    {
      ret = ext2_file_flush (vp);
      if (ret)
	return ret;
      file->flags &= ~EXT2_FILE_BUFFER_VALID;
    }
  file->block = block;
  return 0;
}

int
ext2_load_file_buffer (struct vnode *vp, int nofill)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  int retflags;
  int ret;
  if (!(file->flags & EXT2_FILE_BUFFER_VALID))
    {
      ret = ext2_bmap (fs, file->ino, &file->inode,
		       file->buffer + fs->blksize, 0, file->block, &retflags,
		       &file->physblock);
      if (ret)
	return ret;
      if (!nofill)
	{
	  if (file->physblock && !(retflags & BMAP_RET_UNINIT))
	    {
	      ret = ext2_read_blocks (file->buffer, fs, file->physblock, 1);
	      if (ret)
		return ret;
	    }
	  else
	    memset (file->buffer, 0, fs->blksize);
	}
      file->flags |= EXT2_FILE_BUFFER_VALID;
    }
  return 0;
}

int
ext2_iblk_add_blocks (struct ext2_fs *fs, struct ext2_inode *inode,
		      block_t nblocks)
{
  blkcnt_t b = inode->i_blocks;
  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
    b += (blkcnt_t) inode->osd2.linux2.l_i_blocks_hi << 32;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
      || !(inode->i_flags & EXT4_HUGE_FILE_FL))
    nblocks *= fs->blksize / 512;
  nblocks *= EXT2_CLUSTER_RATIO (fs);
  b += nblocks;
  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
    inode->osd2.linux2.l_i_blocks_hi = b >> 32;
  else if (b > 0xffffffff)
    RETV_ERROR (EOVERFLOW, -1);
  inode->i_blocks = b & 0xffffffff;
  return 0;
}

int
ext2_iblk_sub_blocks (struct ext2_fs *fs, struct ext2_inode *inode,
		      block_t nblocks)
{
  blkcnt_t b = inode->i_blocks;
  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
    b += (blkcnt_t) inode->osd2.linux2.l_i_blocks_hi << 32;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
      || !(inode->i_flags & EXT4_HUGE_FILE_FL))
    nblocks *= fs->blksize / 512;
  nblocks *= EXT2_CLUSTER_RATIO (fs);
  if (nblocks > (block_t) b)
    RETV_ERROR (EOVERFLOW, -1);
  b -= nblocks;
  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
    inode->osd2.linux2.l_i_blocks_hi = b >> 32;
  inode->i_blocks = b & 0xffffffff;
  return 0;
}

int
ext2_iblk_set (struct ext2_fs *fs, struct ext2_inode *inode, block_t nblocks)
{
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
      || !(inode->i_flags & EXT4_HUGE_FILE_FL))
    nblocks *= fs->blksize / 512;
  nblocks *= EXT2_CLUSTER_RATIO (fs);
  inode->i_blocks = nblocks & 0xffffffff;
  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_HUGE_FILE)
    inode->osd2.linux2.l_i_blocks_hi = nblocks >> 32;
  else if (nblocks >> 32)
    RETV_ERROR (EOVERFLOW, -1);
  return 0;
}

int
ext2_zero_blocks (struct ext2_fs *fs, block_t block, int num, block_t *result,
		  int *count)
{
  static void *buffer;
  static int stride_len;
  int c;
  int i;
  int ret;
  if (!fs)
    {
      if (buffer)
	{
	  free (buffer);
	  buffer = NULL;
	  stride_len = 0;
	}
      return 0;
    }
  if (num <= 0)
    return 0;

  if (num > stride_len && stride_len < EXT2_MAX_STRIDE_LENGTH (fs->super))
    {
      void *ptr;
      int new_stride = num;
      if (new_stride > EXT2_MAX_STRIDE_LENGTH (fs->super))
	new_stride = EXT2_MAX_STRIDE_LENGTH (fs->super);
      ptr = malloc (fs->blksize * new_stride);
      if (UNLIKELY (!ptr))
	RETV_ERROR (ENOMEM, -1);
      free (buffer);
      buffer = ptr;
      stride_len = new_stride;
      memset (buffer, 0, fs->blksize * stride_len);
    }

  i = 0;
  while (i < num)
    {
      if (block % stride_len)
	{
	  c = stride_len - block % stride_len;
	  if (c > num - i)
	    c = num - i;
	}
      else
	{
	  c = num - i;
	  if (c > stride_len)
	    c = stride_len;
	}
      ret = ext2_write_blocks (buffer, fs, block, c);
      if (ret)
	{
	  if (count)
	    *count = c;
	  if (result)
	    *result = block;
	  return ret;
	}
      i += c;
      block += c;
    }
  return 0;
}

int
ext2_new_dir_block (struct ext2_fs *fs, ino_t ino, ino_t parent, char **block)
{
  struct ext2_dirent *dir = NULL;
  char *buffer;
  int rec_len;
  int filetype = 0;
  int csum_size = 0;
  struct ext2_dirent_tail *t;
  int ret;

  buffer = calloc (1, fs->blksize);
  if (UNLIKELY (!buffer))
    RETV_ERROR (ENOMEM, -1);
  dir = (struct ext2_dirent *) buffer;

  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM)
    csum_size = sizeof (struct ext2_dirent_tail);

  ret = ext2_set_rec_len (fs, fs->blksize - csum_size, dir);
  if (ret)
    {
      free (buffer);
      return ret;
    }

  if (ino)
    {
      if (fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_FILETYPE)
	filetype = EXT2_FILE_DIR;

      /* Setup `.' */
      dir->d_inode = ino;
      dir->d_name_len = (filetype << 8) | 1;
      dir->d_name[0] = '.';
      rec_len = fs->blksize - csum_size - ext2_dir_rec_len (1, 0);
      dir->d_rec_len = ext2_dir_rec_len (1, 0);

      /* Setup `..' */
      dir = (struct ext2_dirent *) (buffer + dir->d_rec_len);
      ret = ext2_set_rec_len (fs, rec_len, dir);
      if (ret)
	{
	  free (buffer);
	  return ret;
	}
      dir->d_inode = parent;
      dir->d_name_len = (filetype << 8) | 2;
      dir->d_name[0] = dir->d_name[1] = '.';
    }

  if (csum_size > 0)
    {
      t = EXT2_DIRENT_TAIL (buffer, fs->blksize);
      ext2_init_dirent_tail (fs, t);
    }
  *block = buffer;
  return 0;
}

int
ext2_write_dir_block (struct ext2_fs *fs, block_t block, char *buffer,
		      int flags, struct vnode *vp)
{
  int ret =
    ext2_dir_block_checksum_update (fs, vp, (struct ext2_dirent *) buffer);
  if (ret)
    return ret;
  return ext2_write_blocks (buffer, fs, block, 1);
}

int
ext2_new_block (struct ext2_fs *fs, block_t goal, struct ext2_bitmap *map,
		block_t *result, struct ext2_balloc_ctx *ctx)
{
  block_t b = 0;
  int ret;
  if (!map)
    map = fs->block_bitmap;
  if (!map)
    RETV_ERROR (EINVAL, -1);

  if (!goal || goal >= (block_t) ext2_blocks_count (&fs->super))
    goal &= ~EXT2_CLUSTER_MASK (fs);

  ret = ext2_find_first_zero_bitmap (map, goal,
				     ext2_blocks_count (&fs->super) - 1, &b);
  if (ret == -1 && errno == ENOENT && goal != fs->super.s_first_data_block)
    ret = ext2_find_first_zero_bitmap (map, fs->super.s_first_data_block,
				       goal - 1, &b);
  if (ret)
    return ret;
  ext2_clear_block_uninit (fs, ext2_group_of_block (fs, b));
  *result = b;
  return 0;
}

int
ext2_new_inode (struct ext2_fs *fs, ino_t dir, struct ext2_bitmap *map,
		ino_t *result)
{
  ino_t start_inode = 0;
  ino_t ino_in_group;
  ino_t upto;
  ino_t first_zero;
  ino_t i;
  unsigned int group;
  int ret;
  if (!map)
    map = fs->inode_bitmap;
  if (!map)
    RETV_ERROR (EINVAL, -1);

  if (dir > 0)
    {
      group = (dir - 1) / fs->super.s_inodes_per_group;
      start_inode = group * fs->super.s_inodes_per_group + 1;
    }

  if (start_inode < EXT2_FIRST_INODE (fs->super))
    start_inode = EXT2_FIRST_INODE (fs->super);
  if (start_inode > fs->super.s_inodes_count)
    RETV_ERROR (ENOSPC, -1);

  i = start_inode;
  do
    {
      ino_in_group = (i - 1) % fs->super.s_inodes_per_group;
      group = (i - 1) / fs->super.s_inodes_per_group;

      ext2_check_inode_uninit (fs, map, group);
      upto = i + fs->super.s_inodes_per_group - ino_in_group;

      if (i < start_inode && upto >= start_inode)
	upto = start_inode - 1;
      if (upto > fs->super.s_inodes_count)
	upto = fs->super.s_inodes_count;

      ret = ext2_find_first_zero_bitmap (map, i, upto, &first_zero);
      if (!ret)
	{
	  i = first_zero;
	  break;
	}
      else if (errno != ENOENT)
	RETV_ERROR (ENOSPC, -1);

      i = upto + 1;
      if (i > fs->super.s_inodes_count)
	i = EXT2_FIRST_INODE (fs->super);
    }
  while (i != start_inode);

  if (ext2_test_bitmap (map, i))
    RETV_ERROR (ENOSPC, -1);
  *result = i;
  return 0;
}

int
ext2_write_new_inode (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode)
{
  struct ext2_inode *buffer;
  struct ext2_large_inode *large;
  size_t size = EXT2_INODE_SIZE (fs->super);
  time_t t = time (NULL);
  int ret;
  if (!inode->i_ctime)
    inode->i_ctime = t;
  if (!inode->i_mtime)
    inode->i_mtime = t;
  if (!inode->i_atime)
    inode->i_atime = t;

  if (size == sizeof (struct ext2_inode))
    return ext2_update_inode (fs, ino, inode, sizeof (struct ext2_inode));

  buffer = malloc (size);
  if (UNLIKELY (!buffer))
    RETV_ERROR (ENOMEM, -1);
  memcpy (buffer, inode, sizeof (struct ext2_inode));

  large = (struct ext2_large_inode *) buffer;
  large->i_extra_isize = sizeof (struct ext2_large_inode) - EXT2_OLD_INODE_SIZE;
  if (!large->i_crtime)
    large->i_crtime = t;

  ret = ext2_update_inode (fs, ino, buffer, size);
  free (buffer);
  return ret;
}

int
ext2_new_file (struct vnode *dir, const char *name, mode_t mode,
	       struct vnode **result)
{
  struct ext2_fs *fs = dir->mount->data;
  struct vnode *vp;
  struct ext2_file *file;
  struct ext2_inode *inode;
  ino_t ino;
  int ret = ext2_read_bitmaps (fs);
  if (ret)
    return ret;

  vp = vnode_alloc ();
  if (UNLIKELY (!vp))
    RETV_ERROR (ENOMEM, -1);
  ret = ext2_new_inode (fs, dir->ino, fs->inode_bitmap, &ino);
  if (ret)
    {
      UNREF_OBJECT (vp);
      return ret;
    }
  vp->ops = &ext2_vnode_ops;
  vp->data = calloc (1, sizeof (struct ext2_file));
  REF_ASSIGN (vp->mount, dir->mount);
  if (UNLIKELY (!vp->data))
    {
      UNREF_OBJECT (vp);
      return ret;
    }
  ret = ext2_open_file (fs, ino, vp->data);
  if (ret)
    {
      UNREF_OBJECT (vp);
      return ret;
    }
  file = vp->data;
  inode = &file->inode;
  memset (inode, 0, sizeof (struct ext2_inode));
  inode->i_mode = mode;
  inode->i_uid = THIS_PROCESS->euid;
  inode->i_gid = THIS_PROCESS->egid;
  inode->i_links_count = 1;
  ext2_write_new_inode (fs, ino, inode);
  ext2_inode_alloc_stats (fs, ino, 1, S_ISDIR (inode->i_mode));
  ret = ext2_add_link (fs, dir, name, ino, ext2_dir_type (mode));
  if (ret)
    {
      UNREF_OBJECT (vp);
      return ret;
    }

  vp->ino = ino;
  vp->mode = inode->i_mode;
  vp->uid = inode->i_uid;
  vp->gid = inode->i_gid;
  vp->nlink = inode->i_links_count;

  if (!result)
    UNREF_OBJECT (vp);
  else
    *result = vp;
  return ret;
}

int
ext2_alloc_block (struct ext2_fs *fs, block_t goal, char *blockbuf,
		  block_t *result, struct ext2_balloc_ctx *ctx)
{
  block_t block;
  int ret;
  if (!fs->block_bitmap)
    {
      ret = ext2_read_bitmap (fs, EXT2_BITMAP_BLOCK, 0,
			      fs->group_desc_count - 1);
      if (ret)
	return ret;
    }
  ret = ext2_new_block (fs, goal, NULL, &block, ctx);
  if (ret)
    return ret;
  if (blockbuf)
    {
      memset (blockbuf, 0, fs->blksize);
      ret = ext2_write_blocks (blockbuf, fs, block, 1);
    }
  else
    ret = ext2_zero_blocks (fs, block, 1, NULL, NULL);
  if (ret)
    return ret;
  ext2_block_alloc_stats (fs, block, 1);
  *result = block;
  return ret;
}

int
ext2_dealloc_blocks (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		     char *blockbuf, block_t start, block_t end)
{
  struct ext2_inode inode_buf;
  int ret;
  if (start > end)
    RETV_ERROR (EINVAL, -1);
  if (!inode)
    {
      ret = ext2_read_inode (fs, ino, &inode_buf);
      if (ret)
	return ret;
      inode = &inode_buf;
    }

  if (inode->i_flags & EXT4_INLINE_DATA_FL)
    RETV_ERROR (ENOTSUP, -1);

  if (inode->i_flags & EXT4_EXTENTS_FL)
    ret = ext3_extent_dealloc_blocks (fs, ino, inode, start, end);
  else
    ret = ext2_dealloc_indirect (fs, inode, blockbuf, start, end);
  if (ret)
    return ret;
  return ext2_update_inode (fs, ino, inode, sizeof (struct ext2_inode));
}

int
ext2_lookup_inode (struct ext2_fs *fs, struct vnode *dir, const char *name,
		   int namelen, char *buffer, ino_t *inode)
{
  struct ext2_lookup_ctx l;
  int ret;
  l.name = name;
  l.namelen = namelen;
  l.inode = inode;
  l.found = 0;
  ret = ext2_dir_iterate (fs, dir, 0, buffer, ext2_process_lookup, &l);
  if (ret)
    return ret;
  if (l.found)
    return 0;
  else
    RETV_ERROR (ENOENT, -1);
}

int
ext2_expand_dir (struct vnode *dir)
{
  struct ext2_fs *fs = dir->mount->data;
  struct ext2_file *file = dir->data;
  struct ext2_dir_expand_ctx e;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  e.done = 0;
  e.err = 0;
  e.goal = ext2_find_inode_goal (fs, dir->ino, &file->inode, 0);
  e.newblocks = 0;
  e.dir = dir;
  ret = ext2_block_iterate (fs, dir, BLOCK_FLAG_APPEND, NULL,
			    ext2_process_dir_expand, &e);
  if (ret == -1 && errno == ENOTSUP)
    RETV_ERROR (ENOTSUP, -1);
  if (e.err)
    return -1;
  if (!e.done)
    RETV_ERROR (EIO, -1);
  ret = ext2_inode_set_size (fs, &file->inode,
			     EXT2_I_SIZE (file->inode) + fs->blksize);
  if (ret)
    return ret;
  ext2_iblk_add_blocks (fs, &file->inode, e.newblocks);
  return ext2_update_inode (fs, dir->ino, &file->inode,
			    sizeof (struct ext2_inode));
}

int
ext2_get_rec_len (struct ext2_fs *fs, struct ext2_dirent *dirent,
		  unsigned int *rec_len)
{
  unsigned int len = dirent->d_rec_len;
  if (fs->blksize < 65536)
    *rec_len = len;
  else if (len == 65535 || !len)
    *rec_len = fs->blksize;
  else
    *rec_len = (len & 65532) | ((len & 3) << 16);
  return 0;
}

int
ext2_set_rec_len (struct ext2_fs *fs, unsigned int len,
		  struct ext2_dirent *dirent)
{
  if (len > fs->blksize || fs->blksize > 262144 || (len & 3))
    RETV_ERROR (EINVAL, -1);
  if (len < 65536)
    {
      dirent->d_rec_len = len;
      return 0;
    }
  if (len == fs->blksize)
    {
      if (fs->blksize == 65536)
	dirent->d_rec_len = 65535;
      else
	dirent->d_rec_len = 0;
    }
  else
    dirent->d_rec_len = (len & 65532) | ((len >> 16) & 3);
  return 0;
}

int
ext2_block_iterate (struct ext2_fs *fs, struct vnode *dir, int flags,
		    char *blockbuf, ext2_block_iter_t func, void *private)
{
  struct ext2_file *file = dir->data;
  struct ext2_inode *inode = &file->inode;
  struct ext2_block_ctx ctx;
  block_t block;
  int limit;
  int i;
  int r;
  int ret = 0;
  if (inode->i_flags & EXT4_INLINE_DATA_FL)
    RETV_ERROR (ENOTSUP, -1);

  if (flags & BLOCK_FLAG_NO_LARGE)
    {
      if (!S_ISDIR (inode->i_mode) && inode->i_size_high)
	RETV_ERROR (EFBIG, -1);
    }

  limit = fs->blksize >> 2;
  ctx.fs = fs;
  ctx.func = func;
  ctx.private = private;
  ctx.flags = flags;
  ctx.blkcnt = 0;
  ctx.err = 0;
  if (blockbuf)
    ctx.ind_buf = blockbuf;
  else
    {
      ctx.ind_buf = malloc (fs->blksize * 3);
      if (UNLIKELY (!ctx.ind_buf))
	RETV_ERROR (ENOMEM, -1);
    }
  ctx.dind_buf = ctx.ind_buf + fs->blksize;
  ctx.tind_buf = ctx.dind_buf + fs->blksize;

  if (fs->super.s_creator_os == EXT2_OS_HURD && !(flags & BLOCK_FLAG_DATA_ONLY))
    {
      if (inode->osd1.hurd1.h_i_translator)
	{
	  block = inode->osd1.hurd1.h_i_translator;
	  ret |= ctx.func (fs, &block, BLOCK_COUNT_TRANSLATOR, 0, 0, private);
	  inode->osd1.hurd1.h_i_translator = block;
	  if (ret & BLOCK_ABORT)
	    goto err;
	  if ((ctx.flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
	    {
	      ctx.err = -1;
	      errno = EROFS;
	      ret |= BLOCK_ABORT | BLOCK_ERROR;
	      goto err;
	    }
	}
    }

  if (inode->i_flags & EXT4_EXTENTS_FL)
    {
      struct ext3_extent_handle *handle;
      struct ext3_generic_extent extent;
      struct ext3_generic_extent next;
      blkcnt_t blkcnt = 0;
      block_t block;
      block_t new_block;
      int op = EXT2_EXTENT_ROOT;
      int uninit;
      unsigned int j;

      ctx.err = ext3_extent_open (fs, dir->ino, inode, &handle);
      if (ctx.err)
	goto err;

      while (1)
	{
	  if (op == EXT2_EXTENT_CURRENT)
	    ctx.err = 0;
	  else
	    ctx.err = ext3_extent_get (handle, op, &extent);
	  if (ctx.err)
	    {
	      if (errno != ESRCH)
		break;
	      ctx.err = 0;
	      if (!(flags & BLOCK_FLAG_APPEND))
		break;

	    next_block_set:
	      block = 0;
	      r = ctx.func (fs, &block, blkcnt, 0, 0, private);
	      ret |= r;
	      if ((ctx.flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
		{
		  ctx.err = -1;
		  errno = EROFS;
		  ret |= BLOCK_ABORT | BLOCK_ERROR;
		  goto err;
		}
	      if (r & BLOCK_CHANGED)
		{
		  ctx.err = ext3_extent_set_bmap (handle, blkcnt++, block, 0);
		  if (ctx.err || (ret & BLOCK_ABORT))
		    break;
		  if (block)
		    goto next_block_set;
		}
	      break;
	    }

	  op = EXT2_EXTENT_NEXT;
	  block = extent.e_pblk;
	  if (!(extent.e_flags & EXT2_EXTENT_FLAGS_LEAF))
	    {
	      if (ctx.flags & BLOCK_FLAG_DATA_ONLY)
		continue;
	      if ((!(extent.e_flags & EXT2_EXTENT_FLAGS_SECOND_VISIT)
		   && !(ctx.flags & BLOCK_FLAG_DEPTH_TRAVERSE))
		  || ((extent.e_flags & EXT2_EXTENT_FLAGS_SECOND_VISIT)
		      && (ctx.flags & BLOCK_FLAG_DEPTH_TRAVERSE)))
		{
		  ret |= ctx.func (fs, &block, -1, 0, 0, private);
		  if (ret & BLOCK_CHANGED)
		    {
		      extent.e_pblk = block;
		      ctx.err = ext3_extent_replace (handle, 0, &extent);
		      if (ctx.err)
			break;
		    }
		  if (ret & BLOCK_ABORT)
		    break;
		}
	      continue;
	    }

	  uninit = 0;
	  if (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
	    uninit = EXT2_EXTENT_SET_BMAP_UNINIT;
	  ret = ext3_extent_get (handle, op, &next);

	  if (extent.e_lblk + extent.e_len <= (block_t) blkcnt)
	    continue;
	  if (extent.e_lblk > (block_t) blkcnt)
	    blkcnt = extent.e_lblk;
	  j = blkcnt - extent.e_lblk;
	  block += j;
	  for (blkcnt = extent.e_lblk, j = 0; j < extent.e_len;
	       block++, blkcnt++, j++)
	    {
	      new_block = block;
	      r = ctx.func (fs, &new_block, blkcnt, 0, 0, private);
	      ret |= r;
	      if ((ctx.flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
		{
		  ctx.err = -1;
		  errno = EROFS;
		  ret |= BLOCK_ABORT | BLOCK_ERROR;
		  goto err;
		}
	      if (r & BLOCK_CHANGED)
		{
		  ctx.err =
		    ext3_extent_set_bmap (handle, blkcnt, new_block, uninit);
		  if (ctx.err)
		    goto done;
		}
	      if (ret & BLOCK_ABORT)
		goto done;
	    }
	  if (!ret)
	    {
	      extent = next;
	      op = EXT2_EXTENT_CURRENT;
	    }
	}

    done:
      ext3_extent_free (handle);
      ret |= BLOCK_ERROR;
      goto end;
    }

  for (i = 0; i < EXT2_NDIR_BLOCKS; i++, ctx.blkcnt++)
    {
      if (inode->i_block[i] || (flags & BLOCK_FLAG_APPEND))
	{
	  block = inode->i_block[i];
	  ret |= ctx.func (fs, &block, ctx.blkcnt, 0, i, private);
	  inode->i_block[i] = block;
	  if (ret & BLOCK_ABORT)
	    goto err;
	}
    }
  if ((ctx.flags & BLOCK_FLAG_READ_ONLY) && (ret & BLOCK_CHANGED))
    {
      ctx.err = -1;
      errno = EROFS;
      ret |= BLOCK_ABORT | BLOCK_ERROR;
      goto err;
    }

  if (inode->i_block[EXT2_IND_BLOCK] || (flags & BLOCK_FLAG_APPEND))
    {
      ret |= ext2_block_iterate_ind (&inode->i_block[EXT2_IND_BLOCK], 0,
				     EXT2_IND_BLOCK, &ctx);
      if (ret & BLOCK_ABORT)
	goto err;
    }
  else
    ctx.blkcnt += limit;

  if (inode->i_block[EXT2_DIND_BLOCK] || (flags & BLOCK_FLAG_APPEND))
    {
      ret |= ext2_block_iterate_dind (&inode->i_block[EXT2_DIND_BLOCK], 0,
				      EXT2_DIND_BLOCK, &ctx);
      if (ret & BLOCK_ABORT)
	goto err;
    }
  else
    ctx.blkcnt += limit * limit;

  if (inode->i_block[EXT2_TIND_BLOCK] || (flags & BLOCK_FLAG_APPEND))
    {
      ret |= ext2_block_iterate_tind (&inode->i_block[EXT2_TIND_BLOCK], 0,
				      EXT2_TIND_BLOCK, &ctx);
      if (ret & BLOCK_ABORT)
	goto err;
    }

 err:
  if (ret & BLOCK_CHANGED)
    {
      ret = ext2_update_inode (fs, dir->ino, inode, sizeof (struct ext2_inode));
      if (ret)
	{
	  ret |= BLOCK_ERROR;
	  ctx.err = ret;
	}
    }

 end:
  if (!blockbuf)
    free (ctx.ind_buf);
  return ret & BLOCK_ERROR ? ctx.err : 0;
}

int
ext2_dir_iterate (struct ext2_fs *fs, struct vnode *dir, int flags,
		  char *blockbuf, ext2_dir_iter_t func, void *private)
{
  struct ext2_dir_ctx ctx;
  int ret;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);

  ctx.dir = dir;
  ctx.flags = flags;
  if (blockbuf)
    ctx.buffer = blockbuf;
  else
    {
      ctx.buffer = malloc (fs->blksize);
      if (UNLIKELY (!ctx.buffer))
	RETV_ERROR (ENOMEM, -1);
    }
  ctx.func = func;
  ctx.private = private;
  ctx.err = 0;
  ret = ext2_block_iterate (fs, dir, BLOCK_FLAG_READ_ONLY, 0,
			    ext2_process_dir_block, &ctx);
  if (!blockbuf)
    free (ctx.buffer);
  if (ret)
    return ret; /* TODO Support iterating over inline data */
  return ctx.err;
}

void
ext2_init_dirent_tail (struct ext2_fs *fs, struct ext2_dirent_tail *t)
{
  memset (t, 0, sizeof (struct ext2_dirent_tail));
  ext2_set_rec_len (fs, sizeof (struct ext2_dirent_tail),
		    (struct ext2_dirent *) t);
  t->det_reserved_name_len = EXT2_DIR_NAME_CHECKSUM;
}
