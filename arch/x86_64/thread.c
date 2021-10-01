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

/*! @file */

#include <pml/alloc.h>
#include <pml/memory.h>
#include <pml/thread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static struct thread kernel_thread;
static struct process kernel_process;

lock_t thread_switch_lock;
struct process_queue process_queue;

/*!
 * Initializes the scheduler and sets up the kernel process and main thread.
 */

void
sched_init (void)
{
  kernel_thread.args.pml4t = kernel_pml4t;
  kernel_thread.args.stack_base =
    (void *) (PROCESS_STACK_BASE_VMA + 0xffffffc000);
  kernel_thread.args.stack_size = 16384;
  kernel_thread.state = THREAD_STATE_RUNNING;
  kernel_process.threads.queue = malloc (sizeof (struct thread *));
  kernel_process.threads.queue[0] = &kernel_thread;
  kernel_process.threads.len = 1;
  kernel_process.priority = PRIO_MIN;
  process_queue.queue = malloc (sizeof (struct process *));
  process_queue.queue[0] = &kernel_process;
  process_queue.len = 1;
}

/*!
 * Updates the stack pointer of the current thread.
 *
 * @param stack the new stack pointer
 */

void
thread_save_stack (void *stack)
{
  THIS_THREAD->args.stack = stack;
}

/*!
 * Switches to the next thread.
 *
 * @todo support process priorities
 * @param stack pointer to store new thread stack address
 * @param pml4t_phys pointer to store new thread PML4T physical address
 */

void
thread_switch (void **stack, uintptr_t *pml4t_phys)
{
  /* Switch to the next unblocked thread in the current process */
  do
    {
      if (++THIS_PROCESS->threads.front == THIS_PROCESS->threads.len)
	{
	  /* All threads have executed, go to the next process */
	  THIS_PROCESS->threads.front = 0;
	  if (++process_queue.front == process_queue.len)
	    process_queue.front = 0;
	}
    }
  while (THIS_THREAD->state != THREAD_STATE_RUNNING);
  *stack = THIS_THREAD->args.stack;
  *pml4t_phys = (uintptr_t) THIS_THREAD->args.pml4t - KERNEL_VMA;
}

/*!
 * Creates a new thread with the given arguments, allocates a thread ID, and
 * sets its state to running. The returned thread does not correspond to
 * any process.
 *
 * @param args thread arguments
 * @return the new thread
 */

struct thread *
thread_create (struct thread_args *args)
{
  struct thread *thread = malloc (sizeof (struct thread));
  if (UNLIKELY (!thread))
    return NULL;
  thread->tid = alloc_pid ();
  if (UNLIKELY (!thread->tid))
    goto err0;
  thread->process = NULL;
  memcpy (&thread->args, args, sizeof (struct thread_args));
  thread->state = THREAD_STATE_RUNNING;
  return thread;

 err0:
  free (thread);
  return NULL;
}

/*!
 * Destroys a thread. Its thread ID will be unallocated for use by other
 * threads or processes, and its stack and any thread-local data will
 * be unallocated. The thread will not be removed from its parent's queue.
 *
 * @param thread the thread to destroy
 */

void
thread_free (struct thread *thread)
{
  /* Unallocate the thread's ID */
  free_pid (thread->tid);

  /* Unallocate the thread's stack and TLS */
  if (thread->args.pml4t[507] & PAGE_FLAG_PRESENT)
    {
      uintptr_t tlp_phys = ALIGN_DOWN (thread->args.pml4t[507], PAGE_SIZE);
      uintptr_t *tlp = (uintptr_t *) PHYS_REL (tlp_phys);
      free_pdpt (tlp);
      free_page (tlp_phys);
    }
  free_page ((uintptr_t) thread->args.pml4t - KERNEL_VMA);

  /* Free the thread structure */
  free (thread);
}

/*!
 * Attaches a thread as a child of a process.
 *
 * @param process the target process
 * @param thread the target thread
 * @return zero on success
 */

int
thread_attach_process (struct process *process, struct thread *thread)
{
  struct thread **queue;
  thread_switch_lock = 1;
  thread->process = process;
  queue = realloc (process->threads.queue,
		   sizeof (struct thread *) * ++process->threads.len);
  if (UNLIKELY (!queue))
    goto err0;
  process->threads.queue = queue;
  process->threads.queue[process->threads.len - 1] = thread;
  thread_switch_lock = 0;
  return 0;

 err0:
  thread_switch_lock = 0;
  return -1;
}

/*!
 * Clones a thread's stack and updates a PDPT.
 *
 * @param thread the thread whose stack will be cloned
 * @param pdpt the PDPT used to update the stack pointer entries
 * @return zero on success
 */

int
thread_clone_stack (struct thread *thread, uintptr_t *pdpt)
{
  uintptr_t *thread_local_pdpt =
    (uintptr_t *) PHYS_REL (ALIGN_DOWN (thread->args.pml4t[507], PAGE_SIZE));
  uintptr_t *stack_pdt =
    (uintptr_t *) PHYS_REL (ALIGN_DOWN (thread_local_pdpt[511], PAGE_SIZE));
  uintptr_t new_stack_pdt_phys = alloc_page ();
  uintptr_t *new_stack_pdt;
  size_t i;
  if (UNLIKELY (!new_stack_pdt_phys))
    return -1;
  new_stack_pdt = (uintptr_t *) PHYS_REL (new_stack_pdt_phys);
  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
    new_stack_pdt[i] = stack_pdt[i] | PAGE_NP_FLAG_COW;
  pdpt[511] =
    new_stack_pdt_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  return 0;
}

/*!
 * Adds a new thread to the current process.
 *
 * @param tid pointer to store the thread ID of the new thread
 * @param func function to begin execution in new thread
 * @param arg argument passed to the starting function
 * @return zero on success
 */

int
thread_exec (pid_t *tid, int (*func) (void *), void *arg)
{
  struct thread *thread;
  struct thread_args args;
  uintptr_t pml4t_phys;
  uint64_t *pml4t;
  uintptr_t tlp_phys;
  uintptr_t *tlp;

  /* Allocate a new PML4T */
  pml4t_phys = alloc_page ();
  if (UNLIKELY (!pml4t_phys))
    return -1;
  pml4t = (uint64_t *) PHYS_REL (pml4t_phys);
  memcpy (pml4t, THIS_THREAD->args.pml4t, PAGE_STRUCT_SIZE);

  /* Create new thread */
  args.pml4t = pml4t;
  args.stack = THIS_THREAD->args.stack;
  args.stack_base = THIS_THREAD->args.stack_base;
  args.stack_size = THIS_THREAD->args.stack_size;
  thread = thread_create (&args);
  if (UNLIKELY (!thread))
    goto err0;

  /* Clear thread-local storage and clone the stack */
  tlp_phys = alloc_page ();
  if (UNLIKELY (!tlp_phys))
    goto err1;
  tlp = (uintptr_t *) PHYS_REL (tlp_phys);
  if (thread_clone_stack (thread, tlp))
    goto err1;
  /* TODO Setup the new thread's stack frame to return from an IRQ to the
     given function */
  pml4t[507] = tlp_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  *tid = thread->tid;
  if (thread_attach_process (THIS_PROCESS, thread))
    goto err1;
  return 0;

 err1:
  thread_free (thread);
 err0:
  free_page (pml4t_phys);
  return -1;
}
