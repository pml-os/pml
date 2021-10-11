/* super.c -- This file is part of PML.
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

/*!
 * Reads one or more consecutive blocks from an ext2 filesystem.
 *
 * @param buffer buffer to store read data
 * @param fs the filesystem instance
 * @param block the starting block number to read
 * @param num the number of blocks to read
 * @return zero on success
 */

int
ext2_read_blocks (void *buffer, struct ext2_fs *fs, block_t block, size_t num)
{
  return -!(fs->device->read (fs->device, buffer, num * fs->block_size,
			      block * fs->block_size, 1) ==
	    (ssize_t) (num * fs->block_size));
}

/*!
 * Writes one or more consecutive blocks to an ext2 filesystem.
 *
 * @param buffer buffer containing the data to write
 * @param fs the filesystem instance
 * @param block the starting block number to write
 * @param num the number of blocks to write
 * @return zero on success
 */

int
ext2_write_blocks (const void *buffer, struct ext2_fs *fs, block_t block,
		   size_t num)
{
  return -!(fs->device->write (fs->device, buffer, num * fs->block_size,
			       block * fs->block_size, 1) ==
	    (ssize_t) (num * fs->block_size));
}
