/* vnode.c -- This file is part of PML.
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

#include <pml/syslimits.h>
#include <pml/vfs.h>
#include <errno.h>

/*!
 * Allocates an empty vnode.
 *
 * @return a pointer to an empty vnode, or NULL if the allocation failed
 */

struct vnode *
vnode_alloc (void)
{
  struct vnode *vp;
  ALLOC_OBJECT (vp, vfs_dealloc);
  if (UNLIKELY (!vp))
    return NULL;
  vp->children = strmap_create ();
  if (UNLIKELY (!vp->children))
    {
      UNREF_OBJECT (vp);
      return 0;
    }
  return vp;
}

/*!
 * Callback wrapper function for removing a reference from a vnode.
 *
 * @param data the vnode
 */

void
vnode_unref (void *data)
{
  struct vnode *vp = data;
  UNREF_OBJECT (vp);
}

/*!
 * Callback function for freeing a hashmap of child vnodes.
 *
 * @param data pointer to a child vnode
 */

void
vnode_free_child (void *data)
{
  struct vnode *vp = data;
  UNREF_OBJECT (vp);
}

/*!
 * Places a vnode object into its mount structure's vnode cache. If the
 * vnode cannot be added to the cache for any reason, the function fails
 * silently.
 *
 * @param vp the vnode
 */

void
vnode_place_cache (struct vnode *vp)
{
  /* We don't add a reference to the vnode because otherwise vnodes would
     never be freed until the filesystem was unmounted; the entry in the
     vnode cache is removed in the vnode deallocate function. */
  hashmap_insert (vp->mount->vcache, vp->ino, vp);
}

/*!
 * Looks up a vnode structure in a mounted filesystem's vnode cache.
 *
 * @param mp the mount structure of the filesystem'
 * @param ino the inode number
 * @return the vnode structure, or NULL if the lookup failed. The returned
 * pointer should not be freed.
 */

struct vnode *
vnode_lookup_cache (struct mount *mp, ino_t ino)
{
  return hashmap_lookup (mp->vcache, ino);
}

/*!
 * Removes a vnode from its filesystem's vnode cache.
 *
 * @param vp the vnode
 */

void
vnode_remove_cache (struct vnode *vp)
{
  hashmap_remove (vp->mount->vcache, vp->ino);
}
