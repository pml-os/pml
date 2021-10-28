/* stat.h -- This file is part of PML.
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

#ifndef __PML_STAT_H
#define __PML_STAT_H

/*!
 * @file
 * @brief File information structures
 */

#include <pml/time.h>

#define S_IFMT                0170000   /*!< Bits for file type */
#define S_IFIFO               0010000   /*!< Named pipe */
#define S_IFCHR               0020000   /*!< Character device */
#define S_IFDIR               0040000   /*!< Directory */
#define S_IFBLK               0060000   /*!< Block device */
#define S_IFREG               0100000   /*!< Regular file */
#define S_IFLNK               0120000   /*!< Symbolic link */
#define S_IFSOCK              0140000   /*!< Socket */

#define S_IRUSR               00400     /*!< Read permission for owner */
#define S_IWUSR               00200     /*!< Write permission for owner */
#define S_IXUSR               00100     /*!< Execute permission for owner */

#define S_IRGRP               00040     /*!< Read permission for group */
#define S_IWGRP               00020     /*!< Write permission for group */
#define S_IXGRP               00010     /*!< Execute permission for group */

#define S_IROTH               00004     /*!< Read permission for others */
#define S_IWOTH               00002     /*!< Write permission for others */
#define S_IXOTH               00001     /*!< Execute permission for others */

/*! Full permissions for user */
#define S_IRWXU               (S_IRUSR | S_IWUSR | S_IXUSR)
/*! Full permissions for group */
#define S_IRWXG               (S_IRGRP | S_IWGRP | S_IXGRP)
/*! Full permissions for others */
#define S_IRWXO               (S_IROTH | S_IWOTH | S_IXOTH)

#define S_ISUID               04000     /*!< Change user ID to owner */
#define S_ISGID               02000     /*!< Change group ID to owner */
#define S_ISVTX               01000     /*!< Sticky bit */

#define S_ISBLK(x)            (((x) & S_IFMT) == S_IFBLK)
#define S_ISCHR(x)            (((x) & S_IFMT) == S_IFCHR)
#define S_ISDIR(x)            (((x) & S_IFMT) == S_IFDIR)
#define S_ISFIFO(x)           (((x) & S_IFMT) == S_IFIFO)
#define S_ISREG(x)            (((x) & S_IFMT) == S_IFREG)
#define S_ISLNK(x)            (((x) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(x)           (((x) & S_IFMT) == S_IFSOCK)

struct stat
{
  mode_t st_mode;               /*!< File type and permissions */
  nlink_t st_nlink;             /*!< Number of hard links to file */
  ino_t st_ino;                 /*!< Inode number of file */
  uid_t st_uid;                 /*!< User ID of file owner */
  gid_t st_gid;                 /*!< Group ID of file owner */
  dev_t st_dev;                 /*!< Device number of device containing file */
  dev_t st_rdev;                /*!< Device number (for special device files) */
  struct timespec st_atim;      /*!< Time of last access */
  struct timespec st_mtim;      /*!< Time of last data modification */
  struct timespec st_ctim;      /*!< Time of last metadata modification */
  size_t st_size;               /*!< Number of bytes in file */
  blkcnt_t st_blocks;           /*!< Number of blocks allocated to file */
  blksize_t st_blksize;         /*!< Optimal I/O block size */
};

#define st_atime                st_atim.tv_sec
#define st_ctime                st_ctim.tv_sec
#define st_mtime                st_mtim.tv_sec

#endif
