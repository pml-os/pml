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

#include <pml/syslimits.h>
#include <errno.h>
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
  if (strmap_insert (vp->children, name, (void *) child->ino))
    return -1;
  REF_OBJECT (child);
  REF_ASSIGN (child->parent, vp);
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
  ino_t ino = (ino_t) strmap_lookup (dir->children, name);
  if (ino)
    {
      vp = vnode_lookup_cache (dir->mount, ino);
      if (vp)
	REF_OBJECT (vp);
      return vp;
    }
  if (vfs_lookup (&vp, dir, name))
    return NULL;
  vnode_place_cache (vp);
  vnode_add_child (dir, vp, name);
  return vp;
}

/*!
 * Resolves a path to a vnode. The returned object should be passed to
 * UNREF_OBJECT() when no longer needed.
 *
 * @todo follow symbolic links
 * @param path the path to resolve
 * @param link_count number of symbolic links encountered, or -1 if symbolic
 * links should not be followed
 * @return the vnode corresponding to the path, or NULL on failure
 */

struct vnode *
vnode_namei (const char *path, int link_count)
{
  struct vnode *vp;
  char *p = strdup (path);
  char *ptr = p;
  char *end;
  if (UNLIKELY (!p))
    return NULL;
  if (link_count >= SYMLOOP_MAX)
    RETV_ERROR (ELOOP, NULL);
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
	      /* Check if the requested name is a mount point */
	      nvp = vnode_find_mount_point (vp, ptr);
	      if (!nvp)
		{
		  /* Do a normal lookup */
		  nvp = vnode_lookup_child (vp, ptr);
		  if (!nvp)
		    goto err0;
		}
	      if (link_count >= 0 && S_ISLNK (nvp->mode))
		{
		  /* Change working directory for relative symlinks */
		  struct vnode *cwd = THIS_PROCESS->cwd;
		  char *buffer;
		  ssize_t ret;
		  if (link_count >= LINK_MAX)
		    GOTO_ERROR (ELOOP, err0);
		  buffer = malloc (PATH_MAX);
		  if (UNLIKELY (!buffer))
		    goto err0;
		  THIS_PROCESS->cwd = vp;
		  ret = vfs_readlink (nvp, buffer, PATH_MAX - 1);
		  if (ret == -1)
		    {
		      free (buffer);
		      THIS_PROCESS->cwd = cwd;
		      goto err0;
		    }
		  buffer[ret] = '\0';
		  nvp = vnode_namei (buffer, link_count + 1);
		  free (buffer);
		  THIS_PROCESS->cwd = cwd;
		  if (!nvp)
		    goto err0;
		}
	    }
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

/*!
 * Separates a path into a parent directory and name, based on the final
 * component of the path.
 *
 * @param path the path to separate
 * @param dir pointer to store vnode of parent directory, which should be
 * passed to UNREF_OBJECT() when no longer needed
 * @param name pointer to store name of last path component
 * @return zero on success
 */

int
vnode_dir_name (const char *path, struct vnode **dir, const char **name)
{
  char *ptr = strdup (path);
  char *end;
  struct vnode *vp;
  if (UNLIKELY (!ptr))
    return -1;

  end = strrchr (ptr, '/');
  if (!end)
    {
      /* No directory path components, the path is just the name itself */
      REF_ASSIGN (*dir, THIS_PROCESS->cwd);
      *name = path;
      goto end;
    }
  *end = '\0';
  if (!*ptr)
    {
      /* Only slash is at the start of the path, the file is directly in
	 the root directory */
      REF_ASSIGN (vp, root_vnode);
      *name = path + 1;
    }
  else
    {
      vp = vnode_namei (ptr, 0);
      if (!vp)
	goto err0;
      *name = path + 1 + (end - ptr);
    }
  *dir = vp;
 end:
  free (ptr);
  return 0;

 err0:
  free (ptr);
  return -1;
}
