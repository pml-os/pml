/* vtree.c -- This file is part of PML.
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

#include <pml/vfs.h>
#include <stdlib.h>
#include <string.h>

/*!
 * Adds a vnode as a child of a parent vnode.
 *
 * @param vp the parent vnode
 * @param child the child vnode
 * @param name name of the child vnode
 * @return zero on success
 */

int
vnode_add_child (struct vnode *vp, struct vnode *child, const char *name)
{
  if (strmap_insert (vp->children, name, child))
    return -1;
  REF_ASSIGN (child->parent, vp);
  REF_OBJECT (child);
  return 0;
}

/*!
 * Looks up a child of a vnode by name. The array of child vnodes is searched
 * for a vnode with a matching name, and if not found, the VFS looks up the
 * child name and adds it to the list of children.
 *
 * @param dir the directory to search in
 * @param name the name to search for
 * @return the child vnode. Call UNREF_OBJECT() on the returned pointer when
 * no longer needed.
 */

struct vnode *
vnode_lookup_child (struct vnode *dir, const char *name)
{
  struct vnode *vp = strmap_lookup (dir->children, name);
  if (vp)
    {
      REF_OBJECT (vp);
      return vp;
    }
  if (vfs_lookup (&vp, dir, name))
    return NULL;
  REF_OBJECT (vp);
  if (vnode_add_child (dir, vp, name))
    UNREF_OBJECT (vp);
  return vp;
}
