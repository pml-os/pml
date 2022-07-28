/* mount.h -- This file is part of PML.
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

#ifndef __PML_MOUNT_H
#define __PML_MOUNT_H

/*!
 * @file
 * @brief Filesystem mount definitions
 */

/* Mount flags */

#define MS_RDONLY               (1 << 0)    /*!< Filesystem is read-only */
#define MS_NOSUID               (1 << 1)    /*!< Ignore suid and sgid bits */
#define MS_NODEV                (1 << 2)    /*!< Cannot access device files */
#define MS_NOEXEC               (1 << 3)    /*!< Cannot execute programs */
#define MS_SYNCHRONOUS          (1 << 4)    /*!< Synchronize writes instantly */
#define MS_REMOUNT              (1 << 5)    /*!< Alter flags of mounted fs */
#define MS_MANDLOCK             (1 << 6)    /*!< Allow mandatory locks */
#define MS_DIRSYNC              (1 << 7)    /*!< Synchronize dir changes */
#define MS_NOATIME              (1 << 8)    /*!< Do not update access times */
#define MS_NODIRATIME           (1 << 9)    /*!< Do not update dir atimes */
#define MS_BIND                 (1 << 10)   /*!< Bind mount */
#define MS_MOVE                 (1 << 11)   /*!< Move existing mount */
#define MS_REC                  (1 << 12)   /*!< Recurse propagation type */
#define MS_SILENT               (1 << 13)   /*!< Do not display warnings */
#define MS_POSIXACL             (1 << 14)   /*!< VFS does not apply umask */
#define MS_UNBINDABLE           (1 << 15)   /*!< Change to unbindable */
#define MS_PRIVATE              (1 << 16)   /*!< Change to private */
#define MS_SLAVE                (1 << 17)   /*!< Change to slave */
#define MS_SHARED               (1 << 18)   /*!< Change to shared */
#define MS_RELATIME             (1 << 19)   /*!< Update atime relatively */
#define MS_STRICTATIME          (1 << 20)   /*!< Always update atime */

/*! Flags that can be altered by @ref MS_REMOUNT */
#define MS_RMT_MASK (MS_RDONLY | MS_SYNCHRONOUS | MS_MANDLOCK)

#endif
