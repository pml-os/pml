/* mman.h -- This file is part of PML.
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

#ifndef __PML_MMAN_H
#define __PML_MMAN_H

/*!
 * @file
 * @brief Memory mapping routines
 */

#define PROT_NONE               0           /*!< Memory is not accessible */
#define PROT_READ               (1 << 0)    /*!< Memory may be read */
#define PROT_WRITE              (1 << 1)    /*!< Memory may be written */
#define PROT_EXEC               (1 << 2)    /*!< Memory may be executed */

#define MAP_SHARED              (1 << 0)
#define MAP_PRIVATE             (1 << 1)
#define MAP_ANONYMOUS           (1 << 2)
#define MAP_ANON                MAP_ANONYMOUS
#define MAP_DENYWRITE           (1 << 3)
#define MAP_EXECUTABLE          (1 << 4)
#define MAP_FILE                (1 << 5)
#define MAP_FIXED               (1 << 6)
#define MAP_STACK               (1 << 7)

#define MS_ASYNC                (1 << 0)
#define MS_SYNC                 (1 << 1)
#define MS_INVALIDATE           (1 << 2)

#define MREMAP_MAYMOVE          (1 << 0)
#define MREMAP_FIXED            (1 << 1)

/*! Value returned by @ref sys_mmap on failure */
#define MAP_FAILED              ((void *) -1)

#endif
