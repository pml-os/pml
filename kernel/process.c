/* process.c -- This file is part of PML.
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

#include <errno.h>
#include <string.h>

/*! System process queue. */
struct process_queue process_queue;

/*!
 * Set to nonzero when thread switching should be disabled. This is necessary
 * when modifying process or thread structures.
 */

lock_t thread_switch_lock;

/*!
 * Allocates a new process structure. The process will not be added to the
 * system process queue and will have no threads.
 *
 * @param priority the priority of the process
 * @return the process structure, or NULL on failure
 */

struct process *
process_alloc (int priority)
{
  struct process *process = calloc (1, sizeof (struct process));
  if (UNLIKELY (!process))
    return NULL;
  process->priority = priority;
  return process;
}

/*!
 * Frees a process. Any threads belonging to the process are also freed.
 *
 * @param process the process to free
 */

void
process_free (struct process *process)
{
  size_t i;
  if (process->threads.len)
    thread_unmap_user_mem (process->threads.queue[0]);
  for (i = 0; i < process->threads.len; i++)
    thread_free (process->threads.queue[i]);
  free (process->threads.queue);
  unmap_pid (process->pid);
  for (i = 0; i < process->fds.size; i++)
    {
      if (process->fds.table[i])
	free_fd (process, i);
    }
  for (i = 0; i < process->cpids.len; i++)
    {
      struct process *child = lookup_pid (process->cpids.pids[i]);
      if (LIKELY (child))
	child->ppid = 1;
    }
  free (process->cpids.pids);
  free (process);
}

/*!
 * Frees a terminated process's data.
 *
 * @param index index into the process queue of the freed process
 * @param status exit status of the process
 */

void
process_exit (unsigned int index, int status)
{
  thread_switch_lock = 1;
  process_free (process_queue.queue[index]);
  memmove (process_queue.queue + index, process_queue.queue + index + 1,
	   sizeof (struct process *) * (--process_queue.len - index));
  if (process_queue.front > index)
    process_queue.front--;
  thread_switch_lock = 0;
}

/*!
 * Adds a process to the process queue. This function must not be called
 * on a process already in the queue.
 *
 * @param process the process to enqueue
 * @return zero on success
 */

int
process_enqueue (struct process *process)
{
  struct process **queue;
  thread_switch_lock = 1;
  queue = realloc (process_queue.queue,
		   ++process_queue.len * sizeof (struct process *));
  if (UNLIKELY (!queue))
    {
      thread_switch_lock = 0;
      return -1;
    }
  queue[process_queue.len - 1] = process;
  process_queue.queue = queue;
  map_pid_process (process->pid, process);
  thread_switch_lock = 0;
  return 0;
}

/*!
 * Forks the currently running thread into a new process.
 *
 * @param t pointer to store forked thread
 * @param copy whether to copy the user-mode address space
 * @return the new process, or NULL on failure
 */

struct process *
process_fork (struct thread **t, int copy)
{
  struct thread *thread;
  struct process *process = process_alloc (THIS_PROCESS->priority);
  size_t i;
  if (UNLIKELY (!process))
    return NULL;

  /* Clone the current thread and attach it to the new process */
  thread = thread_clone (THIS_THREAD, copy);
  if (UNLIKELY (!thread))
    goto err0;
  if (thread_attach_process (process, thread))
    goto err1;
  process->pid = thread->tid;
  process->ppid = THIS_PROCESS->pid;
  process->pgid = THIS_PROCESS->pgid;
  process->sid = THIS_PROCESS->sid;
  process->uid = THIS_PROCESS->uid;
  process->euid = THIS_PROCESS->euid;
  process->suid = THIS_PROCESS->suid;
  process->gid = THIS_PROCESS->gid;
  process->egid = THIS_PROCESS->egid;
  process->sgid = THIS_PROCESS->sgid;
  REF_ASSIGN (process->cwd, THIS_PROCESS->cwd);
  *t = thread;

  /* Copy program break data */
  memcpy (&process->brk, &THIS_PROCESS->brk, sizeof (struct brk));

  /* Copy memory mapping data */
  process->mmaps.len = THIS_PROCESS->mmaps.len;
  process->mmaps.table = malloc (sizeof (struct mmap) * process->mmaps.len);
  if (UNLIKELY (!process->mmaps.table))
    goto err2;
  memcpy (process->mmaps.table, THIS_PROCESS->mmaps.table,
	  sizeof (struct mmap) * process->mmaps.len);

  /* Copy file descriptor table */
  process->fds.curr = THIS_PROCESS->fds.curr;
  process->fds.size = THIS_PROCESS->fds.size;
  process->fds.max_size = THIS_PROCESS->fds.max_size;
  process->fds.table = malloc (sizeof (struct fd *) * process->fds.size);
  if (UNLIKELY (!process->fds.table))
    goto err3;
  for (i = 0; i < process->fds.size; i++)
    {
      process->fds.table[i] = THIS_PROCESS->fds.table[i];
      if (process->fds.table[i])
	process->fds.table[i]->count++;
    }

  /* Add new process to child process table */
  THIS_PROCESS->cpids.pids =
    realloc (THIS_PROCESS->cpids.pids,
	     sizeof (pid_t *) * ++THIS_PROCESS->cpids.len);
  THIS_PROCESS->cpids.pids[THIS_PROCESS->cpids.len - 1] = process->pid;
  return process;

 err3:
  free (process->mmaps.table);
 err2:
  UNREF_OBJECT (process->cwd);
 err1:
  thread_free (thread);
 err0:
  process_free (process);
  return NULL;
}

/*!
 * Determines the PID of a process. This function is meant to be called by
 * assembly code.
 *
 * @param process the process
 * @return the PID of the process
 */

pid_t
process_get_pid (struct process *process)
{
  return process->pid;
}

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

void
process_fill_wait (struct process *process, int mode, int status)
{
  pid_t target = process->pid;
  pid_t pid = process->ppid;
  int direct = 1;
  while (pid != process->pid)
    {
      struct wait_state *ws;
      process = lookup_pid (pid);
      if (!process)
	break;
      ws = &process->wait;
      if (ws->status == PROCESS_WAIT_WAITING
	  && (ws->pid == target || (direct && (ws->pid == -1 || ws->pid == 0)))
	  && (!ws->do_stopped || mode != PROCESS_WAIT_STOPPED))
	{
	  ws->pid = target;
	  ws->pgid = process->pgid;
	  ws->status = mode;
	  ws->code = status;
	  memcpy (&ws->rusage, &THIS_PROCESS->self_rusage,
		  sizeof (struct rusage));
	}
      pid = process->ppid;
      direct = 0;
    }
}
