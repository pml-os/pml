/* dirent.h -- This file is part of PML.
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

#ifndef __PML_DIRENT_H
#define __PML_DIRENT_H

/*!
 * @file
 * @brief Directory entry structures
 */

#include <pml/types.h>

#define DT_UNKNOWN              0
#define DT_FIFO                 1
#define DT_CHR                  2
#define DT_DIR                  4
#define DT_BLK                  6
#define DT_REG                  8
#define DT_LNK                  10
#define DT_SOCK                 12

struct dirent
{
  ino_t d_ino;
  uint16_t d_reclen;
  uint16_t d_namlen;
  unsigned char d_type;
  char d_name[256];
};

#endif
