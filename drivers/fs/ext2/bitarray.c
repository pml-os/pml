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
#include <stdlib.h>

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
  itn ret = ext2_bitarray_alloc_private (bmap);
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
