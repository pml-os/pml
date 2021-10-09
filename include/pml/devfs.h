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

/*! Bitmask used in inode to indicate a non-device-file inode */
#define DEVFS_SPECIAL_INO       0xffffffff00000000ULL

/*! Root inode of the devfs filesystem */
#define DEVFS_ROOT_INO          DEVFS_SPECIAL_INO

/*! Mode of directories in devfs */
#define DEVFS_DIR_MODE          (__S_IFDIR	\
				 | __S_IRUSR	\
				 | __S_IXUSR	\
				 | __S_IRGRP	\
				 | __S_IXGRP	\
				 | __S_IROTH	\
				 | __S_IXOTH)

/*! Mode of block device files in devfs */
#define DEVFS_BLOCK_DEVICE_MODE (__S_IFBLK	\
				 | __S_IRUSR	\
				 | __S_IWUSR	\
				 | __S_IRGRP	\
				 | __S_IWGRP	\
				 | __S_IROTH	\
				 | __S_IWOTH)

/*! Mode of character device files in devfs */
#define DEVFS_CHAR_DEVICE_MODE  (__S_IFCHR	\
				 | __S_IRUSR	\
				 | __S_IWUSR	\
				 | __S_IRGRP	\
				 | __S_IWGRP	\
				 | __S_IROTH	\
				 | __S_IWOTH)

__BEGIN_DECLS

extern const struct mount_ops devfs_mount_ops;
extern const struct vnode_ops devfs_vnode_ops;

int devfs_mount (struct mount *mp, unsigned int flags);
int devfs_unmount (struct mount *mp, unsigned int flags);

int devfs_lookup (struct vnode **result, struct vnode *dir, const char *name);
ssize_t devfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset);
ssize_t devfs_write (struct vnode *vp, const void *buffer, size_t len,
		     off_t offset);
off_t devfs_readdir (struct vnode *dir, struct pml_dirent *dirent,
		     off_t offset);
ssize_t devfs_readlink (struct vnode *vp, char *buffer, size_t len);
int devfs_fill (struct vnode *vp);

__END_DECLS

#endif
