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

/*!
 * The system filesystem table. Filesystem drivers add entries to this table
 * to register a filesystem backend to the VFS layer.
 */

struct filesystem *filesystem_table;

/*! Number of entries in the filesystem table. */
size_t filesystem_count;

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

/*!
 * Adds a filesystem to the filesystem table.
 *
 * @param name the name of the filesystem. This string should be a unique name
 * that identifies the filesystem. When an object is mounted, the filesystem
 * type selected will be compared to the requested name.
 * @param ops the mount operation vectors
 * @return zero on success
 */

int
register_filesystem (const char *name, const struct mount_ops *ops)
{
  struct filesystem *table =
    realloc (filesystem_table, sizeof (struct filesystem) * ++filesystem_count);
  if (UNLIKELY (!table))
    return -1;
  table[filesystem_count - 1].name = name;
  table[filesystem_count - 1].ops = ops;
  filesystem_table = table;
  return 0;
}

/*!
 * Performs any initialization required by a filesystem backend. This function
 * is called when a filesystem is mounted.
 *
 * @param mp the mount structure
 * @param flags mount options
 * @param zero on success
 */

int
vfs_mount (struct mount *mp, unsigned int flags)
{
  if (mp->ops->mount)
    return mp->ops->mount (mp, flags);
  else
    return 0;
}

/*!
 * Performs any deallocation needed by a filesystem backend when unmounting
 * a filesystem. The root vnode of a mount should be freed here.
 *
 * @param mp the mount structure
 * @param flags mount options
 * @return zero on success
 */

int
vfs_unmount (struct mount *mp, unsigned int flags)
{
  if (mp->ops->unmount)
    return mp->ops->unmount (mp, flags);
  else
    return 0;
}
