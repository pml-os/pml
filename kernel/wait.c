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
do_wait (pid_t pid, int *status, struct rusage *rusage, struct wait_state *ws)
{
  if (ws->status == PROCESS_WAIT_WAITING)
    return 0;
  if (!pid && THIS_PROCESS->pgid != ws->pgid)
    return 0;

  if (rusage)
    memcpy (rusage, &ws->rusage, sizeof (struct rusage));
  *status = (ws->code & 0xff) << 8;
  switch (ws->status)
    {
    case PROCESS_WAIT_SIGNALED:
      *status |= 0x01;
      break;
    case PROCESS_WAIT_STOPPED:
      *status |= 0x7f;
      break;
    }

  ws->status = PROCESS_WAIT_NONE;
  return ws->pid;
}

pid_t
sys_wait4 (pid_t pid, int *status, int flags, struct rusage *rusage)
{
  struct wait_state *ws = &THIS_PROCESS->wait;
  if (pid < -1)
    pid = -pid;
  if (!THIS_PROCESS->cpids.len)
    RETV_ERROR (ECHILD, -1);
  if (pid > 0 && !lookup_pid (pid))
    RETV_ERROR (ESRCH, -1);

  /* Fill wait state */
  ws->pid = pid;
  ws->status = PROCESS_WAIT_WAITING;
  ws->do_stopped = !!(flags & WUNTRACED);

  if (flags & WNOHANG)
    return do_wait (pid, status, rusage, ws);
  while (1)
    {
      pid_t ret = do_wait (pid, status, rusage, ws);
      if (ret)
	return ret;
      sched_yield ();
    }
}
