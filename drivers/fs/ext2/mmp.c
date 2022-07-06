/* mmp.c -- This file is part of PML.
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

/* TODO Support MMP */

#include <pml/ext2fs.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int
ext4_mmp_start (struct ext2_fs *fs)
{
  RETV_ERROR (ENOTSUP, -1);
}

int
ext4_mmp_stop (struct ext2_fs *fs)
{
  RETV_ERROR (ENOTSUP, -1);
}
