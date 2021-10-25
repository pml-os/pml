/* dir.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/ext2fs.h>
#include <errno.h>
#include <string.h>

/*!
 * Structure used as private data for directory iterations on lookup.
 */

struct ext2_lookup_iter
{
  const char *name;             /*!< Name to look for */
  size_t name_len;              /*!< Length of name */
  ino_t ino;                    /*!< Inode number of entry */
};

static enum ext2_iter_status
ext2_lookup_iter (struct ext2_dirent *dirent, void *data)
{
  struct ext2_lookup_iter *iter = data;
  if (ext2_dirent_name_len (dirent) == iter->name_len
      && !strncmp (dirent->d_name, iter->name, iter->name_len))
    {
      iter->ino = dirent->d_inode;
      return EXT2_ITER_END;
    }
  return EXT2_ITER_OK;
}

static enum ext2_iter_status
ext2_readdir_iter (struct ext2_dirent *dirent, void *data)
{
  *((struct ext2_dirent **) data) = dirent;
  return EXT2_ITER_END;
}

/*!
 * Iterates through the entries of a directory by calling a callback function
 * and passing a pointer to each entry.
 *
 * @param vp the vnode of the directory
 * @param offset offset in file to start iterating from
 * @param func callback function
 * @param data data to pass to callback function
 * @return zero on success
 */

int
ext2_iterate_dir (struct vnode *vp, off_t offset, ext2_dir_iter_t func,
		  void *data)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_dirent *dirent;
  block_t b;
  for (b = offset / fs->block_size, offset %= fs->block_size;
       b < (block_t) vp->blocks; b++, offset = 0)
    {
      if (ext2_read_io_buffer_block (vp, b))
	return -1;
      for (dirent = (struct ext2_dirent *) (file->io_buffer + offset);
	   offset < fs->block_size; offset += dirent->d_rec_len,
	     dirent = (struct ext2_dirent *) (file->io_buffer + offset))
	{
	  if (dirent->d_inode)
	    {
	      switch (func (dirent, data))
		{
		case EXT2_ITER_END:
		  return 0;
		case EXT2_ITER_ERROR:
		  return -1;
		default:
		  break;
		}
	    }
	}
    }
  return 0;
}

int
ext2_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  struct ext2_lookup_iter iter;
  struct vnode *vp;
  iter.name = name;
  iter.name_len = strlen (name);
  iter.ino = 0;
  if (UNLIKELY (iter.name_len > EXT2_MAX_NAME))
    RETV_ERROR (ENOENT, -1);
  if (ext2_iterate_dir (dir, 0, ext2_lookup_iter, &iter))
    return -1;
  if (!iter.ino)
    RETV_ERROR (ENOENT, -1);
  vp = vnode_alloc ();
  if (UNLIKELY (!vp))
    return -1;
  vp->ino = iter.ino;
  vp->ops = &ext2_vnode_ops;
  REF_ASSIGN (vp->mount, dir->mount);
  REF_ASSIGN (vp->parent, dir);
  if (ext2_fill (vp))
    {
      UNREF_OBJECT (vp);
      return -1;
    }
  *result = vp;
  return 0;
}

off_t
ext2_readdir (struct vnode *dir, struct dirent *dirent, off_t offset)
{
  struct ext2_fs *fs = dir->mount->data;
  struct ext2_dirent *entry = NULL;
  size_t name_len;
  if (ext2_iterate_dir (dir, offset, ext2_readdir_iter, &entry))
    return -1;
  if (!entry)
    return 0;
  dirent->d_ino = entry->d_inode;
  dirent->d_namlen = ext2_dirent_name_len (entry);
  name_len = ALIGN_UP (dirent->d_namlen + 1, 8);
  dirent->d_reclen = offsetof (struct dirent, d_name) + name_len;
  dirent->d_type = ext2_dirent_file_type (entry);
  strncpy (dirent->d_name, entry->d_name, dirent->d_namlen);
  dirent->d_name[dirent->d_namlen] = '\0';
  return offset + entry->d_rec_len;
}
