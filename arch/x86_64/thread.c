/* thread.c -- This file is part of PML.
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

#include <pml/memory.h>
#include <pml/thread.h>
#include <stdlib.h>

static struct thread kernel_thread;
static struct process kernel_process;

lock_t thread_switch_lock;
struct process_queue process_queue;

void
sched_init (void)
{
  kernel_thread.pml4t = kernel_pml4t;
  kernel_process.threads.queue = malloc (sizeof (struct thread *));
  kernel_process.threads.queue[0] = &kernel_thread;
  kernel_process.threads.len = 1;
  kernel_process.priority = PRIO_MIN;
  process_queue.queue = malloc (sizeof (struct process *));
  process_queue.queue[0] = &kernel_process;
  process_queue.len = 1;
}

void
thread_save_stack (void *stack)
{
  THIS_THREAD->stack = stack;
}

/* Switches to the next thread.
   TODO Support process priorities */

void
thread_switch (void **stack, uintptr_t *pml4t_phys)
{
  /* Switch to the next thread in the current process */
  if (++THIS_PROCESS->threads.front == THIS_PROCESS->threads.len)
    {
      /* All threads have executed, go to the next process */
      THIS_PROCESS->threads.front = 0;
      if (++process_queue.front == process_queue.len)
	process_queue.front = 0;
    }
  *stack = THIS_THREAD->stack;
  *pml4t_phys = (uintptr_t) THIS_THREAD->pml4t - KERNEL_VMA;
}
