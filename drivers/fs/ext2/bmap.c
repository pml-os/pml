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
