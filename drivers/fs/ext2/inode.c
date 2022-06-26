/* inode.c -- This file is part of PML.
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

const struct vnode_ops ext2_vnode_ops = {
  .lookup = ext2_lookup,
  .read = ext2_read,
  .write = ext2_write,
  .sync = ext2_sync,
  .create = ext2_create,
  .mkdir = ext2_mkdir,
  .rename = ext2_rename,
  .link = ext2_link,
  .unlink = ext2_unlink,
  .symlink = ext2_symlink,
  .readdir = ext2_readdir,
  .readlink = ext2_readlink,
  .truncate = ext2_truncate,
  .fill = ext2_fill,
  .dealloc = ext2_dealloc
};

/*!
 * Copies information from the on-disk inode structure to the vnode structure.
 *
 * @param vp the vnode
 */

static void
ext2_fill_vnode_data (struct vnode *vp)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = vp->data;
  vp->mode = file->inode.i_mode;
  vp->nlink = file->inode.i_links_count;
  vp->uid = file->inode.i_uid;
  vp->gid = file->inode.i_gid;
  vp->rdev = 0;
  vp->atime.tv_sec = file->inode.i_atime;
  vp->mtime.tv_sec = file->inode.i_mtime;
  vp->ctime.tv_sec = file->inode.i_ctime;
  vp->atime.tv_nsec = vp->mtime.tv_nsec = vp->ctime.tv_nsec = 0;
  vp->blocks = (file->inode.i_blocks * 512 + fs->blksize - 1) / fs->blksize;
  vp->blksize = fs->blksize;
  vp->size = file->inode.i_size;
  if (fs->super.s_feature_ro_compat & EXT2_FT_RO_COMPAT_LARGE_FILE)
    vp->size |= (size_t) file->inode.i_size_high << 32;
}

int
ext2_lookup (struct vnode **result, struct vnode *dir, const char *name)
{
  struct ext2_fs *fs = dir->mount->data;
  struct vnode *vp;
  ino_t ino;
  int ret = ext2_lookup_inode (fs, dir, name, strlen (name), NULL, &ino);
  if (ret)
    return ret;

  vp = vnode_alloc ();
  if (UNLIKELY (!vp))
    RETV_ERROR (ENOMEM, -1);
  vp->ops = &ext2_vnode_ops;
  vp->ino = ino;
  REF_ASSIGN (vp->mount, dir->mount);
  if (ext2_fill (vp))
    {
      UNREF_OBJECT (vp);
      return ret;
    }
  *result = vp;
  return 0;
}

ssize_t
ext2_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  unsigned int count = 0;
  unsigned int start;
  unsigned int c;
  uint64_t left;
  char *ptr = buffer;
  int ret = 0;
  file->pos = offset;

  if (file->inode.i_flags & EXT4_INLINE_DATA_FL)
    RETV_ERROR (ENOTSUP, -1);

  while (file->pos < EXT2_I_SIZE (file->inode) && len > 0)
    {
      ret = ext2_sync_file_buffer_pos (file);
      if (ret)
	return ret;
      ret = ext2_load_file_buffer (file, 0);
      if (ret)
	return ret;
      start = file->pos % fs->blksize;
      c = fs->blksize - start;
      if (c > len)
	c = len;
      left = EXT2_I_SIZE (file->inode) - file->pos;
      if (c > left)
	c = left;
      memcpy (ptr, file->buffer + start, c);
      file->pos += c;
      ptr += c;
      count += c;
      len -= c;
    }
  return count;
}

ssize_t
ext2_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  struct ext2_file *file = vp->data;
  struct ext2_fs *fs = vp->mount->data;
  const char *ptr = buffer;
  unsigned int count = 0;
  unsigned int start;
  unsigned int c;
  int ret = 0;
  file->pos = offset;

  if (file->inode.i_flags & EXT4_INLINE_DATA_FL)
    RETV_ERROR (ENOTSUP, -1);

  while (len > 0)
    {
      ret = ext2_sync_file_buffer_pos (file);
      if (ret)
	goto end;
      start = file->pos % fs->blksize;
      c = fs->blksize - start;
      if (c > len)
	c = len;
      ret = ext2_load_file_buffer (file, c == fs->blksize);
      if (ret)
	goto end;
      file->flags |= EXT2_FILE_BUFFER_DIRTY;
      memcpy (file->buffer + start, ptr, c);

      if (!file->physblock)
	{
	  ret = ext2_bmap (fs, file->ino, &file->inode,
			   file->buffer + fs->blksize,
			   file->ino ? BMAP_ALLOC : 0, file->block, 0,
			   &file->physblock);
	  if (ret)
	    goto end;
	}

      file->pos += c;
      ptr += c;
      count += c;
      len -= c;
    }

 end:
  if (count && EXT2_I_SIZE (file->inode) < file->pos)
    {
      int ret2 = ext2_file_set_size (file, file->pos);
      if (!ret)
	ret = ret2;
      if (!ret)
	vp->size = file->pos;
    }
  return ret ?: (ssize_t) count;
}

void
ext2_sync (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  ext2_file_flush (file);
}

int
ext2_create (struct vnode **result, struct vnode *dir, const char *name,
	     mode_t mode, dev_t rdev)
{
  return ext2_new_file (dir, name, (mode & ~S_IFMT) | S_IFREG, NULL);
}

int
ext2_mkdir (struct vnode **result, struct vnode *dir, const char *name,
	    mode_t mode)
{
  struct ext2_fs *fs = dir->mount->data;
  struct ext3_extent_handle *handle;
  struct ext2_inode inode;
  struct vnode *temp = NULL;
  block_t b;
  ino_t ino;
  char *block = NULL;
  int drop_ref = 0;
  int ret = ext2_read_bitmaps (fs);
  if (ret)
    goto end;

  ret = ext2_new_inode (fs, dir->ino, NULL, &ino);
  if (ret)
    goto end;

  memset (&inode, 0, sizeof (struct ext2_inode));
  ret = ext2_new_block (fs, ext2_find_inode_goal (fs, ino, &inode, 0),
			NULL, &b, NULL);
  if (ret)
    goto end;

  ret = ext2_new_dir_block (fs, ino, dir->ino, &block);
  if (ret)
    goto end;

  inode.i_mode = S_IFDIR | mode;
  inode.i_uid = THIS_PROCESS->euid;
  inode.i_gid = THIS_PROCESS->egid;
  if (fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_EXTENTS)
    inode.i_flags |= EXT4_EXTENTS_FL;
  else
    inode.i_block[0] = b;
  inode.i_size = fs->blksize;
  ext2_iblk_set (fs, &inode, 1);
  inode.i_links_count = 2;

  ret = ext2_write_new_inode (fs, ino, &inode);
  if (ret)
    goto end;

  temp = vnode_alloc ();
  if (UNLIKELY (!temp))
    GOTO_ERROR (ENOMEM, end);
  temp->ops = &ext2_vnode_ops;
  temp->data = malloc (sizeof (struct ext2_file));
  REF_ASSIGN (temp->mount, dir->mount);
  if (UNLIKELY (!temp->data))
    goto end;
  temp->ino = ino;
  ret = ext2_open_file (fs, ino, temp->data);
  if (ret)
    goto end;
  ret = ext2_write_dir_block (fs, b, block, 0, temp);
  if (ret)
    goto end;

  if (fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_EXTENTS)
    {
      ret = ext3_extent_open (fs, ino, &inode, &handle);
      if (ret)
	goto end;
      ret = ext3_extent_set_bmap (handle, 0, b, 0);
      if (ret)
	goto end;
    }

  ext2_block_alloc_stats (fs, b, 1);
  ext2_inode_alloc_stats (fs, ino, 1, 1);
  drop_ref = 1;

  ret = ext2_add_link (fs, dir, name, ino, EXT2_FILE_DIR);
  if (ret)
    goto end;
  if (dir->ino != ino)
    {
      dir->nlink++;
      ret = ext2_update_inode (fs, ino, &inode, sizeof (struct ext2_inode));
      if (ret)
	goto end;
    }
  drop_ref = 0;
  REF_OBJECT (temp);
  ext2_fill_vnode_data (temp);
  *result = temp;

 end:
  if (block)
    free (block);
  if (temp)
    UNREF_OBJECT (temp);
  if (drop_ref)
    {
      ext2_block_alloc_stats (fs, b, -1);
      ext2_inode_alloc_stats (fs, ino, -1, 1);
    }
  return ret;
}

int
ext2_rename (struct vnode *olddir, const char *oldname, struct vnode *newdir,
	     const char *newname)
{
  struct ext2_fs *fs = olddir->mount->data;
  ino_t ino;
  struct ext2_inode inode;
  int ret =
    ext2_lookup_inode (fs, olddir, oldname, strlen (oldname), NULL, &ino);
  if (ret)
    return ret;

  /* Remove existing link, if any */
  ret = ext2_unlink_dirent (fs, newdir, newname, 0);
  if (ret == -1 && errno != ENOENT)
    return ret;

  /* Determine mode and replace link */
  ret = ext2_read_inode (fs, ino, &inode);
  if (ret)
    return ret;
  ret = ext2_add_link (fs, newdir, newname, ino, inode.i_mode);
  if (ret)
    return ret;

  /* Remove old entry */
  return ext2_unlink_dirent (fs, olddir, oldname, 0);
}

int
ext2_link (struct vnode *dir, struct vnode *vp, const char *name)
{
  return ext2_add_link (dir->mount->data, dir, name, dir->ino,
			ext2_dir_type (dir->mode));
}

int
ext2_unlink (struct vnode *dir, const char *name)
{
  return ext2_unlink_dirent (dir->mount->data, dir, name, 0);
}

int
ext2_symlink (struct vnode *dir, const char *name, const char *target)
{
  struct ext2_fs *fs = dir->mount->data;
  blksize_t blksize = fs->blksize;
  block_t block;
  unsigned int target_len;
  ino_t ino;
  struct ext2_inode inode;
  char *blockbuf = NULL;
  int fast_link;
  int inline_link;
  int drop_ref = 0;
  int ret = ext2_read_bitmaps (fs);
  if (ret)
    return ret;

  target_len = strnlen (name, blksize + 1);
  if (target_len >= blksize)
    RETV_ERROR (ENAMETOOLONG, -1);

  blockbuf = calloc (blksize, 1);
  if (UNLIKELY (!blockbuf))
    RETV_ERROR (ENOMEM, -1);
  strncpy (blockbuf, name, blksize);

  fast_link = target_len < 60;
  if (!fast_link)
    {
      ret = ext2_new_block (fs, ext2_find_inode_goal (fs, dir->ino, NULL, 0),
			    NULL, &block, NULL);
      if (ret)
	goto end;
    }

  memset (&inode, 0, sizeof (struct ext2_inode));
  ret = ext2_new_inode (fs, dir->ino, NULL, &ino);
  if (ret)
    goto end;

  inode.i_mode = SYMLINK_MODE;
  inode.i_uid = THIS_PROCESS->euid;
  inode.i_gid = THIS_PROCESS->egid;
  inode.i_links_count = 1;
  ext2_inode_set_size (fs, &inode, target_len);

  inline_link = !fast_link
    && (fs->super.s_feature_incompat & EXT4_FT_INCOMPAT_INLINE_DATA);
  if (fast_link)
    strcpy ((char *) &inode.i_block, name);
  else if (inline_link)
    RETV_ERROR (ENOTSUP, -1);
  else
    {
      ext2_iblk_set (fs, &inode, 1);
      if (fs->super.s_feature_incompat & EXT3_FT_INCOMPAT_EXTENTS)
	inode.i_flags |= EXT4_EXTENTS_FL;
    }

  if (inline_link)
    ret = ext2_update_inode (fs, ino, &inode, sizeof (struct ext2_inode));
  else
    ret = ext2_write_new_inode (fs, ino, &inode);
  if (ret)
    goto end;

  if (!fast_link && !inline_link)
    {
      ret = ext2_bmap (fs, ino, &inode, NULL, BMAP_SET, 0, NULL, &block);
      if (ret)
	goto end;
      ret = ext2_write_blocks (blockbuf, fs, block, 1);
      if (ret)
	goto end;
      ext2_block_alloc_stats (fs, block, 1);
    }
  ext2_inode_alloc_stats (fs, ino, 1, 0);
  drop_ref = 1;

  ret = ext2_add_link (fs, dir, target, ino, EXT2_FILE_LNK);
  if (ret)
    goto end;
  drop_ref = 0;

 end:
  free (blockbuf);
  if (drop_ref)
    {
      if (!fast_link && !inline_link)
	ext2_block_alloc_stats (fs, block, -1);
      ext2_inode_alloc_stats (fs, ino, -1, 0);
    }
  return ret;
}

ssize_t
ext2_readlink (struct vnode *vp, char *buffer, size_t len)
{
  size_t size = vp->size;
  struct ext2_inode *inode = vp->data;
  size_t i;
  if (size <= 60)
    {
      /* Link target is stored directly in block pointers */
      for (i = 0; i < len && i < size; i++)
	buffer[i] = ((char *) inode->i_block)[i];
      return 0;
    }
  else
    {
      if (size < len)
	len = size;
      return ext2_read (vp, buffer, len, 0);
    }
}

int
ext2_truncate (struct vnode *vp, off_t len)
{
  return ext2_file_set_size (vp->data, len);
}

int
ext2_fill (struct vnode *vp)
{
  struct ext2_fs *fs = vp->mount->data;
  struct ext2_file *file = calloc (1, sizeof (struct ext2_file));
  if (UNLIKELY (!file))
    return -1;
  if (ext2_read_inode (fs, vp->ino, &file->inode))
    return -1;
  vp->data = file;
  ext2_fill_vnode_data (vp);
  return 0;

 err0:
  free (file);
  return -1;
}

void
ext2_dealloc (struct vnode *vp)
{
  struct ext2_file *file = vp->data;
  ext2_file_flush (file);
  free (file);
}
