/* vfs.h -- This file is part of PML.
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

#ifndef __PML_VFS_H
#define __PML_VFS_H

/*!
 * @file
 * @brief VFS structures and definitions
 */

#include <pml/object.h>
#include <pml/types.h>

/*!
 * Possible types of @ref vnode objects.
 */

enum vnode_type
{
  VNODE_TYPE_NONE,              /*!< Unknown type */
  VNODE_TYPE_REG,               /*!< Regular file */
  VNODE_TYPE_DIR,               /*!< Directory */
  VNODE_TYPE_CHR,               /*!< Character device */
  VNODE_TYPE_BLK,               /*!< Block device */
  VNODE_TYPE_LNK,               /*!< Symbolic link */
  VNODE_TYPE_SOCK,              /*!< Socket */
  VNODE_TYPE_FIFO               /*!< Named pipe */
};

/*!
 * Represents a vnode, a VFS abstraction of a filesystem inode.
 */

struct vnode
{
  REF_COUNT;
  enum vnode_type type;         /*!< Type of vnode */
  mode_t mode;                  /*!< Permissions */
  uid_t uid;                    /*!< User ID of vnode owner */
  gid_t gid;                    /*!< Group ID of vnode owner */
};

/*!
 * Represents a directory entry in the VFS layer. Directory entries store
 * a @ref vnode object and a name string.
 */

struct dentry
{
  REF_COUNT;
  struct vnode *vnode;          /*!< Pointer to vnode */
  char *name;                   /*!< Name of entry path component */
  size_t child_count;           /*!< Number of child entries */
  struct dentry **children;     /*!< Array of pointers to child entries */
};

__BEGIN_DECLS

extern struct dentry *root_dentry;

void mount_root (void);

__END_DECLS

#endif
