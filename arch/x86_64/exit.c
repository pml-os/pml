/* exit.c -- This file is part of PML.
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
#include <pml/syscall.h>
#include <string.h>

/*!
 * Index into the process queue of a process that recently terminated. The
 * scheduler will free a process listed in this variable on the next tick.
 */

unsigned int exit_process;

/*!
 * Status of a recently-terminated process. Processes terminated with a signal
 * will have a status equal to the signal number plus 128.
 */

int exit_status;

/*!
 * Fills wait structures of any parent processes on process exit. 
 * A parent waiting for a specific PID will be updated if the process with 
 * that PID is a descendent of the parent process, otherwise the child must 
 * be a direct descendent of the parent for the parent's wait status to be 
 * updated.
 *
 * @param process the exiting process
 * @param status exit code of the process
 */

static void
fill_wait (struct process *process, int status)
{
  pid_t target = process->pid;
  pid_t pid = process->ppid;
  int direct = 1;
  while (pid)
    {
      struct wait_state *ws;
      process = lookup_pid (pid);
      if (!process)
	break;
      ws = &process->wait;
      if (ws->status == PROCESS_WAIT_WAITING
	  && (ws->pid == target || (direct && (ws->pid == -1 || ws->pid == 0))))
	{
	  ws->pid = target;
	  ws->pgid = process->pgid;
	  ws->status = PROCESS_WAIT_EXITED;
	  ws->code = status;
	  memcpy (&ws->rusage, &THIS_PROCESS->self_rusage,
		  sizeof (struct rusage));
	}
      pid = process->ppid;
      direct = 0;
    }
}

void
sys_exit (int status)
{
  thread_switch_lock = 1;
  fill_wait (THIS_PROCESS, status);
  exit_process = process_queue.front;
  exit_status = status;
  thread_switch_lock = 0;
  sched_yield ();
  __builtin_unreachable ();
}
