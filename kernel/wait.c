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
  struct wait_queue *waits = &THIS_PROCESS->waits;
  thread_switch_lock = 1;
  for (i = 0; i < waits->len; i++)
    {
      if (!(pid > 0 && waits->states[i].pid != pid)
	  && waits->states[i].status != PROCESS_WAIT_RUNNING
	  && !(!pid && waits->states[i].pgid != THIS_PROCESS->pgid))
	{
	  pid_t ret;
	  if (rusage)
	    memcpy (rusage, &waits->states[i].rusage, sizeof (struct rusage));
	  *status = (waits->states[i].code & 0xff) << 8;
	  switch (waits->states[i].status)
	    {
	    case PROCESS_WAIT_SIGNALED:
	      *status |= 0x01;
	      break;
	    case PROCESS_WAIT_STOPPED:
	      *status |= 0x7f;
	      break;
	    }
	  ret = waits->states[i].pid;
	  memmove (waits->states + i, waits->states + i + 1, --waits->len - i);
	  thread_switch_lock = 0;
	  return ret;
	}
    }
  thread_switch_lock = 0;
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
