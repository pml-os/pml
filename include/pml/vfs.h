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
 * @todo use a system-wide vnode cache so processes cannot open multiple
 * vnodes for a single inode
 */

#include <pml/dirent.h>
#include <pml/fcntl.h>
#include <pml/object.h>
#include <pml/map.h>
#include <pml/stat.h>

/*! Constant with all permission bits set */
#define FULL_PERM               (S_IRWXU | S_IRWXG | S_IRWXO)

/*! Default permission bits for symbolic links */
#define SYMLINK_MODE (S_IFLNK | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* Vnode flags */

#define VN_FLAG_NO_BLOCK        (1 << 0)    /*!< Prevent I/O from blocking */
#define VN_FLAG_SYNC_PROC       (1 << 1)    /*!< Already processed by sync */

/* Mount flags */

#define MS_RDONLY               (1 << 0)    /*!< Filesystem is read-only */

/*! Represents a block number in a filesystem. */
typedef unsigned long block_t;

struct mount_ops;
struct mount;
struct vnode;

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

  /*!
   * Checks whether a block device vnode is a valid filesystem.
   *
   * @param vp the vnode to check
   * @return nonzero if the vnode contains a valid filesystem
   */
  int (*check) (struct vnode *vp);

  /*!
   * Flushes a filesystem by writing filesystem metadata to disk. Individual
   * vnodes in the filesystem are not synchronized.
   *
   * @param mp the mount structure
   */
  void (*flush) (struct mount *mp);
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

  struct filesystem *fstype;    /*!< Filesystem type of mount */
  struct vnode *parent;         /*!< Dir in parent fs containing root vnode */
  char *root_name;              /*!< Name of dir entry of mount point */
  struct hashmap *vcache;       /*!< Vnode cache */
  unsigned int flags;           /*!< Mount options */
  dev_t device;                 /*!< Device number, if applicable */
  const struct mount_ops *ops;  /*!< Mount operation vector */
  void *data;                   /*!< Driver-specific private data */
};

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
  int (*getattr) (struct stat *stat, struct vnode *vp);

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
   * @return zero on success
   */
  int (*sync) (struct vnode *vp);

  /*!
   * Changes the permissions of a file.
   *
   * @param vp the vnode
   * @param mode an integer containing the new permissions of the file in
   * the corresponding bits
   * @return zero on success
   */
  int (*chmod) (struct vnode *vp, mode_t mode);

  /*!
   * Changes the owner and/or group owner of a file.
   *
   * @param vp the vnode
   * @param uid the new user ID, or -1 to leave the user owner unchanged
   * @param gid the new group ID, or -1 to leave the group owner unchanged
   * @return zero on success
   */
  int (*chown) (struct vnode *vp, uid_t uid, gid_t gid);

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
   * @param olddir the directory containing the file to rename
   * @param oldname name of the file to rename
   * @param newdir the directory to place the renamed file
   * @param newname the new name of the file
   * @return zero on success
   */
  int (*rename) (struct vnode *olddir, const char *oldname,
		 struct vnode *newdir, const char *newname);

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
   * Unlinks a file from a directory. If no more links exist to the unlinked
   * file, it should be deallocated.
   *
   * @param dir the directory containing the file to unlink
   * @param name the name of the file to unlink
   * @return zero on success
   */
  int (*unlink) (struct vnode *dir, const char *name);

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
   * Reads a directory entry. Implementations of this function do not need
   * to set the @ref dirent.reclen member.
   *
   * @param dir the directory to read
   * @param dirent pointer to store directory entry data
   * @param offset offset in directory vnode to read next entry from
   * @return <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>-1</td><td>Error occurred</td></tr>
   * <tr><td>0</td><td>No more directory entries to read</td></tr>
   * <tr><td>Positive value</td><td>An offset that can be passed to another
   * call to this function to read the next directory entry</td></tr>
   * </table>
   */
  off_t (*readdir) (struct vnode *dir, struct dirent *dirent, off_t offset);

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
  ssize_t (*readlink) (struct vnode *vp, char *buffer, size_t len);

  /*!
   * Sets the size of a file, filling any added bytes with zero bytes.
   *
   * @param vp the vnode
   * @param len the new length in bytes
   * @return zero on success
   */
  int (*truncate) (struct vnode *vp, off_t len);

  /*!
   * Updates the access and modify timestamps of the file.
   *
   * @param vp the vnode
   * @param access the new access time
   * @param modify the new modify time
   * @return zero on success
   */
  int (*utime) (struct vnode *vp, const struct timespec *access,
		const struct timespec *modify);

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
  struct timespec atime;        /*!< Time of last access */
  struct timespec mtime;        /*!< Time of last data modification */
  struct timespec ctime;        /*!< Time of last metadata modification */
  size_t size;                  /*!< Number of bytes in file */
  blkcnt_t blocks;              /*!< Number of blocks allocated to file */
  blksize_t blksize;            /*!< Optimal I/O block size */
  const struct vnode_ops *ops;  /*!< Vnode operation vector */
  unsigned int flags;           /*!< Vnode flags */
  struct strmap *children;      /*!< Hashmap of child vnodes' inode numbers */
  struct vnode *parent;         /*!< Parent vnode */
  struct mount *mount;          /*!< Filesystem the vnode is on */
  void *data;                   /*!< Driver-specific private data */
};

/*!
 * Determines a suitable value for the @ref dirent.d_reclen field in
 * the directory entry structure.
 *
 * @param name_len the length of the entry name in bytes
 * @return the record length
 */

static inline uint16_t
dirent_rec_len (size_t name_len)
{
  return ALIGN_UP (offsetof (struct dirent, d_name) + name_len + 1, 8);
}

__BEGIN_DECLS

extern struct filesystem *filesystem_table;
extern struct mount **mount_table;
extern size_t filesystem_count;
extern size_t mount_count;
extern struct vnode *root_vnode;
extern struct mount *devfs;

int vfs_can_read (struct vnode *vp, int real);
int vfs_can_write (struct vnode *vp, int real);
int vfs_can_exec (struct vnode *vp, int real);
int vfs_can_seek (struct vnode *vp);

int vfs_mount (struct mount *mp, unsigned int flags);
int vfs_unmount (struct mount *mp, unsigned int flags);
void vfs_flush (struct mount *mp);

int vfs_lookup (struct vnode **result, struct vnode *dir, const char *name);
int vfs_getattr (struct stat *stat, struct vnode *vp);
ssize_t vfs_read (struct vnode *vp, void *buffer, size_t len, off_t offset);
ssize_t vfs_write (struct vnode *vp, const void *buffer, size_t len,
		   off_t offset);
int vfs_sync (struct vnode *vp);
int vfs_chmod (struct vnode *vp, mode_t mode);
int vfs_chown (struct vnode *vp, uid_t uid, gid_t gid);
int vfs_create (struct vnode **result, struct vnode *dir, const char *name,
		mode_t mode, dev_t rdev);
int vfs_mkdir (struct vnode **result, struct vnode *dir, const char *name,
	       mode_t mode);
int vfs_rename (struct vnode *olddir, const char *oldname, struct vnode *newdir,
		const char *newname);
int vfs_link (struct vnode *dir, struct vnode *vp, const char *name);
int vfs_unlink (struct vnode *dir, const char *name);
int vfs_symlink (struct vnode *dir, const char *name, const char *target);
off_t vfs_readdir (struct vnode *dir, struct dirent *dirent, off_t offset);
ssize_t vfs_readlink (struct vnode *vp, char *buffer, size_t len);
int vfs_truncate (struct vnode *vp, off_t len);
int vfs_utime (struct vnode *vp, const struct timespec *access,
	       const struct timespec *modify);
int vfs_fill (struct vnode *vp);
void vfs_dealloc (struct vnode *vp);

void init_vfs (void);
void mount_root (void);
int register_filesystem (const char *name, const struct mount_ops *ops);
struct mount *mount_filesystem (const char *type, dev_t device,
				unsigned int flags, struct vnode *parent,
				const char *name);
const char *guess_filesystem_type (struct vnode *vp);

struct vnode *vnode_alloc (void);
void vnode_unref (void *data);
int vnode_add_child (struct vnode *vp, struct vnode *child, const char *name);
void vnode_place_cache (struct vnode *vp);
struct vnode *vnode_lookup_cache (struct mount *mp, ino_t ino);
void vnode_remove_cache (struct vnode *vp);
struct vnode *vnode_lookup_child (struct vnode *dir, const char *name);
struct vnode *vnode_namei (const char *path, int link_count);
int vnode_dir_name (const char *path, struct vnode **dir, const char **name);
struct vnode *vnode_find_mount_point (struct vnode *vp, const char *name);

__END_DECLS

#endif
