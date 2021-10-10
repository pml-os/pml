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

#define __S_IFMT                0170000
#define __S_IFIFO               0010000
#define __S_IFCHR               0020000
#define __S_IFDIR               0040000
#define __S_IFBLK               0060000
#define __S_IFREG               0100000
#define __S_IFLNK               0120000
#define __S_IFSOCK              0140000

#define __S_IRUSR               00400
#define __S_IWUSR               00200
#define __S_IXUSR               00100
#define __S_IRWXU               (__S_IRUSR | __S_IWUSR | __S_IXUSR)

#define __S_IRGRP               00040
#define __S_IWGRP               00020
#define __S_IXGRP               00010
#define __S_IRWXG               (__S_IRGRP | __S_IWGRP | __S_IXGRP)

#define __S_IROTH               00004
#define __S_IWOTH               00002
#define __S_IXOTH               00001
#define __S_IRWXO               (__S_IROTH | __S_IWOTH | __S_IXOTH)

#define __S_ISUID               04000
#define __S_ISGID               02000
#define __S_ISVTX               01000

#define __S_ISBLK(x)            (((x) & __S_IFMT) == __S_IFBLK)
#define __S_ISCHR(x)            (((x) & __S_IFMT) == __S_IFCHR)
#define __S_ISDIR(x)            (((x) & __S_IFMT) == __S_IFDIR)
#define __S_ISFIFO(x)           (((x) & __S_IFMT) == __S_IFIFO)
#define __S_ISREG(x)            (((x) & __S_IFMT) == __S_IFREG)
#define __S_ISLNK(x)            (((x) & __S_IFMT) == __S_IFLNK)
#define __S_ISSOCK(x)           (((x) & __S_IFMT) == __S_IFSOCK)

/* Vnode flags */

#define VN_FLAG_NO_BLOCK        (1 << 0)    /*!< Prevent I/O from blocking */

/*! Represents a block number in a filesystem. */
typedef unsigned long block_t;

/*!
 * Represents an entry in the filesystem table. Maps filesystem names to
 * operation vectors so mounting a filesystem by its name can select the
 * correct operation vectors.
 */

struct filesystem
{
  const char *name;             /*!< Filesystem name */
  const struct mount_ops *ops;  /*!< Mount operation vectors */
};

struct mount;

/*!
 * Vector of functions for performing operations on mounted filesystems.
 */

struct mount_ops
{
  /*!
   * Performs any initialization required by a filesystem backend. This function
   * is called when a filesystem is mounted.
   *
   * @param mp the mount structure
   * @param flags mount options
   * @param zero on success
   */
  int (*mount) (struct mount *mp, unsigned int flags);

  /*!
   * Performs any deallocation needed by a filesystem backend when unmounting
   * a filesystem. The root vnode of a mount should be freed here.
   *
   * @param mp the mount structure
   * @param flags mount options
   * @return zero on success
   */
  int (*unmount) (struct mount *mp, unsigned int flags);
};

/*!
 * Represents a mounted filesystem.
 */

struct mount
{
  REF_COUNT (void);

  /*!
   * Pointer to the root vnode of a filesystem. This field must be set
   * to a valid vnode pointer by a filesystem's implementation of
   * the @ref mount_ops.mount function.
   */
  struct vnode *root_vnode;

  unsigned int flags;           /*!< Mount options */
  dev_t device;                 /*!< Device number, if applicable */
  const struct mount_ops *ops;  /*!< Mount operation vector */
  void *data;                   /*!< Driver-specific private data */
};

struct vnode;

/*!
 * Vector of functions for performing operations on vnodes. Performing sanity
 * checks, like checking file permissions or existing files, is handled by
 * the VFS layer and is not necessary in implementations of these functions
 * for filesystem drivers.
 */

struct vnode_ops
{
  /*!
   * Finds a vnode that is a child node of a directory through a path component.
   *
   * @param result pointer to store result of lookup, stores NULL on failure
   * @param dir the directory to search in
   * @param name the path component to lookup
   * @return zero on success
   */
  int (*lookup) (struct vnode **result, struct vnode *dir, const char *name);

  /*!
   * Gets information about a vnode.
   *
   * @param stat the stat structure to store the file's attributes
   * @param vp the vnode to stat
   * @return zero on success
   */
  int (*getattr) (struct pml_stat *stat, struct vnode *vp);

  /*!
   * Reads data from a file.
   *
   * @param vp the vnode to read
   * @param buffer the buffer to store the read data
   * @param len number of bytes to read
   * @param offset offset in file to start reading from
   * @return number of bytes read, or -1 on error
   */
  ssize_t (*read) (struct vnode *vp, void *buffer, size_t len, off_t offset);

  /*!
   * Writes data to a file.
   *
   * @param vp the vnode to write
   * @param buffer the buffer containing the data to write
   * @param len number of bytes to write
   * @param offset offset in file to start writing to
   * @return number of bytes written, or -1 on error
   */
  ssize_t (*write) (struct vnode *vp, const void *buffer, size_t len,
		    off_t offset);

  /*!
   * Updates the on-disk file by synchronizing file metadata and writing
   * any unwritten buffers to disk.
   *
   * @param vp the vnode to synchronize
   */
  void (*sync) (struct vnode *vp);

  /*!
   * Creates a new file under a directory and allocates a vnode for it.
   * This function should not be used to create directories, use @ref mkdir
   * instead.
   *
   * @param result the pointer to store the vnode of the new file
   * @param dir the directory to create the new file in
   * @param name the name of the new file
   * @param mode file type and permissions
   * @param rdev the device numbers if creating a device, otherwise ignored
   * @return zero on success
   */
  int (*create) (struct vnode **result, struct vnode *dir, const char *name,
		 mode_t mode, dev_t rdev);

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
  int (*mkdir) (struct vnode **result, struct vnode *dir, const char *name,
		mode_t mode);

  /*!
   * Moves a file to a new directory with a new name.
   *
   * @param vp the vnode to rename
   * @param dir the directory to place the vnode in
   * @param name the name of the new file
   * @return zero on success
   */
  int (*rename) (struct vnode *vp, struct vnode *dir, const char *name);

  /*!
   * Creates a hard link to a vnode.
   *
   * @param dir the directory to create the link in
   * @param vp the vnode to link
   * @param name the name of the new link
   * @return zero on success
   */
  int (*link) (struct vnode *dir, struct vnode *vp, const char *name);

  /*!
   * Creates a symbolic link.
   *
   * @param dir the directory to create the link in
   * @param name the name of the new link
   * @param target target string of symbolic link
   * @return zero on success
   */
  int (*symlink) (struct vnode *dir, const char *name, const char *target);

  /*!
   * Reads a directory entry.
   *
   * @param dir the directory to read
   * @param dirent pointer to store directory entry data
   * @param offset offset in directory vnode to read next entry from
   * @return offset of next unread directory entry which can then be passed
   * to another call to this function to read the next entry, or -1 on failure
   */
  off_t (*readdir) (struct vnode *dir, struct pml_dirent *dirent, off_t offset);

  /*!
   * Reads the contents of a symbolic link.
   *
   * @param vp the vnode of the symbolic link
   * @param buffer where to place contents of link
   * @param len maximum number of bytes to read
   * @return number of bytes read, or -1 on failure. If the number of bytes
   * is equal to @ref len, it is not possible to determine whether the data
   * read was truncated, and this function should be called again with a larger
   * buffer.
   */
  ssize_t (*readlink) (struct vnode *vp, char *buffer, size_t len);

  /*!
   * Determines the physical block numbers of one or more consecutive logical
   * blocks for a vnode.
   *
   * @param vp the vnode
   * @param result where to store physical block numbers. This must point to
   * a buffer capable of storing at least @ref num @ref block_t values.
   * @param block first logical block number
   * @param num number of logical blocks past the first block to map
   * @return zero on success
   */
  int (*bmap) (struct vnode *vp, block_t *result, block_t block, size_t num);

  /*!
   * Fills the fields of the @ref vnode structure by reading information from
   * the on-disk file. A vnode object passed to this function should have
   * its @ref vnode.ino member set to the inode number of the on-disk file.
   *
   * @param vp the vnode
   * @return zero on success
   */
  int (*fill) (struct vnode *vp);

  /*!
   * Deallocates any private data allocated to a vnode. This function is called
   * before deallocating a vnode.
   *
   * @param vp the vnode
   */
  void (*dealloc) (struct vnode *vp);
};

/*!
 * Represents a vnode, a VFS abstraction of a filesystem inode.
 */

struct vnode
{
  REF_COUNT (struct vnode);
  mode_t mode;                  /*!< Type and permissions */
  ino_t ino;                    /*!< Inode number */
  nlink_t nlink;                /*!< Number of hard links */
  uid_t uid;                    /*!< User ID of vnode owner */
  gid_t gid;                    /*!< Group ID of vnode owner */
  dev_t rdev;                   /*!< Device number (for special device files) */
  struct pml_timespec atime;    /*!< Time of last access */
  struct pml_timespec mtime;    /*!< Time of last data modification */
  struct pml_timespec ctime;    /*!< Time of last metadata modification */
  size_t size;                  /*!< Number of bytes in file */
  blkcnt_t blocks;              /*!< Number of blocks allocated to file */
  blksize_t blksize;            /*!< Optimal I/O block size */
  const struct vnode_ops *ops;  /*!< Vnode operation vector */
  char *name;                   /*!< File name */
  unsigned int flags;           /*!< Vnode flags */
  size_t child_count;           /*!< Number of children */
  struct vnode **children;      /*!< Array of child vnodes */
  struct vnode *parent;         /*!< Parent vnode */
  struct mount *mount;          /*!< Filesystem the vnode is on */
  void *data;                   /*!< Driver-specific private data */
};

__BEGIN_DECLS

extern struct filesystem *filesystem_table;
extern size_t filesystem_count;
extern struct vnode *root_vnode;

int vfs_can_read (struct vnode *vp, int real);
int vfs_can_write (struct vnode *vp, int real);
int vfs_can_exec (struct vnode *vp, int real);

int vfs_mount (struct mount *mp, unsigned int flags);
int vfs_unmount (struct mount *mp, unsigned int flags);

int vfs_lookup (struct vnode **result, struct vnode *dir, const char *name);
int vfs_getattr (struct pml_stat *stat, struct vnode *vp);
ssize_t vfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset);
ssize_t vfs_write (struct vnode *vp, const void *buffer, size_t len,
		   off_t offset);
void vfs_sync (struct vnode *vp);
int vfs_create (struct vnode **result, struct vnode *dir, const char *name,
		mode_t mode, dev_t rdev);
int vfs_mkdir (struct vnode **result, struct vnode *dir, const char *name,
	       mode_t mode);
int vfs_rename (struct vnode *vp, struct vnode *dir, const char *name);
int vfs_link (struct vnode *dir, struct vnode *vp, const char *name);
int vfs_symlink (struct vnode *dir, const char *name, const char *target);
off_t vfs_readdir (struct vnode *dir, struct pml_dirent *dirent, off_t offset);
ssize_t vfs_readlink (struct vnode *vp, char *buffer, size_t len);
int vfs_bmap (struct vnode *vp, block_t *result, block_t block, size_t num);
int vfs_fill (struct vnode *vp);
void vfs_dealloc (struct vnode *vp);

void mount_root (void);
int register_filesystem (const char *name, const struct mount_ops *ops);
struct mount *mount_filesystem (const char *type, unsigned int flags);
int vnode_add_child (struct vnode *vp, struct vnode *child);
struct vnode *vnode_lookup_child (struct vnode *dir, const char *name);

__END_DECLS

#endif
