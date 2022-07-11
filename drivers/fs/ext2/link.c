/* link.c -- This file is part of PML.
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

#include <pml/ext2fs.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int
ext2_check_empty (struct vnode *dir, int entry, struct ext2_dirent *dirent,
		  int offset, blksize_t blksize, char *buffer, void *private)
{
  if (strcmp (dirent->d_name, ".") && strcmp (dirent->d_name, ".."))
    {
      *((int *) private) = 0;
      return DIRENT_ABORT;
    }
  return 0;
}

static int
ext2_process_link (struct vnode *dir, int entry, struct ext2_dirent *dirent,
		   int offset, blksize_t blksize, char *buffer, void *private)
{
  struct ext2_link_ctx *l = private;
  struct ext2_fs *fs = l->fs;
  struct ext2_dirent *next;
  unsigned int rec_len;
  unsigned int min_rec_len;
  unsigned int curr_rec_len;
  int csum_size = 0;
  int ret = 0;
  if (l->done)
    return DIRENT_ABORT;

  rec_len = ext2_dir_rec_len (l->namelen, 0);
  l->err = ext2_get_rec_len (fs, dirent, &curr_rec_len);
  if (l->err)
    return DIRENT_ABORT;

  if (fs->super.s_feature_ro_compat & EXT4_FT_RO_COMPAT_METADATA_CSUM)
    csum_size = sizeof (struct ext2_dirent_tail);

  next = (struct ext2_dirent *) (buffer + offset + curr_rec_len);
  if (offset + curr_rec_len < blksize - csum_size - 8 && !next->d_inode
      && offset + curr_rec_len + next->d_rec_len <= blksize)
    {
      curr_rec_len += next->d_rec_len;
      l->err = ext2_set_rec_len (fs, curr_rec_len, dirent);
      if (l->err)
	return DIRENT_ABORT;
      ret = DIRENT_CHANGED;
    }

  if (dirent->d_inode)
    {
      min_rec_len = ext2_dir_rec_len (dirent->d_name_len & 0xff, 0);
      if (curr_rec_len < min_rec_len + rec_len)
	return ret;
      rec_len = curr_rec_len - min_rec_len;
      l->err = ext2_set_rec_len (fs, min_rec_len, dirent);
      if (l->err)
	return DIRENT_ABORT;

      next = (struct ext2_dirent *) (buffer + offset + dirent->d_rec_len);
      next->d_inode = 0;
      next->d_name_len = 0;
      l->err = ext2_set_rec_len (fs, rec_len, next);
      if (l->err)
	return DIRENT_ABORT;
      return DIRENT_CHANGED;
    }

  if (curr_rec_len < rec_len)
    return ret;
  dirent->d_inode = l->inode;
  dirent->d_name_len = (dirent->d_name_len & 0xff00) | (l->namelen & 0xff);
  strncpy (dirent->d_name, l->name, l->namelen);
  if (fs->super.s_feature_incompat & EXT2_FT_INCOMPAT_FILETYPE)
    dirent->d_name_len = (dirent->d_name_len & 0xff) | ((l->flags & 7) << 8);
  l->done = 1;
  return DIRENT_ABORT | DIRENT_CHANGED;
}

static int
ext2_process_unlink (struct vnode *dir, int entry, struct ext2_dirent *dirent,
		     int offset, blksize_t blksize, char *buffer, void *private)
{
  struct ext2_link_ctx *l = private;
  struct ext2_fs *fs = l->fs;
  struct ext2_file *file;
  struct ext2_dirent *prev = l->prev;
  struct vnode *vp;
  int ret;
  l->prev = dirent;
  if (l->name)
    {
      if ((dirent->d_name_len & 0xff) != l->namelen)
	return 0;
      if (strncmp (l->name, dirent->d_name, dirent->d_name_len & 0xff))
	return 0;
    }
  if (!dirent->d_inode)
    return 0;

  vp = ext2_lookup_or_read (dir, dirent->d_inode);
  if (!vp)
    return -1;
  file = vp->data;

  /* Remove link from inode link count and unallocate if no remaining links */
  if (S_ISDIR (file->inode.i_mode))
    {
      /* Make sure the directory is empty */
      int empty = 1;
      ext2_dir_iterate (fs, dir, DIRENT_FLAG_EMPTY, NULL, ext2_check_empty,
			&empty);
      if (!empty)
	{
	  l->err = -1;
	  errno = ENOTEMPTY;
	  return DIRENT_ABORT;
	}
    }
  vp->nlink--;
  file->inode.i_links_count--;
  if (!vp->nlink)
    {
      file->inode.i_dtime = time (NULL);
      ext2_inode_alloc_stats (fs, dirent->d_inode, -1,
			      S_ISDIR (file->inode.i_mode));
      ext2_dealloc_blocks (fs, dirent->d_inode, &file->inode, NULL, 0, ~0UL);
    }
  ext2_update_inode (fs, dirent->d_inode, &file->inode,
		     sizeof (struct ext2_inode));

 end:
  if (offset)
    prev->d_rec_len += dirent->d_rec_len;
  else
    dirent->d_inode = 0;
  l->done = 1;
  return DIRENT_ABORT | DIRENT_CHANGED;
}

int
ext2_add_index_link (struct ext2_fs *fs, struct vnode *dir, const char *name,
		     ino_t ino, int flags)
{
  RETV_ERROR (ENOTSUP, -1);
}

int
ext2_add_link (struct ext2_fs *fs, struct vnode *dir, const char *name,
	       ino_t ino, int flags)
{
  struct ext2_file *file = dir->data;
  struct ext2_inode *inode = &file->inode;
  struct ext2_link_ctx l;
  struct vnode *vp;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);

  if (inode->i_flags & EXT2_INDEX_FL)
    return ext2_add_index_link (fs, dir, name, ino, flags);

  l.fs = fs;
  l.name = name;
  l.namelen = name ? strlen (name) : 0;
  l.inode = ino;
  l.flags = flags;
  l.done = 0;
  l.err = 0;
  ret =
    ext2_dir_iterate (fs, dir, DIRENT_FLAG_EMPTY, NULL, ext2_process_link, &l);
  if (ret)
    return ret;
  if (l.err)
    return l.err;
  if (l.done)
    return 0;

  /* Couldn't add entry, expand the directory and try again */
  ret = ext2_expand_dir (dir);
  if (ret)
    RETV_ERROR (ENOSPC, -1);
  ret =
    ext2_dir_iterate (fs, dir, DIRENT_FLAG_EMPTY, NULL, ext2_process_link, &l);
  if (ret)
    return ret;
  if (l.err)
    return l.err;
  if (l.done)
    return 0;
  else
    RETV_ERROR (ENOSPC, -1);
}

int
ext2_unlink_dirent (struct ext2_fs *fs, struct vnode *dir, const char *name,
		    int flags)
{
  struct ext2_link_ctx l;
  int ret;
  if (fs->mflags & MS_RDONLY)
    RETV_ERROR (EROFS, -1);
  l.fs = fs;
  l.name = name;
  l.namelen = name ? strlen (name) : 0;
  l.flags = flags;
  l.done = 0;
  l.err = 0;
  l.prev = NULL;
  ret = ext2_dir_iterate (fs, dir, DIRENT_FLAG_EMPTY, NULL,
			  ext2_process_unlink, &l);
  if (ret)
    return ret;
  if (l.err)
    return l.err;
  if (l.done)
    return 0;
  else
    RETV_ERROR (ENOENT, -1);
}

int
ext2_dir_type (mode_t mode)
{
  if (S_ISREG (mode))
    return EXT2_FILE_REG;
  if (S_ISDIR (mode))
    return EXT2_FILE_DIR;
  if (S_ISCHR (mode))
    return EXT2_FILE_CHR;
  if (S_ISBLK (mode))
    return EXT2_FILE_BLK;
  if (S_ISFIFO (mode))
    return EXT2_FILE_FIFO;
  if (S_ISSOCK (mode))
    return EXT2_FILE_SOCK;
  if (S_ISLNK (mode))
    return EXT2_FILE_LNK;
  return EXT2_FILE_UNKNOWN;
}
