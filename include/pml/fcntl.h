/* fcntl.h -- This file is part of PML.
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

#ifndef __PML_FCNTL_H
#define __PML_FCNTL_H

/*!
 * @file
 * @brief Macros for file descriptor system calls
 */

#define O_RDONLY                0
#define O_WRONLY                1
#define O_RDWR                  2
#define O_ACCMODE               3
#define O_APPEND                (1 << 2)
#define O_CREAT                 (1 << 3)
#define O_TRUNC                 (1 << 4)
#define O_EXCL                  (1 << 5)
#define O_SYNC                  (1 << 6)
#define O_NONBLOCK              (1 << 7)
#define O_NOCTTY                (1 << 8)
#define O_CLOEXEC               (1 << 9)
#define O_NOFOLLOW              (1 << 10)
#define O_DIRECTORY             (1 << 11)

#define SEEK_SET                0
#define SEEK_CUR                1
#define SEEK_END                2

#endif
