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
 * @return zero on success
 */

int
vnode_add_child (struct vnode *vp, struct vnode *child)
{
  struct vnode **children =
    realloc (vp->children, sizeof (struct vnode *) * ++vp->child_count);
  if (UNLIKELY (!children))
    return -1;
  children[vp->child_count - 1] = child;
  REF_OBJECT (child);
  vp->children = children;
  child->parent = vp;
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
  struct vnode *vp;
  struct vnode **children;
  size_t i;
  for (i = 0; i < dir->child_count; i++)
    {
      if (!strcmp (dir->children[i]->name, name))
	{
	  REF_OBJECT (dir->children[i]);
	  return dir->children[i];
	}
    }
  if (vfs_lookup (&vp, dir, name))
    return NULL;
  REF_OBJECT (vp);
  if (vnode_add_child (dir, vp))
    UNREF_OBJECT (vp);
  return vp;
}
