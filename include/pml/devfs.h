/* devfs.h -- This file is part of PML.
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

#ifndef __PML_DEVFS_H
#define __PML_DEVFS_H

/*!
 * @file
 * @brief @c devfs filesystem support
 */

#include <pml/vfs.h>

__BEGIN_DECLS

extern struct vnode_ops devfs_vnode_ops;

int devfs_lookup (struct vnode **result, struct vnode *dir, const char *name);
int devfs_getattr (struct pml_stat *stat, struct vnode *vp);
ssize_t devfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset);
ssize_t devfs_write (struct vnode *vp, const void *buffer, size_t len,
		     off_t offset);
off_t devfs_readdir (struct vnode *dir, struct pml_dirent *dirent,
		     off_t offset);
ssize_t devfs_readlink (struct vnode *vp, char *buffer, size_t len);

__END_DECLS

#endif
