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
 * @brief Defines POSIX types and other helper types
 */

#include <stddef.h>
#include <stdint.h>

/* POSIX types in kernel space. These are prefixed to avoid name collisions
   if this header is included in user applications. */

struct pml_fsid_t
{
  int __val[2];
};

typedef unsigned long           pml_dev_t;        /*!< Device number */
typedef unsigned int            pml_uid_t;        /*!< User ID */
typedef unsigned int            pml_gid_t;        /*!< Group ID */
typedef unsigned long           pml_ino_t;        /*!< Inode number */
typedef unsigned int            pml_mode_t;       /*!< Type and permissions */
typedef unsigned long           pml_nlink_t;      /*!< Hard link count */
typedef long                    pml_off_t;        /*!< File offset */
typedef int                     pml_pid_t;        /*!< Process ID */
typedef unsigned long           pml_rlim_t;       /*!< Hard limit */
typedef long                    pml_blkcnt_t;     /*!< File block count */
typedef unsigned long           pml_fsblkcnt_t;   /*!< Filesystem block count */
typedef unsigned long           pml_fsfilcnt_t;   /*!< Filesystem file count */
typedef unsigned int            pml_id_t;         /*!< Generic ID */
typedef long                    pml_clock_t;      /*!< Clock ticks */
typedef long                    pml_time_t;       /*!< Timestamp value */
typedef unsigned int            pml_useconds_t;   /*!< Microsecond value */
typedef long                    pml_suseconds_t;  /*!< Signed microseconds */
typedef int                     pml_key_t;        /*!< Key number */
typedef int                     pml_clockid_t;    /*!< Clock ID */
typedef void *                  pml_timer_t;      /*!< Timer number */
typedef long                    pml_blksize_t;    /*!< Block size */
typedef long                    pml_ssize_t;      /*!< Signed generic size */
typedef struct pml_fsid_t       pml_fsid_t;       /*!< Filesystem ID */

/* GCC 128-bit integer extensions */

#ifdef __GNUC__

typedef __int128_t              pml_int128_t;
typedef __uint128_t             pml_uint128_t;

#endif /* __GNUC__ */

/* If included in kernel code, define the real type names for convenience. */

#ifdef PML_KERNEL

typedef pml_dev_t               dev_t;
typedef pml_uid_t               uid_t;
typedef pml_gid_t               gid_t;
typedef pml_ino_t               ino_t;
typedef pml_mode_t              mode_t;
typedef pml_nlink_t             nlink_t;
typedef pml_off_t               off_t;
typedef pml_pid_t               pid_t;
typedef pml_rlim_t              rlim_t;
typedef pml_blkcnt_t            blkcnt_t;
typedef pml_fsblkcnt_t          fsblkcnt_t;
typedef pml_fsfilcnt_t          fsfilcnt_t;
typedef pml_id_t                id_t;
typedef pml_clock_t             clock_t;
typedef pml_time_t              time_t;
typedef pml_useconds_t          useconds_t;
typedef pml_suseconds_t         suseconds_t;
typedef pml_key_t               key_t;
typedef pml_clockid_t           clockid_t;
typedef pml_timer_t             timer_t;
typedef pml_blksize_t           blksize_t;
typedef pml_ssize_t             ssize_t;
typedef pml_fsid_t              fsid_t;

#ifdef __GNUC__

typedef pml_int128_t            int128_t;
typedef pml_uint128_t           uint128_t;

#endif /* __GNUC__ */

#endif /* PML_KERNEL */


/* Kernel versions of user-space structures, used for system calls. System
   calls expect these structures to be used instead of any user-space ones,
   and C libraries providing wrapper functions must translate the fields in
   these structures to the user-space structures. */

/*!
 * Kernel equivalent of `struct timespec'. Represents a calendar time or
 * elapsed time with nanosecond resolution.
 */

struct pml_timespec
{
  time_t sec;                   /*!< Seconds elapsed */
  long nsec;                    /*!< Additional nanoseconds elapsed */
};

/*!
 * Kernel equivalent of `struct stat'. Stores information about a file.
 */

struct pml_stat
{
  mode_t mode;                  /*!< File type and permissions */
  nlink_t nlink;                /*!< Number of hard links to file */
  ino_t ino;                    /*!< Inode number of file */
  uid_t uid;                    /*!< User ID of file owner */
  gid_t gid;                    /*!< Group ID of file owner */
  dev_t dev;                    /*!< Device number of device containing file */
  dev_t rdev;                   /*!< Device number (for special device files) */
  struct pml_timespec atime;    /*!< Time of last access */
  struct pml_timespec mtime;    /*!< Time of last data modification */
  struct pml_timespec ctime;    /*!< Time of last metadata modification */
  size_t size;                  /*!< Number of bytes in file */
  blkcnt_t blocks;              /*!< Number of blocks allocated to file */
  blksize_t blksize;            /*!< Optimal I/O block size */
};

struct pml_dirent
{
  ino_t ino;
  uint16_t reclen;
  uint16_t namlen;
  unsigned char type;
  char name[256];
};

#endif
