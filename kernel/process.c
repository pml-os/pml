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

#include <pml/process.h>
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
    thread_free_user_mem (process->threads.queue[0]);
  for (i = 0; i < process->threads.len; i++)
    thread_free (process->threads.queue[i]);
  free (process->threads.queue);
  free (process);
}

/*!
 * Free's a terminated process's data.
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
  if (process_queue.len == process_queue.front)
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
  uintptr_t cr3;
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
  REF_ASSIGN (process->cwd, THIS_PROCESS->cwd);
  process->cwd_path = strdup (THIS_PROCESS->cwd_path);
  *t = thread;
  return process;

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
