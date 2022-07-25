/* wait.c -- This file is part of PML.
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

#include <pml/syscall.h>
#include <pml/wait.h>
#include <errno.h>
#include <string.h>

static pid_t
do_wait (pid_t pid, int *status, struct rusage *rusage)
{
  size_t i;
  struct child_table *children = &THIS_PROCESS->children;
  for (i = 0; i < children->len; i++)
    {
      if (!(pid > 0 && children->info[i].pid != pid)
	  && children->info[i].status != PROCESS_WAIT_RUNNING
	  && !(!pid && children->info[i].pgid != THIS_PROCESS->pgid))
	{
	  int remove = 1;
	  if (rusage)
	    memcpy (rusage, &children->info[i].rusage, sizeof (struct rusage));
	  *status = (children->info[i].code & 0xff) << 8;
	  switch (children->info[i].status)
	    {
	    case PROCESS_WAIT_SIGNALED:
	      *status |= 0x01;
	      break;
	    case PROCESS_WAIT_STOPPED:
	      *status |= 0x7f;
	      remove = 0;
	      break;
	    }
	  if (remove)
	    memmove (children->info + i, children->info + i + 1,
		     --children->len - i);
	  else
	    {
	      children->info[i].status = PROCESS_WAIT_STOPPED;
	      children->info[i].code = 0;
	    }
	  return children->info[i].pid;
	}
    }
  return 0;
}

pid_t
sys_wait4 (pid_t pid, int *status, int flags, struct rusage *rusage)
{
  if (pid < -1)
    pid = -pid;
  if (!THIS_PROCESS->children.len)
    RETV_ERROR (ECHILD, -1);
  if (pid > 0 && !lookup_pid (pid))
    RETV_ERROR (ESRCH, -1);

  if (flags & WNOHANG)
    return do_wait (pid, status, rusage);
  while (1)
    {
      pid_t ret = do_wait (pid, status, rusage);
      if (ret)
	return ret;
      sched_yield ();
    }
}
