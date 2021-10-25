/* perm.c -- This file is part of PML.
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

#include <pml/process.h>
#include <errno.h>

/*! Macro to pass a VFS permission check. */
#define PASS                    return 1
/*! Macro to fail a VFS permission check. */
#define FAIL                    RETV_ERROR (EACCES, 0)

/*!
 * Checks if the current process has permission to read from a file. If the
 * check fails, @ref errno is set to @ref EACCES. Directories additionally
 * need the execute bit set for the check to pass.
 *
 * @param vp the vnode to check
 * @param real whether to compare real or effective user/group IDs
 * @return nonzero if the check passed
 */

int
vfs_can_read (struct vnode *vp, int real)
{
  uid_t uid = real ? THIS_PROCESS->uid : THIS_PROCESS->euid;
  gid_t gid = real ? THIS_PROCESS->gid : THIS_PROCESS->egid;
  if (!uid)
    PASS; /* Super-user has all permissions */
  if (vp->uid == uid)
    {
      if (!(vp->mode & S_IRUSR))
	FAIL;
      if (S_ISDIR (vp->mode) && !(vp->mode & S_IXUSR))
	FAIL;
    }
  else if (vp->gid == gid)
    {
      if (!(vp->mode & S_IRGRP))
	FAIL;
      if (S_ISDIR (vp->mode) && !(vp->mode & S_IXGRP))
	FAIL;
    }
  else if (!(vp->mode & S_IROTH))
    FAIL;
  else if (S_ISDIR (vp->mode) && !(vp->mode & S_IXOTH))
    FAIL;
  PASS;
}

/*!
 * Checks if the current process has permission to write from a file. If the
 * check fails, @ref errno is set to @ref EACCES. Directories additionally
 * need the execute bit set for the check to pass.
 *
 * @param vp the vnode to check
 * @param real whether to compare real or effective user/group IDs
 * @return nonzero if the check passed
 */

int
vfs_can_write (struct vnode *vp, int real)
{
  uid_t uid = real ? THIS_PROCESS->uid : THIS_PROCESS->euid;
  gid_t gid = real ? THIS_PROCESS->gid : THIS_PROCESS->egid;
  if (!uid)
    PASS; /* Super-user has all permissions */
  if (vp->uid == uid)
    {
      if (!(vp->mode & S_IWUSR))
	FAIL;
      if (S_ISDIR (vp->mode) && !(vp->mode & S_IXUSR))
	FAIL;
    }
  else if (vp->gid == gid)
    {
      if (!(vp->mode & S_IWGRP))
	FAIL;
      if (S_ISDIR (vp->mode) && !(vp->mode & S_IXGRP))
	FAIL;
    }
  else if (!(vp->mode & S_IWOTH))
    FAIL;
  else if (S_ISDIR (vp->mode) && !(vp->mode & S_IXOTH))
    FAIL;
  PASS;
}

/*!
 * Checks if the current process has permission to execute a file. If the
 * check fails, @ref errno is set to @ref EACCES.
 *
 * @param vp the vnode to check
 * @param real whether to compare real or effective user/group IDs
 * @return nonzero if the check passed
 */

int
vfs_can_exec (struct vnode *vp, int real)
{
  uid_t uid = real ? THIS_PROCESS->uid : THIS_PROCESS->euid;
  gid_t gid = real ? THIS_PROCESS->gid : THIS_PROCESS->egid;
  if (!uid)
    {
      /* Super-user can execute as long as any execute bit is set */
      if (vp->mode & (S_IXUSR | S_IXGRP | S_IXOTH))
	PASS;
      else
	FAIL;
    }
  if (vp->uid == uid)
    {
      if (!(vp->mode & S_IXUSR))
	FAIL;
    }
  else if (vp->gid == gid)
    {
      if (!(vp->mode & S_IXGRP))
	FAIL;
    }
  else if (!(vp->mode & S_IXOTH))
    FAIL;
  PASS;
}
