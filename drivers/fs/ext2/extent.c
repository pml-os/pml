/* extent.c -- This file is part of PML.
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

/* Copyright (C) 2007 Theodore Ts'o.
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
ext3_extent_update_path (struct ext3_extent_handle *handle)
{
  struct ext2_fs *fs = handle->fs;
  struct ext3_extent_index *index;
  struct ext3_extent_header *eh;
  block_t block;
  int ret;
  if (!handle->level)
    ret = ext2_update_inode (fs, handle->ino, handle->inode,
			     sizeof (struct ext2_inode));
  else
    {
      index = handle->path[handle->level - 1].curr;
      block = index->ei_leaf + ((uint64_t) index->ei_leaf_hi << 32);
      eh = (struct ext3_extent_header *) handle->path[handle->level].buffer;
      ret = ext3_extent_block_checksum_update (fs, handle->ino, eh);
      if (ret)
	return ret;
      ret = ext2_write_blocks (handle->path[handle->level].buffer, fs,
			       block, 1);
    }
  return ret;
}

static int
ext3_extent_splitting_eof (struct ext3_extent_handle *handle,
			   struct ext3_generic_extent_path *path)
{
  struct ext3_generic_extent_path *p = path;
  if (!handle->level)
    return 0;
  do
    {
      if (p->left)
	return 0;
      p--;
    }
  while (p >= handle->path);
  return 1;
}

static int
ext3_extent_dealloc_range (struct ext2_fs *fs, ino_t ino,
			   struct ext2_inode *inode, block_t lfree_start,
			   block_t free_start, unsigned int free_count,
			   int *freed)
{
  block_t block;
  unsigned int cluster_freed;
  int freed_now = 0;
  int ret;

  if (EXT2_CLUSTER_RATIO (fs) == 1)
    {
      *freed += free_count;
      while (free_count-- > 0)
	ext2_block_alloc_stats (fs, free_start++, -1);
      return 0;
    }

  if (free_start & EXT2_CLUSTER_MASK (fs))
    {
      ret = ext2_map_cluster_block (fs, ino, inode, lfree_start, &block);
      if (ret)
	goto end;
      if (!block)
	{
	  ext2_block_alloc_stats (fs, free_start, -1);
	  freed_now++;
	}
    }

  while (free_count > 0 && free_count >= (unsigned int) EXT2_CLUSTER_RATIO (fs))
    {
      ext2_block_alloc_stats (fs, free_start, -1);
      freed_now++;
      cluster_freed = EXT2_CLUSTER_RATIO (fs);
      free_count -= cluster_freed;
      free_start += cluster_freed;
      lfree_start += cluster_freed;
    }

  if (free_count > 0)
    {
      ret = ext2_map_cluster_block (fs, ino, inode, lfree_start, &block);
      if (ret)
	goto end;
      if (!block)
	{
	  ext2_block_alloc_stats (fs, free_start, -1);
	  freed_now++;
	}
    }

 end:
  *freed += freed_now;
  return ret;
}

int
ext3_extent_open (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		  struct ext3_extent_handle **handle)
{
  struct ext3_extent_handle *h;
  struct ext3_extent_header *eh;
  int ret;
  int i;

  if (!inode)
    {
      if (!ino || ino > fs->super.s_inodes_count)
	RETV_ERROR (EINVAL, -1);
    }

  h = calloc (1, sizeof (struct ext3_extent_handle));
  if (UNLIKELY (!h))
    RETV_ERROR (ENOMEM, -1);
  h->ino = ino;
  h->fs = fs;

  if (inode)
    h->inode = inode;
  else
    {
      h->inode = &h->inode_buf;
      ret = ext2_read_inode (fs, ino, h->inode);
      if (ret)
	goto err;
    }

  eh = (struct ext3_extent_header *) h->inode->i_block;
  for (i = 0; i < EXT2_N_BLOCKS; i++)
    {
      if (h->inode->i_block[i])
	break;
    }
  if (i >= EXT2_N_BLOCKS)
    {
      eh->eh_magic = EXT3_EXTENT_MAGIC;
      eh->eh_depth = 0;
      eh->eh_entries = 0;
      i = (sizeof (h->inode->i_block) - sizeof (struct ext3_extent_header)) /
	sizeof (struct ext3_extent);
      eh->eh_max = i;
      h->inode->i_flags |= EXT4_EXTENTS_FL;
    }
  if (!(h->inode->i_flags & EXT4_EXTENTS_FL))
    GOTO_ERROR (EINVAL, err);

  ret = ext3_extent_header_valid (eh, sizeof (h->inode->i_block));
  if (ret)
    goto err;

  h->max_depth = eh->eh_depth;
  h->type = eh->eh_magic;
  h->max_paths = h->max_depth + 1;
  h->path = malloc (h->max_paths * sizeof (struct ext3_generic_extent_path));
  if (UNLIKELY (!h->path))
    GOTO_ERROR (ENOMEM, err);
  h->path[0].buffer = (char *) h->inode->i_block;
  h->path[0].left = eh->eh_entries;
  h->path[0].entries = eh->eh_entries;
  h->path[0].max_entries = eh->eh_max;
  h->path[0].curr = 0;
  h->path[0].end_block = (EXT2_I_SIZE (*h->inode) + fs->blksize - 1) >>
    EXT2_BLOCK_SIZE_BITS (fs->super);
  h->path[0].visit_num = 1;
  h->level = 0;
  *handle = h;
  return 0;

 err:
  ext3_extent_free (h);
  return ret;
}

int ext3_extent_header_valid (struct ext3_extent_header *eh, size_t size)
{
  int hmax;
  size_t entry_size;
  if (eh->eh_magic != EXT3_EXTENT_MAGIC)
    RETV_ERROR (EUCLEAN, -1);
  if (eh->eh_entries > eh->eh_max)
    RETV_ERROR (EUCLEAN, -1);

  if (!eh->eh_depth)
    entry_size = sizeof (struct ext3_extent);
  else
    entry_size = sizeof (struct ext3_extent_index);
  hmax = (size - sizeof (struct ext3_extent_header)) / entry_size;
  if (eh->eh_max > hmax || eh->eh_max < hmax - 2)
    RETV_ERROR (EUCLEAN, -1);
  return 0;
}

int
ext3_extent_goto (struct ext3_extent_handle *handle, int leaflvl, block_t block)
{
  struct ext3_generic_extent extent;
  int ret = ext3_extent_get (handle, EXT2_EXTENT_ROOT, &extent);
  if (ret)
    {
      if (errno == ESRCH)
	errno = ENOENT;
      return ret;
    }
  if (leaflvl > handle->max_depth)
    RETV_ERROR (ENOTSUP, -1);

  while (1)
    {
      if (handle->max_depth - handle->level == leaflvl)
	{
	  if (block >= extent.e_lblk && block < extent.e_lblk + extent.e_len)
	    return 0;
	  if (block < extent.e_lblk)
	    {
	      ext3_extent_get (handle, EXT2_EXTENT_PREV_SIB, &extent);
	      RETV_ERROR (ENOENT, -1);
	    }
	  ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_SIB, &extent);
	  if (ret && errno == ESRCH)
	    RETV_ERROR (ENOENT, -1);
	  if (ret)
	    return ret;
	  continue;
	}

      ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_SIB, &extent);
      if (ret && errno == ESRCH)
	goto go_down;
      if (ret)
	return ret;
      if (block == extent.e_lblk)
	goto go_down;
      if (block > extent.e_lblk)
	continue;

      ret = ext3_extent_get (handle, EXT2_EXTENT_PREV_SIB, &extent);
      if (ret)
	return ret;

    go_down:
      ret = ext3_extent_get (handle, EXT2_EXTENT_DOWN, &extent);
      if (ret)
	return ret;
    }
}

int
ext3_extent_get (struct ext3_extent_handle *handle, int flags,
		 struct ext3_generic_extent *extent)
{
  struct ext2_fs *fs = handle->fs;
  struct ext3_generic_extent_path *path;
  struct ext3_generic_extent_path *newpath;
  struct ext3_extent_header *eh;
  struct ext3_extent_index *index = NULL;
  struct ext3_extent *ex;
  block_t block;
  block_t endblock;
  int orig_op;
  int op;
  int fail_csum = 0;
  int ret;

  if (!handle->path)
    RETV_ERROR (ENOENT, -1);
  orig_op = op = flags & EXT2_EXTENT_MOVE_MASK;

 retry:
  path = handle->path + handle->level;
  if (orig_op == EXT2_EXTENT_NEXT || orig_op == EXT2_EXTENT_NEXT_LEAF)
    {
      if (handle->level < handle->max_depth)
	{
	  if (!path->visit_num)
	    {
	      path->visit_num++;
	      op = EXT2_EXTENT_DOWN;
	    }
	  else if (path->left > 0)
	    op = EXT2_EXTENT_NEXT_SIB;
	  else if (handle->level > 0)
	    op = EXT2_EXTENT_UP;
	  else
	    RETV_ERROR (ESRCH, -1);
	}
      else
	{
	  if (path->left > 0)
	    op = EXT2_EXTENT_NEXT_SIB;
	  else if (handle->level > 0)
	    op = EXT2_EXTENT_UP;
	  else
	    RETV_ERROR (ESRCH, -1);
	}
    }

  if (orig_op == EXT2_EXTENT_PREV || orig_op == EXT2_EXTENT_PREV_LEAF)
    {
      if (handle->level < handle->max_depth)
	{
	  if (path->visit_num > 0)
	    op = EXT2_EXTENT_DOWN_LAST;
	  else if (path->left < path->entries - 1)
	    op = EXT2_EXTENT_PREV_SIB;
	  else if (handle->level > 0)
	    op = EXT2_EXTENT_UP;
	  else
	    RETV_ERROR (ESRCH, -1);
	}
      else
	{
	  if (path->left < path->entries - 1)
	    op = EXT2_EXTENT_PREV_SIB;
	  else if (handle->level > 0)
	    op = EXT2_EXTENT_UP;
	  else
	    RETV_ERROR (ESRCH, -1);
	}
    }

  if (orig_op == EXT2_EXTENT_LAST_LEAF)
    {
      if (handle->level < handle->max_depth && !path->left)
	op = EXT2_EXTENT_DOWN;
      else
	op = EXT2_EXTENT_LAST_SIB;
    }

  switch (op)
    {
    case EXT2_EXTENT_CURRENT:
      index = path->curr;
      break;
    case EXT2_EXTENT_ROOT:
      handle->level = 0;
      path = handle->path + handle->level;
      /* fallthrough */
    case EXT2_EXTENT_FIRST_SIB:
      path->left = path->entries;
      path->curr = NULL;
      /* fallthrough */
    case EXT2_EXTENT_NEXT_SIB:
      if (path->left <= 0)
	RETV_ERROR (ESRCH, -1);
      if (path->curr)
	{
	  index = path->curr;
	  index++;
	}
      else
	{
	  eh = (struct ext3_extent_header *) path->buffer;
	  index = EXT2_FIRST_INDEX (eh);
	}
      path->left--;
      path->curr = index;
      path->visit_num = 0;
      break;
    case EXT2_EXTENT_PREV_SIB:
      if (!path->curr || path->left + 1 >= path->entries)
        RETV_ERROR (ESRCH, -1);
      index = path->curr;
      index--;
      path->curr = index;
      path->left++;
      if (handle->level < handle->max_depth)
        path->visit_num = 1;
      break;
    case EXT2_EXTENT_LAST_SIB:
      eh = (struct ext3_extent_header *) path->buffer;
      path->curr = EXT2_LAST_EXTENT (eh);
      index = path->curr;
      path->left = 0;
      path->visit_num = 0;
      break;
    case EXT2_EXTENT_UP:
      if (handle->level <= 0)
        RETV_ERROR (EINVAL, -1);
      handle->level--;
      path--;
      index = path->curr;
      if (orig_op == EXT2_EXTENT_PREV || orig_op == EXT2_EXTENT_PREV_LEAF)
        path->visit_num = 0;
      break;
    case EXT2_EXTENT_DOWN:
    case EXT2_EXTENT_DOWN_LAST:
      if (!path->curr || handle->level >= handle->max_depth)
        RETV_ERROR (EINVAL, -1);
      index = path->curr;
      newpath = path + 1;
      if (!newpath->buffer)
        {
          newpath->buffer = malloc (fs->blksize);
          if (UNLIKELY (!newpath->buffer))
            RETV_ERROR (ENOMEM, -1);
        }
      block = index->ei_leaf + ((uint64_t) index->ei_leaf_hi << 32);
      ret = ext2_read_blocks (newpath->buffer, fs, block, 1);
      if (ret)
        return ret;
      handle->level++;
      eh = (struct ext3_extent_header *) newpath->buffer;
      ret = ext3_extent_header_valid (eh, fs->blksize);
      if (ret)
        {
          handle->level--;
          return ret;
        }

      if (!ext3_extent_block_checksum_valid (fs, handle->ino, eh))
        fail_csum = 1;
      newpath->entries = eh->eh_entries;
      newpath->left = eh->eh_entries;
      newpath->max_entries = eh->eh_max;
      if (path->left > 0)
        {
          index++;
          newpath->end_block = index->ei_block;
        }
      else
        newpath->end_block = path->end_block;
      path = newpath;

      if (op == EXT2_EXTENT_DOWN)
        {
          index = EXT2_FIRST_INDEX (eh);
          path->curr = index;
          path->left = path->entries - 1;
          path->visit_num = 0;
        }
      else
        {
          index = EXT2_LAST_INDEX (eh);
          path->curr = index;
          path->left = 0;
          if (handle->level < handle->max_depth)
            path->visit_num = 1;
        }
      break;
    default:
      RETV_ERROR (EINVAL, -1);
    }
  if (!index)
    RETV_ERROR (ENOENT, -1);
  extent->e_flags = 0;

  if (handle->level == handle->max_depth)
    {
      ex = (struct ext3_extent *) index;
      extent->e_pblk = ex->ee_start + ((uint64_t) ex->ee_start_hi << 32);
      extent->e_lblk = ex->ee_block;
      extent->e_len = ex->ee_len;
      extent->e_flags |= EXT2_EXTENT_FLAGS_LEAF;
      if (extent->e_len > EXT2_INIT_MAX_LEN)
        {
          extent->e_len -= EXT2_INIT_MAX_LEN;
          extent->e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
        }
    }
  else
    {
      extent->e_pblk = index->ei_leaf + ((uint64_t) index->ei_leaf_hi << 32);
      extent->e_lblk = index->ei_block;
      if (path->left > 0)
        {
          index++;
          endblock = index->ei_block;
        }
      else
        endblock = path->end_block;
      extent->e_len = endblock - extent->e_lblk;
    }
  if (path->visit_num)
    extent->e_flags |= EXT2_EXTENT_FLAGS_SECOND_VISIT;

  if ((orig_op == EXT2_EXTENT_NEXT_LEAF || orig_op == EXT2_EXTENT_PREV_LEAF)
      && handle->level != handle->max_depth)
    goto retry;
  if (orig_op == EXT2_EXTENT_LAST_LEAF
      && (handle->level != handle->max_depth || path->left))
    goto retry;
  if (fail_csum)
    RETV_ERROR (EUCLEAN, -1);
  else
    return 0;
}

int
ext3_extent_get_info (struct ext3_extent_handle *handle,
		      struct ext3_extent_info *info)
{
  struct ext3_generic_extent_path *path;
  memset (info, 0, sizeof (struct ext3_extent_info));
  path = handle->path + handle->level;
  if (path)
    {
      if (path->curr)
	info->curr_entry = ((char *) path->curr - path->buffer) /
	  sizeof (struct ext3_extent_index);
      else
	info->curr_entry = 0;
      info->num_entries = path->entries;
      info->max_entries = path->max_entries;
      info->bytes_avail =
	(path->max_entries - path->entries) * sizeof (struct ext3_extent);
    }
  info->curr_level = handle->level;
  info->max_depth = handle->max_depth;
  info->max_lblk = EXT2_MAX_EXTENT_LBLK;
  info->max_pblk = EXT2_MAX_EXTENT_PBLK;
  info->max_len = EXT2_INIT_MAX_LEN;
  info->max_uninit_len = EXT2_UNINIT_MAX_LEN;
  return 0;
}

int
ext3_extent_node_split (struct ext3_extent_handle *handle, int canexpand)
{
  struct ext2_fs *fs = handle->fs;
  block_t new_node_block;
  block_t new_node_start;
  block_t orig_block;
  block_t goal_block = 0;
  char *blockbuf = NULL;
  struct ext3_generic_extent extent;
  struct ext3_generic_extent_path *path;
  struct ext3_generic_extent_path *new_path = NULL;
  struct ext3_extent_header *eh;
  struct ext3_extent_header *new_eh;
  struct ext3_extent_info info;
  int orig_height;
  int new_root = 0;
  int to_copy;
  int no_balance;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!handle->path)
    RETV_ERROR (ENOENT, -1);

  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
  if (ret)
    goto end;
  ret = ext3_extent_get_info (handle, &info);
  if (ret)
    goto end;

  orig_height = info.max_depth - info.curr_level;
  orig_block = extent.e_lblk;

  path = handle->path + handle->level;
  eh = (struct ext3_extent_header *) path->buffer;
  if (handle->level == handle->max_depth)
    {
      struct ext3_extent *ex = EXT2_FIRST_EXTENT (eh);
      goal_block = ex->ee_start + ((block_t) ex->ee_start_hi << 32);
    }
  else
    {
      struct ext3_extent_index *index = EXT2_FIRST_INDEX (eh);
      goal_block = index->ei_leaf + ((block_t) index->ei_leaf_hi << 32);
    }
  goal_block -= EXT2_CLUSTER_RATIO (fs);
  goal_block &= ~EXT2_CLUSTER_MASK (fs);

  if (handle->level && handle->path[handle->level - 1].entries >=
      handle->path[handle->level - 1].max_entries)
    {
      ret = ext3_extent_get (handle, EXT2_EXTENT_UP, &extent);
      if (ret)
	goto end;
      ret = ext3_extent_node_split (handle, canexpand);
      if (ret)
	goto end;
      ret = ext3_extent_goto (handle, orig_height, orig_block);
      if (ret)
	goto end;
    }

  path = handle->path + handle->level;
  if (!path->curr)
    RETV_ERROR (ENOENT, -1);
  no_balance = canexpand ? ext3_extent_splitting_eof (handle, path) : 0;
  eh = (struct ext3_extent_header *) path->buffer;

  if (!handle->level)
    {
      new_root = 1;
      to_copy = eh->eh_entries;
      new_path =
	calloc (handle->max_paths + 1, sizeof (struct ext3_generic_extent));
      if (UNLIKELY (!new_path))
	GOTO_ERROR (ENOMEM, end);
    }
  else
    {
      if (no_balance)
	to_copy = 1;
      else
	to_copy = eh->eh_entries / 2;
    }

  if (!to_copy && !no_balance)
    GOTO_ERROR (ENOSPC, end);

  blockbuf = malloc (fs->blksize);
  if (UNLIKELY (!blockbuf))
    GOTO_ERROR (ENOMEM, end);
  if (!goal_block)
    goal_block = ext2_find_inode_goal (fs, handle->ino,
				       handle->inode, 0);
  ret = ext2_alloc_block (fs, goal_block, blockbuf, &new_node_block,
			  NULL);
  if (ret)
    goto end;

  new_eh = (struct ext3_extent_header *) blockbuf;
  memcpy (new_eh, eh, sizeof (struct ext3_extent_header));
  new_eh->eh_entries = to_copy;
  new_eh->eh_max = (fs->blksize - sizeof (struct ext3_extent_header)) /
    sizeof (struct ext3_extent);
  memcpy (EXT2_FIRST_INDEX (new_eh), EXT2_FIRST_INDEX (eh) +
	  eh->eh_entries - to_copy,
	  sizeof (struct ext3_extent_index) * to_copy);
  new_node_start = EXT2_FIRST_INDEX (new_eh)->ei_block;
  ret =
    ext3_extent_block_checksum_update (fs, handle->ino, new_eh);
  if (ret)
    goto end;
  ret = ext2_write_blocks (blockbuf, fs, new_node_block, 1);
  if (ret)
    goto end;

  if (!handle->level)
    {
      memcpy (new_path, path, sizeof (struct ext3_generic_extent_path) *
	      handle->max_paths);
      handle->path = new_path;
      new_path = path;
      path = handle->path;
      path->entries = 1;
      path->left = path->max_entries - 1;
      handle->max_depth++;
      handle->max_paths++;
      eh->eh_depth = handle->max_depth;
    }
  else
    {
      path->entries -= to_copy;
      path->left -= to_copy;
    }

  eh->eh_entries = path->entries;
  ret = ext3_extent_update_path (handle);
  if (ret)
    goto end;

  if (new_root)
    {
      ret = ext3_extent_get (handle, EXT2_EXTENT_FIRST_SIB, &extent);
      if (ret)
	goto end;
      extent.e_lblk = new_node_start;
      extent.e_pblk = new_node_block;
      extent.e_len = handle->path[0].end_block - extent.e_lblk;
      ret = ext3_extent_replace (handle, 0, &extent);
      if (ret)
	goto end;
    }
  else
    {
      unsigned int new_node_len;
      ret = ext3_extent_get (handle, EXT2_EXTENT_UP, &extent);
      new_node_len = new_node_start - extent.e_lblk;
      extent.e_len -= new_node_len;
      ret = ext3_extent_replace (handle, 0, &extent);
      if (ret)
	goto end;
      extent.e_lblk = new_node_start;
      extent.e_pblk = new_node_block;
      extent.e_len = new_node_len;
      ret = ext3_extent_insert (handle, EXT2_EXTENT_INSERT_AFTER, &extent);
      if (ret)
	goto end;
    }

  ret = ext3_extent_goto (handle, orig_height, orig_block);
  if (ret)
    goto end;

  ext2_iblk_add_blocks (fs, handle->inode, 1);
  ret = ext2_update_inode (fs, handle->ino, handle->inode,
			   sizeof (struct ext2_inode));

 end:
  if (new_path)
    free (new_path);
  free (blockbuf);
  return ret;
}

int
ext3_extent_fix_parents (struct ext3_extent_handle *handle)
{
  struct ext2_fs *fs = handle->fs;
  struct ext3_generic_extent_path *path;
  struct ext3_generic_extent extent;
  struct ext3_extent_info info;
  block_t start;
  int orig_height;
  int ret = 0;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!handle->path)
    RETV_ERROR (ENOENT, -1);

  path = handle->path + handle->level;
  if (!path->curr)
    RETV_ERROR (ENOENT, -1);

  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
  if (ret)
    return ret;
  start = extent.e_lblk;

  ret = ext3_extent_get_info (handle, &info);
  if (ret)
    return ret;
  orig_height = info.max_depth - info.curr_level;

  while (handle->level > 0 && path->left == path->entries - 1)
    {
      ret = ext3_extent_get (handle, EXT2_EXTENT_UP, &extent);
      if (ret)
	return ret;
      if (extent.e_lblk == start)
	break;
      path = handle->path + handle->level;
      extent.e_len += extent.e_lblk - start;
      extent.e_lblk = start;
      ret = ext3_extent_replace (handle, 0, &extent);
      if (ret)
	return ret;
      ext3_extent_update_path (handle);
    }
  return ext3_extent_goto (handle, orig_height, start);
}

int
ext3_extent_insert (struct ext3_extent_handle *handle, int flags,
		    struct ext3_generic_extent *extent)
{
  struct ext2_fs *fs = handle->fs;
  struct ext3_generic_extent_path *path;
  struct ext3_extent_index *index;
  struct ext3_extent_header *eh;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!handle->path)
    RETV_ERROR (ENOENT, -1);

  path = handle->path + handle->level;
  if (path->entries >= path->max_entries)
    {
      if (flags & EXT2_EXTENT_INSERT_NOSPLIT)
	RETV_ERROR (ENOSPC, -1);
      else
	{
	  ret = ext3_extent_node_split (handle, 1);
	  if (ret)
	    return ret;
	  path = handle->path + handle->level;
	}
    }

  eh = (struct ext3_extent_header *) path->buffer;
  if (path->curr)
    {
      index = path->curr;
      if (flags & EXT2_EXTENT_INSERT_AFTER)
	{
	  index++;
	  path->left--;
	}
    }
  else
    {
      index = EXT2_FIRST_INDEX (eh);
      path->left = -1;
    }
  path->curr = index;

  if (path->left >= 0)
    memmove (index + 1, index,
	     (path->left + 1) * sizeof (struct ext3_extent_index));
  path->left++;
  path->entries++;

  eh = (struct ext3_extent_header *) path->buffer;
  eh->eh_entries = path->entries;
  ret = ext3_extent_replace (handle, 0, extent);
  if (ret)
    goto err;
  ret = ext3_extent_update_path (handle);
  if (ret)
    goto err;
  return 0;

 err:
  ext3_extent_delete (handle, 0);
  return ret;
}

int
ext3_extent_replace (struct ext3_extent_handle *handle, int flags,
		     struct ext3_generic_extent *extent)
{
  struct ext2_fs *fs = handle->fs;
  struct ext3_generic_extent_path *path;
  struct ext3_extent_index *index;
  struct ext3_extent *ex;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!handle->path)
    RETV_ERROR (ENOENT, -1);

  path = handle->path + handle->level;
  if (!path->curr)
    RETV_ERROR (ENOENT, -1);

  if (handle->level == handle->max_depth)
    {
      ex = path->curr;
      ex->ee_block = extent->e_lblk;
      ex->ee_start = extent->e_pblk & 0xffffffff;
      ex->ee_start_hi = extent->e_pblk >> 32;
      if (extent->e_flags & EXT2_EXTENT_FLAGS_UNINIT)
	{
	  if (extent->e_len > EXT2_UNINIT_MAX_LEN)
	    RETV_ERROR (EUCLEAN, -1);
	  ex->ee_len = extent->e_len + EXT2_INIT_MAX_LEN;
	}
      else
	{
	  if (extent->e_len > EXT2_INIT_MAX_LEN)
	    RETV_ERROR (EUCLEAN, -1);
	  ex->ee_len = extent->e_len;
	}
    }
  else
    {
      index = path->curr;
      index->ei_leaf = extent->e_pblk & 0xffffffff;
      index->ei_leaf_hi = extent->e_pblk >> 32;
      index->ei_block = extent->e_lblk;
      index->ei_unused = 0;
    }
  ext3_extent_update_path (handle);
  return 0;
}

int
ext3_extent_delete (struct ext3_extent_handle *handle, int flags)
{
  struct ext2_fs *fs = handle->fs;
  struct ext3_extent_header *eh;
  struct ext3_generic_extent_path *path;
  char *ptr;
  int ret = 0;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!handle->path)
    RETV_ERROR (ENOENT, -1);

  path = handle->path + handle->level;
  if (!path->curr)
    RETV_ERROR (ENOENT, -1);
  ptr = path->curr;

  if (path->left)
    {
      memmove (ptr, ptr + sizeof (struct ext3_extent_index),
	       path->left * sizeof (struct ext3_extent_index));
      path->left--;
    }
  else
    {
      struct ext3_extent_index *index = path->curr;
      index--;
      path->curr = index;
    }
  if (!--path->entries)
    path->curr = 0;

  if (!path->entries && handle->level)
    {
      if (!(flags & EXT2_EXTENT_DELETE_KEEP_EMPTY))
	{
	  struct ext3_generic_extent extent;
	  ret = ext3_extent_get (handle, EXT2_EXTENT_UP, &extent);
	  if (ret)
	    return ret;
	  ret = ext3_extent_delete (handle, flags);
	  handle->inode->i_blocks -=
	    (fs->blksize * EXT2_CLUSTER_RATIO (fs)) / 512;
	  ret = ext2_update_inode (fs, handle->ino, handle->inode,
				   sizeof (struct ext2_inode));
	  ext2_block_alloc_stats (fs, extent.e_pblk, -1);
	}
    }
  else
    {
      eh = (struct ext3_extent_header *) path->buffer;
      eh->eh_entries = path->entries;
      if (!path->entries && !handle->level)
	{
	  eh->eh_depth = 0;
	  handle->max_depth = 0;
	}
      ret = ext3_extent_update_path (handle);
    }
  return ret;
}

int
ext3_extent_dealloc_blocks (struct ext2_fs *fs, ino_t ino,
			    struct ext2_inode *inode, block_t start,
			    block_t end)
{
  struct ext3_extent_handle *handle = NULL;
  struct ext3_generic_extent extent;
  block_t free_start;
  block_t lfree_start;
  block_t next;
  unsigned int free_count;
  unsigned int newlen;
  int freed = 0;
  int op;
  int ret = ext3_extent_open (fs, ino, inode, &handle);
  if (ret)
    return ret;

  ext3_extent_goto (handle, 0, start);
  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
  if (ret)
    {
      if (errno == ENOENT)
	ret = 0;
      goto end;
    }

  while (1)
    {
      op = EXT2_EXTENT_NEXT_LEAF;
      next = extent.e_lblk + extent.e_len;
      if (start >= extent.e_lblk)
	{
	  if (end < extent.e_lblk)
	    break;
	  free_start = extent.e_pblk;
	  lfree_start = extent.e_lblk;
	  if (next > end)
	    free_count = end - extent.e_lblk + 1;
	  else
	    free_count = extent.e_len;
	  extent.e_len -= free_count;
	  extent.e_lblk += free_count;
	  extent.e_pblk += free_count;
	}
      else if (end >= next - 1)
	{
	  if (start >= next)
	    goto next_extent;
	  newlen = start - extent.e_lblk;
	  free_start = extent.e_pblk + newlen;
	  lfree_start = extent.e_lblk + newlen;
	  free_count = extent.e_len - newlen;
	  extent.e_len = newlen;
	}
      else
	{
	  struct ext3_generic_extent new_ex;
	  new_ex.e_pblk = extent.e_pblk + end + 1 - extent.e_lblk;
	  new_ex.e_lblk = end + 1;
	  new_ex.e_len = next - end - 1;
	  new_ex.e_flags = extent.e_flags;
	  extent.e_len = start - extent.e_lblk;
	  free_start = extent.e_pblk + extent.e_len;
	  lfree_start = extent.e_lblk + extent.e_len;
	  free_count = end - start + 1;

	  ret = ext3_extent_insert (handle, EXT2_EXTENT_INSERT_AFTER, &new_ex);
	  if (ret)
	    goto end;
	  ret = ext3_extent_fix_parents (handle);
	  if (ret)
	    goto end;
	  ret = ext3_extent_goto (handle, 0, extent.e_lblk);
	  if (ret)
	    goto end;
	}

      if (extent.e_len)
	{
	  ret = ext3_extent_replace (handle, 0, &extent);
	  if (ret)
	    goto end;
	  ret = ext3_extent_fix_parents (handle);
	}
      else
	{
	  struct ext3_generic_extent new_ex;
	  block_t old_block;
	  block_t next_block;
	  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &new_ex);
	  if (ret)
	    goto end;
	  old_block = new_ex.e_lblk;
	  ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_LEAF, &new_ex);
	  if (ret == -1 && errno == ESRCH)
	    next_block = old_block;
	  else if (ret)
	    goto end;
	  else
	    next_block = new_ex.e_lblk;
	  ret = ext3_extent_goto (handle, 0, old_block);
	  if (ret)
	    goto end;
	  ret = ext3_extent_delete (handle, 0);
	  if (ret)
	    goto end;
	  ret = ext3_extent_fix_parents (handle);
	  if (ret == -1 && errno != ENOENT)
	    goto end;
	  ret = 0;

	  ext3_extent_goto (handle, 0, next_block);
	  op = EXT2_EXTENT_CURRENT;
	}
      if (ret)
	goto end;

      ret = ext3_extent_dealloc_range (fs, ino, inode, lfree_start, free_start,
				       free_count, &freed);
      if (ret)
	goto end;

    next_extent:
      ret = ext3_extent_get (handle, op, &extent);
      if (ret == -1 && (errno == ESRCH || errno == ENOENT))
	break;
      if (ret)
	goto end;
    }

  ret = ext2_iblk_sub_blocks (fs, inode, freed);
 end:
  ext3_extent_free (handle);
  return ret;
}

int
ext3_extent_bmap (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		  struct ext3_extent_handle *handle, char *blockbuf, int flags,
		  block_t block, int *retflags, int *blocks_alloc,
		  block_t *physblock)
{
  struct ext2_balloc_ctx alloc_ctx;
  struct ext3_generic_extent extent;
  unsigned int offset;
  block_t b = 0;
  int alloc = 0;
  int ret = 0;
  int set_flags = flags & BMAP_UNINIT ? EXT2_EXTENT_SET_BMAP_UNINIT : 0;
  if (flags & BMAP_SET)
    return ext3_extent_set_bmap (handle, block, *physblock, set_flags);

  ret = ext3_extent_goto (handle, 0, block);
  if (ret)
    {
      if (ret == -1 && errno == ENOENT)
	{
	  extent.e_lblk = block;
	  goto found;
	}
      return ret;
    }
  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
  if (ret)
    return ret;
  offset = block - extent.e_lblk;
  if (block >= extent.e_lblk && offset <= extent.e_len)
    {
      *physblock = extent.e_pblk + offset;
      if (retflags && (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT))
	*retflags |= BMAP_RET_UNINIT;
    }

 found:
  if (!*physblock && (flags & BMAP_ALLOC))
    {
      ext2_cluster_alloc (fs, ino, inode, handle, block, &b);
      if (b)
	goto set_extent;
      ret = ext3_extent_bmap (fs, ino, inode, handle, blockbuf, 0, block - 1, 0,
			      blocks_alloc, &b);
      if (ret)
	b = ext2_find_inode_goal (fs, ino, inode, block);
      alloc_ctx.ino = ino;
      alloc_ctx.inode = inode;
      alloc_ctx.block = extent.e_lblk;
      alloc_ctx.flags = BLOCK_ALLOC_DATA;
      ret = ext2_alloc_block (fs, b, blockbuf, &b, &alloc_ctx);
      if (ret)
	return ret;
      b &= ~EXT2_CLUSTER_MASK (fs);
      b += EXT2_CLUSTER_MASK (fs) & block;
      alloc++;

    set_extent:
      ret = ext3_extent_set_bmap (handle, block, b, set_flags);
      if (ret)
	{
	  ext2_block_alloc_stats (fs, b, -1);
	  return ret;
	}
      ret = ext2_read_inode (fs, ino, inode);
      if (ret)
	return ret;
      *blocks_alloc += alloc;
      *physblock = b;
    }
  return 0;
}

int
ext3_extent_set_bmap (struct ext3_extent_handle *handle, block_t logical,
		      block_t physical, int flags)
{
  int mapped = 1;
  int extent_uninit = 0;
  int prev_uninit = 0;
  int next_uninit = 0;
  int new_uninit = 0;
  int max_len = EXT2_INIT_MAX_LEN;
  int orig_height;
  int has_prev;
  int has_next;
  block_t orig_block;
  struct ext3_generic_extent_path *path;
  struct ext3_generic_extent extent;
  struct ext3_generic_extent next_extent;
  struct ext3_generic_extent prev_extent;
  struct ext3_generic_extent new_extent;
  struct ext3_extent_info info;
  int ec;
  int ret;
  if (handle->fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  if (!handle->path)
    RETV_ERROR (EINVAL, -1);
  path = handle->path + handle->level;

  if (flags & EXT2_EXTENT_SET_BMAP_UNINIT)
    {
      new_uninit = 1;
      max_len = EXT2_UNINIT_MAX_LEN;
    }

  if (physical)
    {
      new_extent.e_len = 1;
      new_extent.e_pblk = physical;
      new_extent.e_lblk = logical;
      new_extent.e_flags = EXT2_EXTENT_FLAGS_LEAF;
      if (new_uninit)
	new_extent.e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
    }
  if (!handle->max_depth && !path->entries)
    return ext3_extent_insert (handle, 0, &new_extent);

  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
  if (ret)
    {
      if (ret == -1 && errno != ENOENT)
	return ret;
      memset (&extent, 0, sizeof (struct ext3_generic_extent));
    }
  ret = ext3_extent_get_info (handle, &info);
  if (ret)
    return ret;
  orig_height = info.max_depth - info.curr_level;
  orig_block = extent.e_lblk;

  ret = ext3_extent_goto (handle, 0, logical);
  if (ret)
    {
      if (ret == -1 && errno == ENOENT)
	{
	  ret = 0;
	  mapped = 0;
	  if (!physical)
	    goto end;
	}
      else
	goto end;
    }
  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
  if (ret)
    goto end;
  if (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
    extent_uninit = 1;
  ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_LEAF, &next_extent);
  if (ret)
    {
      has_next = 0;
      if (ret == -1 && errno != ESRCH)
	goto end;
    }
  else
    {
      has_next = 1;
      if (next_extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
	next_uninit = 1;
    }

  ret = ext3_extent_goto (handle, 0, logical);
  if (ret == -1 && errno != ENOENT)
    goto end;
  ret = ext3_extent_get (handle, EXT2_EXTENT_PREV_LEAF, &prev_extent);
  if (ret)
    {
      has_prev = 0;
      if (ret == -1 && errno != ESRCH)
	goto end;
    }
  else
    {
      has_prev = 1;
      if (prev_extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
	prev_uninit = 1;
    }
  ret = ext3_extent_goto (handle, 0, logical);
  if (ret == -1 && errno != ENOENT)
    goto end;

  if (mapped && new_uninit == extent_uninit
      && extent.e_pblk + logical - extent.e_lblk == physical)
    goto end;
  if (!mapped)
    {
      if (logical == extent.e_lblk + extent.e_len
	  && physical == extent.e_pblk + extent.e_len
	  && new_uninit == extent_uninit && (int) extent.e_len < max_len - 1)
	{
	  extent.e_len++;
	  ret = ext3_extent_replace (handle, 0, &extent);
	}
      else if (logical == extent.e_lblk - 1 && physical == extent.e_pblk - 1
	       && new_uninit == extent_uninit
	       && (int) extent.e_len < max_len - 1)
	{
	  extent.e_len++;
	  extent.e_lblk--;
	  extent.e_pblk--;
	}
      else if (has_next && logical == next_extent.e_lblk - 1
	       && physical == next_extent.e_pblk - 1
	       && new_uninit == next_uninit
	       && (int) next_extent.e_len < max_len - 1)
	{
	  ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_LEAF, &next_extent);
	  if (ret)
	    goto end;
	  next_extent.e_len++;
	  next_extent.e_lblk--;
	  next_extent.e_pblk--;
	  ret = ext3_extent_replace (handle, 0, &next_extent);
	}
      else if (logical < extent.e_lblk)
	ret = ext3_extent_insert (handle, 0, &new_extent);
      else
	ret = ext3_extent_insert (handle, EXT2_EXTENT_INSERT_AFTER,
				  &new_extent);
      if (ret)
	goto end;
      ret = ext3_extent_fix_parents (handle);
      if (ret)
	goto end;
    }
  else if (logical == extent.e_lblk && extent.e_len == 1)
    {
      if (physical)
	ret = ext3_extent_replace (handle, 0, &new_extent);
      else
	{
	  ret = ext3_extent_delete (handle, 0);
	  if (ret)
	    goto end;
	  ec = ext3_extent_fix_parents (handle);
	  if (ec == -1 && errno != ENOENT)
	    ret = ec;
	}
      if (ret)
	goto end;
    }
  else if (logical == extent.e_lblk + extent.e_len - 1)
    {
      if (physical)
	{
	  if (has_next && logical == next_extent.e_lblk - 1
	      && physical == next_extent.e_pblk - 1
	      && new_uninit == next_uninit
	      && (int) next_extent.e_len < max_len - 1)
	    {
	      ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_LEAF,
				     &next_extent);
	      if (ret)
		goto end;
	      next_extent.e_len++;
	      next_extent.e_lblk--;
	      next_extent.e_pblk--;
	      ret = ext3_extent_replace (handle, 0, &next_extent);
	    }
	  else
	    ret = ext3_extent_insert (handle, EXT2_EXTENT_INSERT_AFTER,
				      &new_extent);
	  if (ret)
	    goto end;
	  ret = ext3_extent_fix_parents (handle);
	  if (ret)
	    goto end;

	  ret = ext3_extent_goto (handle, 0, logical);
	  if (ret)
	    goto end;
	  ret = ext3_extent_get (handle, EXT2_EXTENT_CURRENT, &extent);
	  if (ret)
	    goto end;
	}
      extent.e_len--;
      ret = ext3_extent_replace (handle, 0, &extent);
      if (ret)
	goto end;
    }
  else if (logical == extent.e_lblk)
    {
      if (physical)
	{
	  if (has_prev && logical == prev_extent.e_lblk + prev_extent.e_len
	      && physical == prev_extent.e_pblk + prev_extent.e_len
	      && new_uninit == prev_uninit
	      && (int) prev_extent.e_len < max_len - 1)
	    {
	      ret = ext3_extent_get (handle, EXT2_EXTENT_PREV_LEAF,
				     &prev_extent);
	      if (ret)
		goto end;
	      prev_extent.e_len++;
	      ret = ext3_extent_replace (handle, 0, &prev_extent);
	    }
	  else
	    ret = ext3_extent_insert (handle, 0, &new_extent);
	  if (ret)
	    goto end;
	  ret = ext3_extent_fix_parents (handle);
	  if (ret)
	    goto end;
	  ret = ext3_extent_get (handle, EXT2_EXTENT_NEXT_LEAF, &extent);
	  if (ret)
	    goto end;
	}
      extent.e_pblk++;
      extent.e_lblk--;
      extent.e_len--;
      ret = ext3_extent_replace (handle, 0, &extent);
      if (ret)
	goto end;
      ret = ext3_extent_fix_parents (handle);
      if (ret)
	goto end;
    }
  else
    {
      struct ext3_generic_extent save_extent = extent;
      unsigned int save_len = extent.e_len;
      block_t save_block = extent.e_lblk;
      int ret2;
      extent.e_len = logical - extent.e_lblk;
      ret = ext3_extent_replace (handle, 0, &extent);
      if (ret)
	goto end;
      if (physical)
	{
	  ret = ext3_extent_insert (handle, EXT2_EXTENT_INSERT_AFTER,
				    &new_extent);
	  if (ret)
	    {
	      ret2 = ext3_extent_goto (handle, 0, save_block);
	      if (!ret2)
		ext3_extent_replace (handle, 0, &save_extent);
	      goto end;
	    }
	}
      extent.e_pblk += extent.e_len + 1;
      extent.e_lblk += extent.e_len + 1;
      extent.e_len = save_len - extent.e_len - 1;
      ret = ext3_extent_insert (handle, EXT2_EXTENT_INSERT_AFTER, &extent);
      if (ret)
	{
	  if (physical)
	    {
	      ret2 = ext3_extent_goto (handle, 0, new_extent.e_lblk);
	      if (!ret2)
		ext3_extent_delete (handle, 0);
	    }
	  ret2 = ext3_extent_goto (handle, 0, save_block);
	  if (!ret2)
	    ext3_extent_replace (handle, 0, &save_extent);
	  goto end;
	}
    }

 end:
  if (orig_height > handle->max_depth)
    orig_height = handle->max_depth;
  ext3_extent_goto (handle, orig_height, orig_block);
  return ret;
}

void
ext3_extent_free (struct ext3_extent_handle *handle)
{
  int i;
  if (!handle)
    return;
  if (handle->path)
    {
      for (i = 1; i < handle->max_paths; i++)
	{
	  if (handle->path[i].buffer)
	    free (handle->path[i].buffer);
	}
      free (handle->path);
    }
  free (handle);
}
