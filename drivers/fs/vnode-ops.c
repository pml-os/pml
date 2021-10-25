/* vnode-ops.c -- This file is part of PML.
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
 * Finds a vnode that is a child node of a directory through a path component.
 *
 * @param result pointer to store result of lookup, stores NULL on failure
 * @param dir the directory to search in
 * @param name the path component to lookup
 * @return zero on success
 */

int
vfs_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  if (!vfs_can_read (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (dir->ops->lookup)
    return dir->ops->lookup (result, dir, name);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Gets information about a vnode.
 *
 * @param stat the stat structure to store the file's attributes
 * @param vp the vnode to stat
 * @return zero on success
 */

int
vfs_getattr (struct stat *stat, struct vnode *vp)
{
  if (!vfs_can_read (vp, 0))
    return -1;
  stat->st_mode = vp->mode;
  stat->st_nlink = vp->nlink;
  stat->st_ino = vp->ino;
  stat->st_uid = vp->uid;
  stat->st_gid = vp->gid;
  stat->st_dev = vp->mount->device;
  stat->st_rdev = vp->rdev;
  stat->st_atime = vp->atime;
  stat->st_mtime = vp->mtime;
  stat->st_ctime = vp->ctime;
  stat->st_size = vp->size;
  stat->st_blocks = vp->blocks;
  stat->st_blksize = vp->blksize;
  if (vp->ops->getattr)
    return vp->ops->getattr (stat, vp);
  else
    return 0;
}

/*!
 * Reads data from a file.
 *
 * @param vp the vnode to read
 * @param buffer the buffer to store the read data
 * @param len number of bytes to read
 * @param offset offset in file to start reading from
 * @return number of bytes read, or -1 on error
 */

ssize_t
vfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  if (!vfs_can_read (vp, 0))
    return -1;
  if (S_ISDIR (vp->mode))
    RETV_ERROR (EISDIR, -1);
  if (!len)
    return 0;
  if (vp->ops->read)
    return vp->ops->read (vp, buffer, len, offset);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Writes data to a file.
 *
 * @param vp the vnode to write
 * @param buffer the buffer containing the data to write
 * @param len number of bytes to write
 * @param offset offset in file to start writing to
 * @return number of bytes written, or -1 on error
 */

ssize_t
vfs_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  if (!vfs_can_read (vp, 0))
    return -1;
  if (S_ISDIR (vp->mode))
    RETV_ERROR (EISDIR, -1);
  if (!len)
    return 0;
  if (vp->ops->write)
    return vp->ops->write (vp, buffer, len, offset);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Updates the on-disk file by synchronizing file metadata and writing
 * any unwritten buffers to disk.
 *
 * @param vp the vnode to synchronize
 */

void
vfs_sync (struct vnode *vp)
{
  if (vfs_can_write (vp, 0) && vp->ops->sync)
    vp->ops->sync (vp);
}

/*!
 * Creates a new file under a directory and allocates a vnode for it.
 * This function should not be used to create directories, use vfs_mkdir()
 * instead.
 *
 * @param result the pointer to store the vnode of the new file
 * @param dir the directory to create the new file in
 * @param name the name of the new file
 * @param mode file type and permissions
 * @param rdev the device numbers if creating a device, otherwise ignored
 * @return zero on success
 */

int
vfs_create (struct vnode **result, struct vnode *dir, const char *name,
	    mode_t mode, dev_t rdev)
{
  if (!vfs_can_write (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (dir->ops->create)
    return dir->ops->create (result, dir, name, mode, rdev);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Creates a new directory under a directory and allocates a vnode for it.
 * The directory is automatically populated with `.' and `..' entries.
 *
 * @param result the pointer to store the vnode of the new directory
 * @param dir the directory to create the new directory in
 * @param name the name of the new directory
 * @param mode directory permissions, file type bits are ignored
 * @return zero on success
 */

int
vfs_mkdir (struct vnode **result, struct vnode *dir, const char *name,
	   mode_t mode)
{
  if (!vfs_can_write (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (dir->ops->mkdir)
    return dir->ops->mkdir (result, dir, name, mode);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Moves a file to a new directory with a new name.
 *
 * @param vp the vnode to rename
 * @param dir the directory to place the vnode in
 * @param name the name of the new file
 * @return zero on success
 */

int
vfs_rename (struct vnode *vp, struct vnode *dir, const char *name)
{
  if (!vfs_can_write (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (dir->ops->rename)
    return dir->ops->rename (vp, dir, name);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Creates a hard link to a vnode.
 *
 * @param dir the directory to create the link in
 * @param vp the vnode to link
 * @param name the name of the new link
 * @return zero on success
 */

int
vfs_link (struct vnode *dir, struct vnode *vp, const char *name)
{
  if (!vfs_can_write (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (dir->ops->link)
    return dir->ops->link (dir, vp, name);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Creates a symbolic link.
 *
 * @param dir the directory to create the link in
 * @param name the name of the new link
 * @param target target string of symbolic link
 * @return zero on success
 */

int
vfs_symlink (struct vnode *dir, const char *name, const char *target)
{
  if (!vfs_can_write (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (dir->ops->symlink)
    return dir->ops->symlink (dir, name, target);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Reads a directory entry.
 *
 * @param dir the directory to read
 * @param dirent pointer to store directory entry data
 * @param offset offset in directory vnode to read next entry from
 * @return @return <table>
 * <tr><th>Value</th><th>Description</th></tr>
 * <tr><td>-1</td><td>Error occurred</td></tr>
 * <tr><td>0</td><td>No more directory entries to read</td></tr>
 * <tr><td>Positive value</td><td>An offset that can be passed to another
 * call to this function to read the next directory entry</td></tr>
 * </table>
 */

off_t
vfs_readdir (struct vnode *dir, struct dirent *dirent, off_t offset)
{
  off_t ret;
  if (!vfs_can_read (dir, 0))
    return -1;
  if (!S_ISDIR (dir->mode))
    RETV_ERROR (ENOTDIR, -1);
  if (!dir->ops->readdir)
    RETV_ERROR (ENOTSUP, -1);
  ret = dir->ops->readdir (dir, dirent, offset);
  if (ret == -1)
    return -1;
  dirent->d_reclen = offsetof (struct dirent, d_name) + dirent->d_namlen + 1;
  return ret;
}

/*!
 * Reads the contents of a symbolic link.
 *
 * @param vp the vnode of the symbolic link
 * @param buffer where to place contents of link
 * @param len maximum number of bytes to read
 * @return number of bytes read, or -1 on failure. If the number of bytes
 * is equal to @p len, it is not possible to determine whether the data
 * read was truncated, and this function should be called again with a larger
 * buffer.
 */

ssize_t
vfs_readlink (struct vnode *vp, char *buffer, size_t len)
{
  if (!vfs_can_read (vp, 0))
    return -1;
  if (!S_ISLNK (vp->mode))
    RETV_ERROR (EINVAL, -1);
  if (vp->ops->readlink)
    return vp->ops->readlink (vp, buffer, len);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Determines the physical block numbers of one or more consecutive logical
 * blocks for a vnode.
 *
 * @param vp the vnode
 * @param result where to store physical block numbers. This must point to
 * a buffer capable of storing at least @p num @ref block_t values.
 * @param block first logical block number
 * @param num number of logical blocks past the first block to map
 * @return zero on success
 */

int
vfs_bmap (struct vnode *vp, block_t *result, block_t block, size_t num)
{
  if (vp->ops->bmap)
    return vp->ops->bmap (vp, result, block, num);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Fills the fields of the @ref vnode structure by reading information from
 * the on-disk file. A vnode object passed to this function should have
 * its @ref vnode.ino member set to the inode number of the on-disk file.
 *
 * @param vp the vnode
 * @return zero on success
 */

int
vfs_fill (struct vnode *vp)
{
  if (vp->ops->fill)
    return vp->ops->fill (vp);
  else
    RETV_ERROR (ENOTSUP, -1);
}

/*!
 * Deallocates any private data allocated to a vnode. This function is called
 * before deallocating a vnode.
 *
 * @param vp the vnode
 */

void
vfs_dealloc (struct vnode *vp)
{
  if (vp->children)
    strmap_free (vp->children, vnode_free_child);
  UNREF_OBJECT (vp->parent);
  if (vp->ops->dealloc)
    vp->ops->dealloc (vp);
}
