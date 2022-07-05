/* checksum.c -- This file is part of PML.
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
#include <pml/hash.h>
#include <pml/memory.h>
#include <errno.h>
#include <string.h>

static int
ext2_get_dirent_tail (struct ext2_fs *fs, struct ext2_dirent *dirent,
		      struct ext2_dirent_tail **tail)

{
  struct ext2_dirent *d = dirent;
  struct ext2_dirent_tail *t;
  void *top = EXT2_DIRENT_TAIL (dirent, fs->blksize);

  while ((void *) d < top)
    {
      if (d->d_rec_len < 8 || (d->d_rec_len & 3))
	RETV_ERROR (EUCLEAN, -1);
      d = (struct ext2_dirent *) ((char *) d + d->d_rec_len);
    }
  if ((char *) d > (char *) dirent + fs->blksize)
    RETV_ERROR (EUCLEAN, -1);
  if (d != top)
    RETV_ERROR (ENOSPC, -1);

  t = (struct ext2_dirent_tail *) d;
  if (t->det_reserved_zero1
      || t->det_rec_len != sizeof (struct ext2_dirent_tail)
      || t->det_reserved_name_len != EXT2_DIR_NAME_CHECKSUM)
    RETV_ERROR (ENOSPC, -1);
  if (tail)
    *tail = t;
  return 0;
}

static int
ext2_get_dx_count_limit (struct ext2_fs *fs, struct ext2_dirent *dirent,
			 struct ext2_dx_countlimit **cl, int *offset)
{
  struct ext2_dirent *d;
  struct ext2_dx_root_info *root;
  struct ext2_dx_countlimit *c;
  int count_offset;
  int max_entries;
  unsigned int rec_len = dirent->d_rec_len;

  if (rec_len == fs->blksize && !dirent->d_name_len)
    count_offset = 8;
  else if (rec_len == 12)
    {
      d = (struct ext2_dirent *) ((char *) dirent + 12);
      rec_len = d->d_rec_len;
      if (rec_len != fs->blksize - 12)
	RETV_ERROR (EUCLEAN, -1);
      root = (struct ext2_dx_root_info *) ((char *) d + 12);
      if (root->reserved_zero
	  || root->info_length != sizeof (struct ext2_dx_root_info))
	RETV_ERROR (EUCLEAN, -1);
      count_offset = 32;
    }
  else
    RETV_ERROR (EUCLEAN, -1);

  c = (struct ext2_dx_countlimit *) ((char *) dirent + count_offset);
  max_entries = (fs->blksize - count_offset) / sizeof (struct ext2_dx_entry);
  if (c->limit > max_entries || c->count > max_entries)
    RETV_ERROR (ENOSPC, -1);

  if (offset)
    *offset = count_offset;
  if (cl)
    *cl = c;
  return 0;
}

uint32_t
ext2_superblock_checksum (struct ext2_super *s)
{
  int offset = offsetof (struct ext2_super, s_checksum);
  return crc32 (0xffffffff, s, offset);
}

int
ext2_superblock_checksum_valid (struct ext2_fs *fs)
{
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;
  return fs->super.s_checksum == ext2_superblock_checksum (&fs->super);
}

void
ext2_superblock_checksum_update (struct ext2_fs *fs, struct ext2_super *s)
{
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return;
  s->s_checksum = ext2_superblock_checksum (s);
}

uint16_t
ext2_bg_checksum (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  return gdp->bg_checksum;
}

void
ext2_bg_checksum_update (struct ext2_fs *fs, unsigned int group,
			 uint16_t checksum)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  gdp->bg_checksum = checksum;
}

uint16_t
ext2_group_desc_checksum (struct ext2_fs *fs, unsigned int group)
{
  struct ext2_group_desc *desc = ext2_group_desc (fs, fs->group_desc, group);
  size_t size = EXT2_DESC_SIZE (fs->super);
  size_t offset;
  uint16_t crc;
  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM)
    {
      uint16_t old_crc = desc->bg_checksum;
      uint32_t c32;
      desc->bg_checksum = 0;
      c32 = crc32 (fs->checksum_seed, &group, sizeof (unsigned int));
      c32 = crc32 (c32, desc, size);
      desc->bg_checksum = old_crc;
      crc = c32 & 0xffff;
    }
  else
    {
      offset = offsetof (struct ext2_group_desc, bg_checksum);
      crc = crc16 (0xffff, fs->super.s_uuid, 16);
      crc = crc16 (crc, &group, sizeof (unsigned int));
      crc = crc16 (crc, desc, offset);
      offset += sizeof (desc->bg_checksum);
      if (offset < size)
	crc = crc16 (crc, (char *) desc + offset, size - offset);
    }
  return crc;
}

int
ext2_group_desc_checksum_valid (struct ext2_fs *fs, unsigned int group)
{
  if (ext2_has_group_desc_checksum (&fs->super)
      && ext2_bg_checksum (fs, group) != ext2_group_desc_checksum (fs, group))
    return 0;
  return 1;
}

void
ext2_group_desc_checksum_update (struct ext2_fs *fs, unsigned int group)
{
  if (!ext2_has_group_desc_checksum (&fs->super))
    return;
  ext2_bg_checksum_update (fs, group, ext2_group_desc_checksum (fs, group));
}

uint32_t
ext2_inode_checksum (struct ext2_fs *fs, ino_t ino,
		     struct ext2_large_inode *inode, int has_hi)
{
  uint32_t gen;
  size_t size = EXT2_INODE_SIZE (fs->super);
  uint16_t old_lo;
  uint16_t old_hi = 0;
  uint32_t crc;

  /* Set checksum fields to zero */
  old_lo = inode->i_checksum_lo;
  inode->i_checksum_lo = 0;
  if (has_hi)
    {
      old_hi = inode->i_checksum_hi;
      inode->i_checksum_hi = 0;
    }

  /* Calculate checksum */
  gen = inode->i_generation;
  crc = crc32 (fs->checksum_seed, &ino, sizeof (ino_t));
  crc = crc32 (crc, &gen, 4);
  crc = crc32 (crc, inode, size);

  /* Restore inode checksum fields */
  inode->i_checksum_lo = old_lo;
  if (has_hi)
    inode->i_checksum_hi = old_hi;
  return crc;
}

int
ext2_inode_checksum_valid (struct ext2_fs *fs, ino_t ino,
			   struct ext2_large_inode *inode)
{
  uint32_t crc;
  uint32_t provided;
  unsigned int i;
  unsigned int has_hi;
  char *ptr;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;

  has_hi = EXT2_INODE_SIZE (fs->super) > EXT2_OLD_INODE_SIZE
    && inode->i_extra_isize >= EXT4_INODE_CSUM_HI_EXTRA_END;
  provided = inode->i_checksum_lo;
  crc = ext2_inode_checksum (fs, ino, inode, has_hi);

  if (has_hi)
    provided |= inode->i_checksum_hi << 16;
  else
    crc &= 0xffff;
  if (provided == crc)
    return 1;

  /* Check if inode is all zeros */
  for (ptr = (char *) inode, i = 0; i < sizeof (struct ext2_inode); ptr++, i++)
    {
      if (*ptr)
	return 0;
    }
  return 1;
}

void
ext2_inode_checksum_update (struct ext2_fs *fs, ino_t ino,
			    struct ext2_large_inode *inode)
{
  uint32_t crc;
  int has_hi;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return;

  has_hi = EXT2_INODE_SIZE (fs->super) > EXT2_OLD_INODE_SIZE
    && inode->i_extra_isize >= EXT4_INODE_CSUM_HI_EXTRA_END;
  crc = ext2_inode_checksum (fs, ino, inode, has_hi);

  inode->i_checksum_lo = crc & 0xffff;
  if (has_hi)
    inode->i_checksum_hi = crc >> 16;
}

int
ext3_extent_block_checksum (struct ext2_fs *fs, ino_t ino,
			    struct ext3_extent_header *eh, uint32_t *crc)
{
  struct ext2_inode inode;
  uint32_t gen;
  size_t size = EXT2_EXTENT_TAIL_OFFSET (eh) +
    offsetof (struct ext3_extent_tail, et_checksum);
  int ret = ext2_read_inode (fs, ino, &inode);
  if (ret)
    return ret;
  gen = inode.i_generation;
  *crc = crc32 (fs->checksum_seed, &ino, sizeof (ino_t));
  *crc = crc32 (*crc, &gen, 4);
  *crc = crc32 (*crc, eh, size);
  return 0;
}

int
ext3_extent_block_checksum_valid (struct ext2_fs *fs, ino_t ino,
				  struct ext3_extent_header *eh)
{
  struct ext3_extent_tail *t = EXT2_EXTENT_TAIL (eh);
  uint32_t provided;
  uint32_t crc;
  int ret;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;

  provided = t->et_checksum;
  ret = ext3_extent_block_checksum (fs, ino, eh, &crc);
  if (ret)
    return 0;
  return provided == crc;
}

int
ext3_extent_block_checksum_update (struct ext2_fs *fs, ino_t ino,
				   struct ext3_extent_header *eh)
{
  struct ext3_extent_tail *t = EXT2_EXTENT_TAIL (eh);
  uint32_t crc;
  int ret;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;

  ret = ext3_extent_block_checksum (fs, ino, eh, &crc);
  if (ret)
    return ret;
  t->et_checksum = crc;
  return 0;
}

uint32_t
ext2_block_bitmap_checksum (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  uint32_t checksum = gdp->bg_block_bitmap_csum_lo;
  if (EXT2_DESC_SIZE (fs->super) >= EXT4_BG_BLOCK_BITMAP_CSUM_HI_END)
    checksum |= (uint32_t) gdp->bg_block_bitmap_csum_hi << 16;
  return checksum;
}

int
ext2_block_bitmap_checksum_valid (struct ext2_fs *fs, unsigned int group,
				  char *bitmap, int size)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  uint32_t provided;
  uint32_t crc;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;

  provided = gdp->bg_block_bitmap_csum_lo;
  crc = crc32 (fs->checksum_seed, bitmap, size);

  if (EXT2_DESC_SIZE (fs->super) >= EXT4_BG_BLOCK_BITMAP_CSUM_HI_END)
    provided |= (uint32_t) gdp->bg_block_bitmap_csum_hi << 16;
  else
    crc &= 0xffff;
  return provided == crc;
}

void
ext2_block_bitmap_checksum_update (struct ext2_fs *fs, unsigned int group,
				   char *bitmap, int size)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  uint32_t crc;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return;

  crc = crc32 (fs->checksum_seed, bitmap, size);
  gdp->bg_block_bitmap_csum_lo = crc & 0xffff;
  if (EXT2_DESC_SIZE (fs->super) >= EXT4_BG_BLOCK_BITMAP_CSUM_HI_END)
    gdp->bg_block_bitmap_csum_hi = crc >> 16;
}

uint32_t
ext2_inode_bitmap_checksum (struct ext2_fs *fs, unsigned int group)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  uint32_t checksum = gdp->bg_inode_bitmap_csum_lo;
  if (EXT2_DESC_SIZE (fs->super) >= EXT4_BG_BLOCK_BITMAP_CSUM_HI_END)
    checksum |= (uint32_t) gdp->bg_inode_bitmap_csum_hi << 16;
  return checksum;
}

int
ext2_inode_bitmap_checksum_valid (struct ext2_fs *fs, unsigned int group,
				  char *bitmap, int size)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  uint32_t provided;
  uint32_t crc;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;

  provided = gdp->bg_inode_bitmap_csum_lo;
  crc = crc32 (fs->checksum_seed, bitmap, size);

  if (EXT2_DESC_SIZE (fs->super) >= EXT4_BG_INODE_BITMAP_CSUM_HI_END)
    provided |= (uint32_t) gdp->bg_inode_bitmap_csum_hi << 16;
  else
    crc &= 0xffff;
  return provided == crc;
}

void
ext2_inode_bitmap_checksum_update (struct ext2_fs *fs, unsigned int group,
				   char *bitmap, int size)
{
  struct ext4_group_desc *gdp = ext4_group_desc (fs, fs->group_desc, group);
  uint32_t crc;
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return;

  crc = crc32 (fs->checksum_seed, bitmap, size);
  gdp->bg_inode_bitmap_csum_lo = crc & 0xffff;
  if (EXT2_DESC_SIZE (fs->super) >= EXT4_BG_INODE_BITMAP_CSUM_HI_END)
    gdp->bg_inode_bitmap_csum_hi = crc >> 16;
}

uint32_t
ext2_dirent_checksum (struct ext2_fs *fs, struct vnode *dir,
		      struct ext2_dirent *dirent, size_t size)
{
  struct ext2_file *file = dir->data;
  uint32_t gen = file->inode.i_generation;
  uint32_t crc = crc32 (fs->checksum_seed, &file->ino, sizeof (ino_t));
  crc = crc32 (crc, &gen, 4);
  crc = crc32 (crc, dirent, size);
  return crc;
}

int
ext2_dirent_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
			    struct ext2_dirent *dirent)
{
  struct ext2_dirent_tail *t;
  int ret = ext2_get_dirent_tail (fs, dirent, &t);
  if (ret)
    return 1;
  return t->det_checksum ==
    ext2_dirent_checksum (fs, dir, dirent, (char *) t - (char *) dirent);
}

int
ext2_dirent_checksum_update (struct ext2_fs *fs, struct vnode *dir,
			     struct ext2_dirent *dirent)
{
  struct ext2_dirent_tail *t;
  int ret = ext2_get_dirent_tail (fs, dirent, &t);
  if (ret)
    return 1;
  t->det_checksum =
    ext2_dirent_checksum (fs, dir, dirent, (char *) t - (char *) dirent);
  return 0;
}

int
ext2_dx_checksum (struct ext2_fs *fs, struct vnode *dir,
		  struct ext2_dirent *dirent, uint32_t *crc,
		  struct ext2_dx_tail **tail)
{
  struct ext2_file *file = dir->data;
  struct ext2_dx_tail *t;
  struct ext2_dx_countlimit *c;
  uint32_t dummy_checksum = 0;
  size_t size;
  uint32_t gen;
  int count_offset;
  int limit;
  int count;
  int ret = ext2_get_dx_count_limit (fs, dirent, &c, &count_offset);
  if (ret)
    return ret;
  limit = c->limit;
  count = c->count;
  if (count_offset + limit * sizeof (struct ext2_dx_entry) > fs->blksize -
      sizeof (struct ext2_dx_tail))
    RETV_ERROR (ENOSPC, -1);

  t = (struct ext2_dx_tail *) ((struct ext2_dx_entry *) c + limit);
  size = count_offset + count * sizeof (struct ext2_dx_entry);
  gen = file->inode.i_generation;

  *crc = crc32 (fs->checksum_seed, &file->ino, sizeof (ino_t));
  *crc = crc32 (*crc, &gen, 4);
  *crc = crc32 (*crc, dirent, size);
  *crc = crc32 (*crc, t, 4);
  *crc = crc32 (*crc, &dummy_checksum, 4);

  if (tail)
    *tail = t;
  return 0;
}

int
ext2_dx_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
			struct ext2_dirent *dirent)
{
  struct ext2_dx_tail *t;
  uint32_t crc;
  int ret = ext2_dx_checksum (fs, dir, dirent, &crc, &t);
  if (ret)
    return 0;
  return t->dt_checksum == crc;
}

int
ext2_dx_checksum_update (struct ext2_fs *fs, struct vnode *dir,
			 struct ext2_dirent *dirent)
{
  struct ext2_dx_tail *t;
  uint32_t crc;
  int ret = ext2_dx_checksum (fs, dir, dirent, &crc, &t);
  if (ret)
    return ret;
  t->dt_checksum = crc;
  return 0;
}

int
ext2_dir_block_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
			       struct ext2_dirent *dirent)
{
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 1;
  if (!ext2_get_dirent_tail (fs, dirent, NULL))
    return ext2_dirent_checksum_valid (fs, dir, dirent);
  if (!ext2_get_dx_count_limit (fs, dirent, NULL, NULL))
    return ext2_dx_checksum_valid (fs, dir, dirent);
  return 0;
}

int
ext2_dir_block_checksum_update (struct ext2_fs *fs, struct vnode *dir,
				struct ext2_dirent *dirent)
{
  if (!(fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM))
    return 0;
  if (!ext2_get_dirent_tail (fs, dirent, NULL))
    return ext2_dirent_checksum_update (fs, dir, dirent);
  if (!ext2_get_dx_count_limit (fs, dirent, NULL, NULL))
    return ext2_dx_checksum_update (fs, dir, dirent);
  RETV_ERROR (ENOSPC, -1);
}
