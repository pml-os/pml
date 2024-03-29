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
#include <pml/ext2fs.h>
#include <pml/panic.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*!
 * Mount structure of dummy filesystem present before the root filesystem
 * is mounted
 */

static struct mount default_mount;

/*!
 * The system filesystem table. Filesystem drivers add entries to this table
 * to register a filesystem backend to the VFS layer.
 */

struct filesystem *filesystem_table;

/*!
 * The system mount table. Stores all mounted filesystem instances.
 */

struct mount **mount_table;

/*! Number of entries in the filesystem table. */
size_t filesystem_count;

/*! Number of entries in the mount table. */
size_t mount_count;

/*! The vnode representing the root of the VFS filesystem. */
struct vnode *root_vnode;

static void
free_mp (void *data)
{
  struct mount *mp = data;
  hashmap_free (mp->vcache, vnode_unref);
  UNREF_OBJECT (mp->root_vnode);
  UNREF_OBJECT (mp->parent);
  free (mp->root_name);
  free (mp);
}

/*!
 * Initializes the VFS and allocates and initializes the root vnode. The
 * device filesystem (devfs) and other standard filesystems are registered.
 */

void
init_vfs (void)
{
  root_vnode = vnode_alloc ();
  if (UNLIKELY (!root_vnode))
    goto err0;

  /* Set working directory to root directory */
  THIS_PROCESS->cwd = root_vnode;
  default_mount.vcache = hashmap_create ();
  if (UNLIKELY (!default_mount.vcache))
    panic ("Failed to allocate vnode cache");
  root_vnode->mount = &default_mount;

  /* Mount devfs on /dev */
  if (register_filesystem ("devfs", &devfs_mount_ops))
    panic ("Failed to register devfs");
  devfs = mount_filesystem ("devfs", 0, 0, root_vnode, "dev");
  if (UNLIKELY (!devfs))
    goto err0;

  /* Register standard filesystems */
  register_filesystem ("ext2", &ext2_mount_ops);
  return;

 err0:
  panic ("Filesystem initialization failed");
}

/*!
 * Mounts the root filesystem.
 */

void
mount_root (void)
{
  struct vnode *vp;
  struct mount *mp;
  const char *fs_type;
  init_vfs ();

  if (!boot_options.root_device)
    panic ("No root device file specified");
  vp = vnode_namei (boot_options.root_device, 0);
  if (!vp)
    panic ("Failed to open root device: %s\n", boot_options.root_device);

  /* Mount root filesystem */
  fs_type = guess_filesystem_type (vp);
  if (UNLIKELY (!fs_type))
    goto err0;
  mp = mount_filesystem (fs_type, vp->ino, 0, NULL, NULL);
  if (UNLIKELY (!mp))
    goto err0;
  if (vnode_add_child (mp->root_vnode, devfs->root_vnode, "dev"))
    goto err0;

  /* Fix the root vnode pointer and update devfs's parent vnode */
  UNREF_OBJECT (root_vnode); /* Remove reference from mount parent */
  UNREF_OBJECT (root_vnode); /* Remove reference from initial allocation */
  REF_ASSIGN (root_vnode, mp->root_vnode);
  REF_ASSIGN (devfs->parent, root_vnode);
  REF_ASSIGN (THIS_PROCESS->cwd, root_vnode);

  /* Make /.. link to / */
  REF_ASSIGN (root_vnode->parent, root_vnode);
  return;

 err0:
  panic ("Failed to mount root filesystem");
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
 * @param dev the device number of a corresponding device, if any
 * @param flags mount flags, passed to vfs_mount()
 * @param parent the vnode of the directory containing the mount point in
 * the parent filesystem
 * @param name the directory entry under the parent directory containing
 * the mount point
 * @return the mount structure, or NULL on failure
 */

struct mount *
mount_filesystem (const char *type, dev_t device, unsigned int flags,
		  struct vnode *parent, const char *name)
{
  struct mount **table;
  struct mount *mp;
  size_t i;
  for (i = 0; i < filesystem_count; i++)
    {
      if (!strcmp (filesystem_table[i].name, type))
	{
	  ALLOC_OBJECT (mp, free_mp);
	  if (UNLIKELY (!mp))
	    return NULL;
	  mp->ops = filesystem_table[i].ops;
	  mp->device = device;
	  mp->flags = flags;
	  mp->vcache = hashmap_create ();
	  mp->root_name = NULL;
	  if (UNLIKELY (!mp->vcache))
	    {
	      UNREF_OBJECT (mp);
	      return NULL;
	    }
	  if (vfs_mount (mp, flags))
	    {
	      UNREF_OBJECT (mp);
	      return NULL;
	    }
	  if (hashmap_insert (mp->vcache, mp->root_vnode->ino, mp->root_vnode))
	    {
	      UNREF_OBJECT (mp);
	      return NULL;
	    }

	  if (name)
	    {
	      mp->root_name = strdup (name);
	      if (UNLIKELY (!mp->root_name))
		{
		  UNREF_OBJECT (mp);
		  return NULL;
		}
	    }

	  table =
	    realloc (mount_table, sizeof (struct mount *) * ++mount_count);
	  if (UNLIKELY (!table))
	    {
	      UNREF_OBJECT (mp);
	      mount_count--;
	      return NULL;
	    }
	  table[mount_count - 1] = mp;
	  mount_table = table;

	  REF_OBJECT (mp->root_vnode);
	  mp->fstype = filesystem_table + i;
	  if (parent)
	    REF_ASSIGN (mp->parent, parent);
	  else
	    REF_ASSIGN (mp->parent, mp->root_vnode);
	  return mp;
	}
    }
  RETV_ERROR (EINVAL, NULL);
}

/*!
 * Performs any initialization required by as filesystem backend. This function
 * is called when a filesystem is mounted.
 *
 * @param mp the mount structure
 * @param flags mount options
 * @return zero on success
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

/*!
 * Flushes a filesystem by writing filesystem metadata to disk. Individual
 * vnodes in the filesystem are not synchronized.
 *
 * @param mp the mount structure
 */

void
vfs_flush (struct mount *mp)
{
  if (mp->ops->flush)
    mp->ops->flush (mp);
}

/*!
 * Guesses the type of a filesystem.
 */

const char *
guess_filesystem_type (struct vnode *vp)
{
  size_t i;
  for (i = 0; i < filesystem_count; i++)
    {
      if (filesystem_table[i].ops->check (vp))
	return filesystem_table[i].name;
    }
  return NULL;
}

struct vnode *
vnode_find_mount_point (struct vnode *vp, const char *name)
{
  size_t i;
  for (i = 0; i < mount_count; i++)
    {
      if (mount_table[i]->parent == vp && mount_table[i]->root_name
	  && !strcmp (mount_table[i]->root_name, name))
	{
	  REF_OBJECT (mount_table[i]->root_vnode);
	  return mount_table[i]->root_vnode;
	}
    }
  return NULL;
}
