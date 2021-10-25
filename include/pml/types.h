/* types.h -- This file is part of PML.
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

#ifndef __PML_TYPES_H
#define __PML_TYPES_H

/*!
 * @file
 * @brief POSIX type definitions
 */

#include <stddef.h>
#include <stdint.h>

struct fsid_t
{
  int __val[2];
};

typedef unsigned long           dev_t;        /*!< Device number */
typedef unsigned int            uid_t;        /*!< User ID */
typedef unsigned int            gid_t;        /*!< Group ID */
typedef unsigned long           ino_t;        /*!< Inode number */
typedef unsigned int            mode_t;       /*!< Type and permissions */
typedef unsigned long           nlink_t;      /*!< Hard link count */
typedef long                    off_t;        /*!< File offset */
typedef int                     pid_t;        /*!< Process ID */
typedef unsigned long           rlim_t;       /*!< Hard limit */
typedef long                    blkcnt_t;     /*!< File block count */
typedef unsigned long           fsblkcnt_t;   /*!< Filesystem block count */
typedef unsigned long           fsfilcnt_t;   /*!< Filesystem file count */
typedef unsigned int            id_t;         /*!< Generic ID */
typedef long                    clock_t;      /*!< Clock ticks */
typedef long                    time_t;       /*!< Timestamp value */
typedef unsigned int            useconds_t;   /*!< Microsecond value */
typedef long                    suseconds_t;  /*!< Signed microseconds */
typedef int                     key_t;        /*!< Key number */
typedef int                     clockid_t;    /*!< Clock ID */
typedef void *                  timer_t;      /*!< Timer number */
typedef long                    blksize_t;    /*!< Block size */
typedef long                    ssize_t;      /*!< Signed generic size */
typedef struct fsid_t           fsid_t;       /*!< Filesystem ID */

/* GCC 128-bit integer extensions */

#ifdef __GNUC__

typedef __int128_t              int128_t;
typedef __uint128_t             uint128_t;

#endif /* __GNUC__ */

#endif
