/* super.c -- This file is part of PML.
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

/*! @file */

#include <pml/ext2fs.h>
#include <errno.h>

const struct mount_ops ext2_mount_ops = {
  .mount = ext2_mount,
  .unmount = ext2_unmount
};

int
ext2_mount (struct mount *mp, unsigned int flags)
{
  mp->root_vnode = vnode_alloc ();
  if (UNLIKELY (!mp->root_vnode))
    return -1;
  mp->ops = &ext2_mount_ops;
  mp->root_vnode->ino = EXT2_ROOT_INO;
  mp->root_vnode->ops = &ext2_vnode_ops;
  REF_ASSIGN (mp->root_vnode->mount, mp);
  ext2_fill (mp->root_vnode);
  return 0;
}

int
ext2_unmount (struct mount *mp, unsigned int flags)
{
  UNREF_OBJECT (mp->root_vnode);
  return 0;
}
