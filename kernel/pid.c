/* pid.c -- This file is part of PML.
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

#include <pml/map.h>
#include <pml/panic.h>
#include <pml/syscall.h>
#include <errno.h>
#include <string.h>

#define PID_BITMAP_INCREMENT    1024
#define PID_BITMAP_SIZE_LIMIT   32768

static void *pid_bitmap;        /* Bitmap of allocated PIDs */
static size_t next_pid;         /* PID to start searching from */
static size_t pid_bitmap_size;  /* Size of bitmap in bytes */
static lock_t pid_bitmap_lock;
static struct hashmap *pid_hashmap; /* Map of PIDs to process structures */

/*!
 * Initializes the PID allocator by marking PID 0 as used and allocating
 * the PID bitmap.
 */

void
init_pid_allocator (void)
{
  pid_bitmap = calloc (PID_BITMAP_INCREMENT, 1);
  if (UNLIKELY (!pid_bitmap))
    panic ("Failed to allocate PID bitmap");
  pid_bitmap_size = PID_BITMAP_INCREMENT;
  set_bit (pid_bitmap, 0);
  next_pid = 1;

  pid_hashmap = hashmap_create ();
  if (UNLIKELY (!pid_hashmap))
    panic ("Failed to create PID hashmap\n");
  map_pid_process (0, process_queue.queue[0]);
}

/*!
 * Returns a number suitable as a process or thread ID. The returned value
 * is guaranteed to be unique from all other values returned from this
 * function unless freed with free_pid().
 *
 * @return an unused ID, or -1 if no ID could be allocated
 */

pid_t
alloc_pid (void)
{
  void *temp;
  spinlock_acquire (&pid_bitmap_lock);
  while (pid_bitmap_size < PID_BITMAP_SIZE_LIMIT)
    {
      for (; next_pid < pid_bitmap_size * 8; next_pid++)
	{
	  if (!test_bit (pid_bitmap, next_pid))
	    {
	      pid_t pid = next_pid++;
	      set_bit (pid_bitmap, pid);
	      spinlock_release (&pid_bitmap_lock);
	      return pid;
	    }
	}

      /* No PID was found, enlarge the bitmap and try again */
      pid_bitmap_size += PID_BITMAP_INCREMENT;
      temp = realloc (pid_bitmap, pid_bitmap_size);
      if (UNLIKELY (!temp))
	{
	  spinlock_release (&pid_bitmap_lock);
	  return -1;
	}
      pid_bitmap = temp;
      memset (pid_bitmap + pid_bitmap_size - PID_BITMAP_INCREMENT, 0,
	      PID_BITMAP_INCREMENT);
    }
  spinlock_release (&pid_bitmap_lock);
  RETV_ERROR (ENOMEM, -1);
}

/*!
 * Unallocates a process or thread ID so it can be reused by other processes.
 *
 * @param pid the ID to unallocate
 */

void
free_pid (pid_t pid)
{
  if (UNLIKELY (pid < 0))
    return;
  spinlock_acquire (&pid_bitmap_lock);
  clear_bit (pid_bitmap, pid);
  if ((size_t) pid < next_pid)
    next_pid = pid;
  spinlock_release (&pid_bitmap_lock);
}

/*!
 * Inserts a mapping into the PID hashmap. This function panics on failure.
 *
 * @param pid the process ID
 * @param process the process structure corresponding to the PID
 */

void
map_pid_process (pid_t pid, struct process *process)
{
  hash_t key = siphash (&pid, sizeof (pid_t), 0);
  if (hashmap_insert (pid_hashmap, key, process))
    panic ("Failed to add into PID hashmap\n");
}

/*!
 * Removes a mapping from the PID hashmap, if one exists.
 *
 * @param pid the process ID
 */

void
unmap_pid (pid_t pid)
{
  hash_t key = siphash (&pid, sizeof (pid_t), 0);
  hashmap_remove (pid_hashmap, key);
}

/*!
 * Locates the process structure of the process with the given ID.
 *
 * @param pid the process ID
 * @return the process structure, or NULL if no process exists with that PID
 */

struct process *
lookup_pid (pid_t pid)
{
  hash_t key = siphash (&pid, sizeof (pid_t), 0);
  return hashmap_lookup (pid_hashmap, key);
}

pid_t
sys_getpid (void)
{
  return THIS_PROCESS->pid;
}

pid_t
sys_getppid (void)
{
  return THIS_PROCESS->ppid;
}

pid_t
sys_gettid (void)
{
  return THIS_THREAD->tid;
}

pid_t
sys_getpgid (pid_t pid)
{
  struct process *process = pid ? lookup_pid (pid) : THIS_PROCESS;
  if (!process)
    RETV_ERROR (ESRCH, -1);
  return process->pgid;
}

int
sys_setpgid (pid_t pid, pid_t pgid)
{
  struct process *process;
  struct process *leader;
  if (pgid < 0)
    RETV_ERROR (EINVAL, -1);
  process = pid ? lookup_pid (pid) : THIS_PROCESS;
  if (!process)
    RETV_ERROR (ESRCH, -1);
  if (!pgid)
    pgid = process->pid;

  /* Check if trying to change process group ID of a session leader */
  if (process->sid == pid)
    RETV_ERROR (EPERM, -1);

  /* Check if trying to move process into a different session */
  leader = lookup_pid (pgid);
  if (!leader)
    RETV_ERROR (ESRCH, -1);
  if (process->sid != leader->sid)
    RETV_ERROR (EPERM, -1);

  process->pgid = pgid;
  return 0;
}

pid_t
sys_getpgrp (void)
{
  return THIS_PROCESS->pgid;
}

int
sys_setpgrp (void)
{
  return sys_setpgid (0, 0);
}

uid_t
sys_getuid (void)
{
  return THIS_PROCESS->uid;
}

int
sys_setuid (uid_t uid)
{
  if (THIS_PROCESS->euid && THIS_PROCESS->uid != uid)
    RETV_ERROR (EPERM, -1);
  THIS_PROCESS->uid = uid;
  THIS_PROCESS->euid = uid;
  THIS_PROCESS->suid = uid;
  return 0;
}

uid_t
sys_geteuid (void)
{
  return THIS_PROCESS->euid;
}

int
sys_seteuid (uid_t euid)
{
  return sys_setresuid (-1, euid, -1);
}

int
sys_setreuid (uid_t ruid, uid_t euid)
{
  if (THIS_PROCESS->euid)
    {
      if (ruid != (uid_t) -1)
	{
	  if (ruid != THIS_PROCESS->uid && ruid != THIS_PROCESS->euid)
	    RETV_ERROR (EPERM, -1);
	}
      if (euid != (uid_t) -1)
	{
	  if (euid != THIS_PROCESS->uid && euid != THIS_PROCESS->euid
	      && euid != THIS_PROCESS->suid)
	    RETV_ERROR (EPERM, -1);
	}
    }
  if (euid != (uid_t) -1)
    {
      if (THIS_PROCESS->uid != euid)
	THIS_PROCESS->suid = euid;
      THIS_PROCESS->euid = euid;
    }
  if (ruid != (uid_t) -1)
    {
      THIS_PROCESS->uid = ruid;
      THIS_PROCESS->suid = THIS_PROCESS->euid;
    }
  return 0;
}

int
sys_getresuid (uid_t *ruid, uid_t *euid, uid_t *suid)
{
  if (ruid)
    *ruid = THIS_PROCESS->uid;
  if (euid)
    *euid = THIS_PROCESS->euid;
  if (suid)
    *suid = THIS_PROCESS->suid;
  return 0;
}

int
sys_setresuid (uid_t ruid, uid_t euid, uid_t suid)
{
  if (THIS_PROCESS->euid)
    {
      if (ruid != (uid_t) -1)
	{
	  if (ruid != THIS_PROCESS->uid && ruid != THIS_PROCESS->euid
	      && ruid != THIS_PROCESS->suid)
	    RETV_ERROR (EPERM, -1);
	}
      if (euid != (uid_t) -1)
	{
	  if (euid != THIS_PROCESS->uid && euid != THIS_PROCESS->euid
	      && euid != THIS_PROCESS->suid)
	    RETV_ERROR (EPERM, -1);
	}
      if (suid != (uid_t) -1)
	{
	  if (suid != THIS_PROCESS->uid && suid != THIS_PROCESS->euid
	      && suid != THIS_PROCESS->suid)
	    RETV_ERROR (EPERM, -1);
	}
    }
  if (ruid != (uid_t) -1)
    THIS_PROCESS->uid = ruid;
  if (euid != (uid_t) -1)
    THIS_PROCESS->euid = euid;
  if (suid != (uid_t) -1)
    THIS_PROCESS->suid = suid;
  return 0;
}

gid_t
sys_getgid (void)
{
  return THIS_PROCESS->gid;
}

int
sys_setgid (gid_t gid)
{
  if (THIS_PROCESS->euid && THIS_PROCESS->gid != gid)
    RETV_ERROR (EPERM, -1);
  THIS_PROCESS->gid = gid;
  THIS_PROCESS->egid = gid;
  THIS_PROCESS->sgid = gid;
  return 0;
}

gid_t
sys_getegid (void)
{
  return THIS_PROCESS->egid;
}

int
sys_setegid (gid_t egid)
{
  return sys_setresgid (-1, egid, -1);
}

int
sys_setregid (gid_t rgid, gid_t egid)
{
  if (THIS_PROCESS->euid)
    {
      if (rgid != (gid_t) -1)
	{
	  if (rgid != THIS_PROCESS->gid && rgid != THIS_PROCESS->egid)
	    RETV_ERROR (EPERM, -1);
	}
      if (egid != (gid_t) -1)
	{
	  if (egid != THIS_PROCESS->gid && egid != THIS_PROCESS->egid
	      && egid != THIS_PROCESS->sgid)
	    RETV_ERROR (EPERM, -1);
	}
    }
  if (egid != (gid_t) -1)
    {
      if (THIS_PROCESS->gid != egid)
	THIS_PROCESS->sgid = egid;
      THIS_PROCESS->egid = egid;
    }
  if (rgid != (gid_t) -1)
    {
      THIS_PROCESS->gid = rgid;
      THIS_PROCESS->sgid = THIS_PROCESS->egid;
    }
  return 0;
}

int
sys_getresgid (gid_t *rgid, gid_t *egid, gid_t *sgid)
{
  if (rgid)
    *rgid = THIS_PROCESS->gid;
  if (egid)
    *egid = THIS_PROCESS->egid;
  if (sgid)
    *sgid = THIS_PROCESS->sgid;
  return 0;
}

int
sys_setresgid (gid_t rgid, gid_t egid, gid_t sgid)
{
  if (THIS_PROCESS->euid)
    {
      if (rgid != (gid_t) -1)
	{
	  if (rgid != THIS_PROCESS->gid && rgid != THIS_PROCESS->egid
	      && rgid != THIS_PROCESS->sgid)
	    RETV_ERROR (EPERM, -1);
	}
      if (egid != (gid_t) -1)
	{
	  if (egid != THIS_PROCESS->gid && egid != THIS_PROCESS->egid
	      && egid != THIS_PROCESS->sgid)
	    RETV_ERROR (EPERM, -1);
	}
      if (sgid != (gid_t) -1)
	{
	  if (sgid != THIS_PROCESS->gid && sgid != THIS_PROCESS->egid
	      && sgid != THIS_PROCESS->sgid)
	    RETV_ERROR (EPERM, -1);
	}
    }
  if (rgid != (gid_t) -1)
    THIS_PROCESS->gid = rgid;
  if (egid != (gid_t) -1)
    THIS_PROCESS->egid = egid;
  if (sgid != (gid_t) -1)
    THIS_PROCESS->sgid = sgid;
  return 0;
}

int
sys_getgroups (int size, gid_t *list)
{
  if (!size)
    return THIS_PROCESS->nsup_gids;
  else if (THIS_PROCESS->nsup_gids > (size_t) size)
    RETV_ERROR (EINVAL, -1);
  memcpy (list, THIS_PROCESS->sup_gids,
	  sizeof (gid_t) * THIS_PROCESS->nsup_gids);
  return 0;
}

int
sys_setgroups (size_t size, const gid_t *list)
{
  if (THIS_PROCESS->euid)
    RETV_ERROR (EPERM, -1);
  else if (size > NGROUPS_MAX)
    RETV_ERROR (EINVAL, -1);
  memcpy (THIS_PROCESS->sup_gids, list, sizeof (gid_t) * size);
  THIS_PROCESS->nsup_gids = size;
  return 0;
}
