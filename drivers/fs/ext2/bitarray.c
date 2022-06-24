/* bitarray.c -- This file is part of PML.
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

/* Copyright (C) 2008 Theodore Ts'o.
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

struct ext2_bitarray_private
{
  char *bitarray;
};

static int
ext2_bitarray_alloc_private (struct ext2_bitmap64 *bmap)
{
  struct ext2_bitarray_private *priv =
    malloc (sizeof (struct ext2_bitarray_private));
  size_t size = (size_t) ((bmap->real_end - bmap->start) / 8 + 1);
  if (UNLIKELY (!priv))
    RETV_ERROR (ENOMEM, -1);
  priv->bitarray = malloc (size);
  if (UNLIKELY (!priv->bitarray))
    {
      free (priv);
      RETV_ERROR (ENOMEM, -1);
    }
  bmap->private = priv;
  return 0;
}

static int
ext2_bitarray_new_bmap (struct ext2_fs *fs, struct ext2_bitmap64 *bmap)
{
  struct ext2_bitarray_private *priv;
  size_t size;
  int ret = ext2_bitarray_alloc_private (bmap);
  if (ret)
    return ret;
  priv = bmap->private;
  size = (size_t) ((bmap->real_end - bmap->start) / 8 + 1);
  memset (priv->bitarray, 0, size);
  return 0;
}

static void
ext2_bitarray_free_bmap (struct ext2_bitmap64 *bmap)
{
  struct ext2_bitarray_private *priv = bmap->private;
  if (priv->bitarray)
    free (priv->bitarray);
  free (priv);
}

static void
ext2_bitarray_mark_bmap (struct ext2_bitmap64 *bmap, uint64_t arg)
{
  struct ext2_bitarray_private *priv = bmap->private;
  set_bit (priv->bitarray, arg - bmap->start);
}

static void
ext2_bitarray_unmark_bmap (struct ext2_bitmap64 *bmap, uint64_t arg)
{
  struct ext2_bitarray_private *priv = bmap->private;
  clear_bit (priv->bitarray, arg - bmap->start);
}

static int
ext2_bitarray_test_bmap (struct ext2_bitmap64 *bmap, uint64_t arg)
{
  struct ext2_bitarray_private *priv = bmap->private;
  return test_bit (priv->bitarray, arg - bmap->start);
}

static void
ext2_bitarray_mark_bmap_extent (struct ext2_bitmap64 *bmap, uint64_t arg,
				unsigned int num)
{
  struct ext2_bitarray_private *priv = bmap->private;
  unsigned int i;
  for (i = 0; i < num; i++)
    set_bit (priv->bitarray, arg + i - bmap->start);
}

static void
ext2_bitarray_unmark_bmap_extent (struct ext2_bitmap64 *bmap, uint64_t arg,
				  unsigned int num)
{
  struct ext2_bitarray_private *priv = bmap->private;
  unsigned int i;
  for (i = 0; i < num; i++)
    clear_bit (priv->bitarray, arg + i - bmap->start);
}

static void
ext2_bitarray_set_bmap_range (struct ext2_bitmap64 *bmap, uint64_t start,
			      size_t num, void *data)
{
  struct ext2_bitarray_private *priv = bmap->private;
  memcpy (priv->bitarray + (start >> 3), data, (num + 7) >> 3);
}

static void
ext2_bitarray_get_bmap_range (struct ext2_bitmap64 *bmap, uint64_t start,
			      size_t num, void *data)
{
  struct ext2_bitarray_private *priv = bmap->private;
  memcpy (data, priv->bitarray + (start >> 3), (num + 7) >> 3);
}

static int
ext2_bitarray_find_first_zero (struct ext2_bitmap64 *bmap, uint64_t start,
			       uint64_t end, uint64_t *result)
{
  struct ext2_bitarray_private *priv = bmap->private;
  const unsigned char *pos;
  unsigned long bitpos = start - bmap->start;
  unsigned long count = end - start + 1;
  unsigned long max_loop;
  unsigned long i;
  int found = 0;

  while ((bitpos & 7) && count > 0)
    {
      if (!test_bit (priv->bitarray, bitpos))
	{
	  *result = bitpos + bmap->start;
	  return 0;
	}
      bitpos++;
      count--;
    }
  if (!count)
    RETV_ERROR (ENOENT, -1);

  pos = (unsigned char *) priv->bitarray + (bitpos >> 3);
  while (count >= 8 && ((uintptr_t) pos & 7))
    {
      if (*pos != 0xff)
	{
	  found = 1;
	  break;
	}
      pos++;
      count -= 8;
      bitpos += 8;
    }
  if (!found)
    {
      max_loop = count >> 6;
      i = max_loop;
      while (i)
	{
	  if (*((uint64_t *) pos) != (uint64_t) -1)
	    break;
	  pos += 8;
	  i--;
	}
      count -= 64 * (max_loop - i);
      bitpos += 64 * (max_loop - i);

      max_loop = count >> 3;
      i = max_loop;
      while (i)
	{
	  if (*pos != 0xff)
	    {
	      found = 1;
	      break;
	    }
	  pos++;
	  i--;
	}
      count -= 8 * (max_loop - i);
      bitpos += 8 * (max_loop - i);
    }

  while (count-- > 0)
    {
      if (!test_bit (priv->bitarray, bitpos))
	{
	  *result = bitpos + bmap->start;
	  return 0;
	}
      bitpos++;
    }
  RETV_ERROR (ENOENT, -1);
}

static int
ext2_bitarray_find_first_set (struct ext2_bitmap64 *bmap, uint64_t start,
			      uint64_t end, uint64_t *result)
{
  struct ext2_bitarray_private *priv = bmap->private;
  const unsigned char *pos;
  unsigned long bitpos = start - bmap->start;
  unsigned long count = end - start + 1;
  unsigned long max_loop;
  unsigned long i;
  int found = 0;

  while ((bitpos & 7) != 0 && count > 0)
    {
      if (test_bit (priv->bitarray, bitpos))
	{
	  *result = bitpos + bmap->start;
	  return 0;
	}
      bitpos++;
      count--;
    }
  if (!count)
    RETV_ERROR (ENOENT, -1);

  pos = (unsigned char *) priv->bitarray + (bitpos >> 3);
  while (count >= 8 && ((uintptr_t) pos & 7))
    {
      if (*pos)
	{
	  found = 1;
	  break;
	}
      pos++;
      count -= 8;
      bitpos += 8;
    }
  if (!found)
    {
      max_loop = count >> 6;
      i = max_loop;
      while (i)
	{
	  if (*((uint64_t *) pos))
	    break;
	  pos += 8;
	  i--;
	}
      count -= 64 * (max_loop - i);
      bitpos += 64 * (max_loop - i);

      max_loop = count >> 3;
      i = max_loop;
      while (i != 0)
	{
	  if (*pos)
	    {
	      found = 1;
	      break;
	    }
	  pos++;
	  i--;
	}
      count -= 8 * (max_loop - i);
      bitpos += 8 * (max_loop - i);
    }

  while (count-- > 0)
    {
      if (test_bit (priv->bitarray, bitpos))
	{
	  *result = bitpos + bmap->start;
	  return 0;
	}
      bitpos++;
    }
  RETV_ERROR (ENOENT, -1);
}

struct ext2_bitmap_ops ext2_bitarray_ops = {
  .type = EXT2_BMAP64_BITARRAY,
  .new_bmap = ext2_bitarray_new_bmap,
  .free_bmap = ext2_bitarray_free_bmap,
  .mark_bmap = ext2_bitarray_mark_bmap,
  .unmark_bmap = ext2_bitarray_unmark_bmap,
  .test_bmap = ext2_bitarray_test_bmap,
  .mark_bmap_extent = ext2_bitarray_mark_bmap_extent,
  .unmark_bmap_extent = ext2_bitarray_unmark_bmap_extent,
  .set_bmap_range = ext2_bitarray_set_bmap_range,
  .get_bmap_range = ext2_bitarray_get_bmap_range,
  .find_first_zero = ext2_bitarray_find_first_zero,
  .find_first_set = ext2_bitarray_find_first_set
};
