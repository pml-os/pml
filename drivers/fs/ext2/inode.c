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

#include <pml/ext2fs.h>
#include <string.h>

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
