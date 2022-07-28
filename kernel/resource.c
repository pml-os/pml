/* resource.c -- This file is part of PML.
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

#include <pml/syscall.h>
#include <errno.h>
#include <string.h>

int
sys_getrusage (int who, struct rusage *rusage)
{
  switch (who)
    {
    case RUSAGE_SELF:
      memcpy (rusage, &THIS_PROCESS->self_rusage, sizeof (struct rusage));
      return 0;
    case RUSAGE_CHILDREN:
      memcpy (rusage, &THIS_PROCESS->child_rusage, sizeof (struct rusage));
      return 0;
    default:
      RETV_ERROR (EINVAL, -1);
    }
}

int
sys_getpriority (int which, id_t who)
{
  struct process *process;
  int prio = PRIO_MIN + 1;
  size_t i;
  switch (which)
    {
    case PRIO_PROCESS:
      process = who ? lookup_pid (who) : THIS_PROCESS;
      if (!process)
	RETV_ERROR (ESRCH, -1);
      prio = process->priority;
      break;
    case PRIO_PGRP:
      if (!who)
	who = THIS_PROCESS->pgid;
      for (i = 0; i < process_queue.len; i++)
	{
	  process = process_queue.queue[i];
	  if ((id_t) process->pgid == who && process->priority < prio)
	    prio = process_queue.queue[i]->priority;
	}
      break;
    case PRIO_USER:
      if (!who)
	who = THIS_PROCESS->uid;
      for (i = 0; i < process_queue.len; i++)
	{
	  process = process_queue.queue[i];
	  if (process->euid == who && process->priority < prio)
	    prio = process_queue.queue[i]->priority;
	}
      break;
    default:
      RETV_ERROR (EINVAL, -1);
    }
  if (prio > PRIO_MIN)
    RETV_ERROR (ESRCH, -1);
  SYSCALL_ERROR_OK;
  return prio;
}

int
sys_setpriority (int which, id_t who, int prio)
{
  struct process *process;
  size_t i;
  if (prio > PRIO_MIN)
    prio = PRIO_MIN;
  else if (prio < PRIO_MAX)
    prio = PRIO_MAX;
  switch (which)
    {
    case PRIO_PROCESS:
      process = who ? lookup_pid (who) : THIS_PROCESS;
      if (!process)
	RETV_ERROR (ESRCH, -1);
      if (THIS_PROCESS->euid && THIS_PROCESS->euid != process->euid
	  && THIS_PROCESS->euid != process->uid)
	RETV_ERROR (EPERM, -1);
      if (THIS_PROCESS->euid && prio < process->priority)
	RETV_ERROR (EACCES, -1);
      process->priority = prio;
      break;
    case PRIO_PGRP:
      if (!who)
	who = THIS_PROCESS->pgid;
      for (i = 0; i < process_queue.len; i++)
	{
	  process = process_queue.queue[i];
	  if ((id_t) process->pgid == who)
	    {
	      if (THIS_PROCESS->euid && THIS_PROCESS->euid != process->euid
		  && THIS_PROCESS->euid != process->uid)
		RETV_ERROR (EPERM, -1);
	      if (THIS_PROCESS->euid && prio < process->priority)
		RETV_ERROR (EACCES, -1);
	      process->priority = prio;
	    }
	}
      break;
    case PRIO_USER:
      if (!who)
	who = THIS_PROCESS->uid;
      for (i = 0; i < process_queue.len; i++)
	{
	  process = process_queue.queue[i];
	  if (process->euid == who)
	    {
	      if (THIS_PROCESS->euid && THIS_PROCESS->euid != process->euid
		  && THIS_PROCESS->euid != process->uid)
		RETV_ERROR (EPERM, -1);
	      if (THIS_PROCESS->euid && prio < process->priority)
		RETV_ERROR (EACCES, -1);
	      process->priority = prio;
	    }
	}
      break;
    default:
      RETV_ERROR (EINVAL, -1);
    }
  return 0;
}
