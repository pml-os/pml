/* statvfs.h -- This file is part of PML.
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

#ifndef __PML_STATVFS_H
#define __PML_STATVFS_H

/*!
 * @file
 * @brief Filesystem information structures
 */

#include <pml/types.h>

/*!
 * Contains information about a mounted filesystem.
 */

struct statvfs
{
  unsigned long f_bsize;        /*!< Filesystem block size */
  unsigned long f_frsize;       /*!< Fragment size */
  fsblkcnt_t f_blocks;          /*!< Filesystem size in fragments */
  fsblkcnt_t f_bfree;           /*!< Number of free blocks */
  fsblkcnt_t f_bavail;          /*!< Number of unreserved free users */
  fsfilcnt_t f_files;           /*!< Number of inodes */
  fsfilcnt_t f_ffree;           /*!< Number of free inodes */
  fsfilcnt_t f_favail;          /*!< Number of unreserved inodes */
  unsigned long f_fsid;         /*!< Filesystem ID */
  unsigned long f_flag;         /*!< Mount flags */
  unsigned long f_namemax;      /*!< Maximum file name length */
};

#define ST_RDONLY               (1 << 0)    /*!< Read-only filesystem */
#define ST_NOSUID               (1 << 1)    /*!< Setuid and setgid ignored */

#endif
