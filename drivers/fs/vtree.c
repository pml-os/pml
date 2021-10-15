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

#include <pml/process.h>
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

/*!
 * Resolves a path to a vnode. The returned object should be passed to
 * UNREF_OBJECT() when no longer needed.
 *
 * @todo follow symbolic links
 * @param path the path to resolve
 * @return the vnode corresponding to the path, or NULL on failure
 */

struct vnode *
vnode_namei (const char *path)
{
  struct vnode *vp;
  char *p = strdup (path);
  char *ptr = p;
  char *end;
  if (UNLIKELY (!p))
    return NULL;
  if (*ptr == '/')
    {
      REF_ASSIGN (vp, root_vnode);
      ptr++;
    }
  else
    REF_ASSIGN (vp, THIS_PROCESS->cwd);
  while (1)
    {
      end = strchr (ptr, '/');
      if (end)
	*end = '\0';
      if (*ptr && strcmp (ptr, "."))
	{
	  struct vnode *nvp;
	  if (!strcmp (ptr, ".."))
	    REF_ASSIGN (nvp, vp->parent);
	  else
	    {
	      nvp = vnode_lookup_child (vp, ptr);
	      if (!nvp)
		goto err0;
	    }
	  UNREF_OBJECT (vp);
	  vp = nvp;
	}
      if (!end)
	break;
      ptr = end + 1;
    }

  free (p);
  return vp;

 err0:
  free (p);
  UNREF_OBJECT (vp);
  return NULL;
}
