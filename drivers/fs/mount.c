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

#include <pml/devfs.h>
#include <pml/panic.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

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
  struct mount *devfs_mp;

  ALLOC_OBJECT (root_vnode, vfs_dealloc);
  if (UNLIKELY (!root_vnode))
    goto err0;
  root_vnode->name = "/";

  /* Make /.. link to / */
  if (vnode_add_child (root_vnode, root_vnode))
    goto err0;

  /* Mount devfs on /dev */
  if (register_filesystem ("devfs", &devfs_mount_ops))
    panic ("Failed to register devfs");
  devfs_mp = mount_filesystem ("devfs", 0);
  if (UNLIKELY (!devfs_mp))
    goto err0;
  devfs_mp->root_vnode->name = "dev";
  if (vnode_add_child (root_vnode, devfs_mp->root_vnode))
    goto err0;
  return;

 err0:
  panic ("Filesystem initialization failed");
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
 * Creates a new mount structure for a filesystem. The structure is initialized
 * by calling the vfs_mount() function.
 *
 * @param type the filesystem type
 * @param flags mount flags, passed to vfs_mount()
 * @return the mount structure, or NULL on failure
 */

struct mount *
mount_filesystem (const char *type, unsigned int flags)
{
  struct mount *mp;
  size_t i;
  for (i = 0; i < filesystem_count; i++)
    {
      if (!strcmp (filesystem_table[i].name, type))
	{
	  ALLOC_OBJECT (mp, free);
	  if (UNLIKELY (!mp))
	    return NULL;
	  mp->ops = filesystem_table[i].ops;
	  if (vfs_mount (mp, flags))
	    {
	      UNREF_OBJECT (mp);
	      return NULL;
	    }
	  return mp;
	}
    }
  RETV_ERROR (EINVAL, NULL);
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
