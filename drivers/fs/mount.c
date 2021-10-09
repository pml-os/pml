/* mount.c -- This file is part of PML.
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

#include <pml/panic.h>
#include <pml/vfs.h>
#include <stdio.h>

/*! The vnode representing the root of the VFS filesystem. */
struct vnode *root_vnode;

/*!
 * Allocates and initializes the root vnode.
 */

void
mount_root (void)
{
  ALLOC_OBJECT (root_vnode, vfs_dealloc);
  if (UNLIKELY (!root_vnode))
    panic ("Failed to allocate root vnode");
  root_vnode->name = "/";
}

int
vfs_mount (struct mount *mp, unsigned int flags)
{
  if (mp->ops->mount)
    return mp->ops->mount (mp, flags);
  else
    return 0;
}

int
vfs_unmount (struct mount *mp, unsigned int flags)
{
  if (mp->ops->unmount)
    return mp->ops->unmount (mp, flags);
  else
    return 0;
}
