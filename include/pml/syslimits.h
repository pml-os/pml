/* syslimits.h -- This file is part of PML.
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

#ifndef __PML_SYSLIMITS_H
#define __PML_SYSLIMITS_H

/*!
 * @file
 * @brief System limit definitions
 */

#define ARG_MAX                 65536   /*!< Max number of bytes for exec */
#define CHILD_MAX               128     /*!< Max processes under same user ID */
#define LINK_MAX                128     /*!< Max file link count */
#define MAX_CANON               1024    /*!< Size of canonical input line */
#define MAX_INPUT               1024    /*!< Max bytes in terminal input line */
#define NAME_MAX                255     /*!< Max bytes in file name */
#define NGROUPS_MAX             16      /*!< Max supplemental group IDs */
#define OPEN_MAX                256     /*!< Max open files per process */
#define PATH_MAX                4096    /*!< Max bytes in path name */
#define PIPE_BUF                512     /*!< Max bytes for atomic pipe writes */
#define IOV_MAX                 1024    /*!< Max elements in I/O vector */
#define HOST_NAME_MAX           64      /*!< Max length of host name */
#define LINE_MAX                MAX_INPUT

#endif
