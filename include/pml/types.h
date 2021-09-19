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

#include <stddef.h>
#include <stdint.h>

/* POSIX types in kernel space. These are prefixed to avoid name collisions
   if this header is included in user applications. */

struct pml_fsid_t
{
  int __val[2];
};

typedef unsigned long           pml_dev_t;
typedef unsigned int            pml_uid_t;
typedef unsigned int            pml_gid_t;
typedef unsigned long           pml_ino_t;
typedef unsigned int            pml_mode_t;
typedef unsigned long           pml_nlink_t;
typedef long                    pml_fsword_t;
typedef long                    pml_off_t;
typedef int                     pml_pid_t;
typedef unsigned long           pml_rlim_t;
typedef long                    pml_blkcnt_t;
typedef unsigned long           pml_fsblkcnt_t;
typedef unsigned long           pml_fsfilcnt_t;
typedef unsigned int            pml_id_t;
typedef long                    pml_clock_t;
typedef long                    pml_time_t;
typedef unsigned int            pml_useconds_t;
typedef long                    pml_suseconds_t;
typedef int                     pml_key_t;
typedef int                     pml_clockid_t;
typedef void *                  pml_timer_t;
typedef long                    pml_blksize_t;
typedef long                    pml_ssize_t;
typedef struct pml_fsid_t       pml_fsid_t;

/* If included in kernel code, define the real type names for convenience. */

#ifdef PML_KERNEL

typedef pml_dev_t               dev_t;
typedef pml_uid_t               uid_t;
typedef pml_gid_t               gid_t;
typedef pml_ino_t               ino_t;
typedef pml_mode_t              mode_t;
typedef pml_nlink_t             nlink_t;
typedef pml_fsword_t            fsword_t;
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

#endif /* PML_KERNEL */

#endif
