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
#include <pml/process.h>
#include <errno.h>
#include <string.h>

static struct thread kernel_thread;
static struct process kernel_process;

/*!
 * Initializes the scheduler and sets up the kernel process and main thread.
 */

void
sched_init (void)
{
  kernel_thread.args.pml4t = kernel_pml4t;
  kernel_thread.args.stack_base =
    (void *) (PROCESS_STACK_BASE_VMA + 0x3fffc000);
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
 * Returns the currently running thread. This function is meant to be called
 * by assembly code, using @ref THIS_THREAD in C code is faster.
 *
 * @return the currently running thread
 */

struct thread *
this_thread (void)
{
  return THIS_THREAD;
}

/*!
 * Queries information about a thread. Pointers besides the thread structure
 * passed to this function can be NULL, in which case their corresponding
 * data in the thread structure will not be read. This function is meant to
 * be used from assembly code; it is faster to directly access the members of
 * @ref thread.args.
 *
 * @param thread the thread
 * @param pml4t pointer to store physical address of PML4T
 * @param stack pointer to store stack pointer
 */

void
thread_get_args (struct thread *thread, uintptr_t *pml4t, void **stack)
{
  if (pml4t)
    *pml4t = (uintptr_t) thread->args.pml4t - KERNEL_VMA;
  if (stack)
    *stack = thread->args.stack;
}

/*!
 * Updates the stack pointer of the current thread.
 *
 * @param stack the new stack pointer
 */

void
thread_save_stack (struct thread *thread, void *stack)
{
  thread->args.stack = stack;
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
  thread_get_args (THIS_THREAD, pml4t_phys, stack);
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
  unsigned int pml4e = PML4T_INDEX (THREAD_LOCAL_BASE_VMA);
  free_pid (thread->tid);
  if (thread->args.pml4t[pml4e] & PAGE_FLAG_PRESENT)
    {
      uintptr_t tlp_phys = ALIGN_DOWN (thread->args.pml4t[pml4e], PAGE_SIZE);
      uintptr_t *tlp = (uintptr_t *) PHYS_REL (tlp_phys);
      free_pdpt (tlp);
      free_page (tlp_phys);
    }
  free_virtual_page (thread->args.pml4t);
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
  process->threads.len--;
  return -1;
}

/*!
 * Clones a thread by creating another copy of the thread with the same
 * address space but a separate stack. An additional stack for kernel-mode
 * code is also created. The new thread will not be attached to a process.
 *
 * @param thread the thread to clone
 * @param copy whether to copy the user-mode address space
 * @return the cloned thread, or NULL on failure
 */

struct thread *
thread_clone (struct thread *thread, int copy)
{
  struct thread *t = malloc (sizeof (struct thread));
  uintptr_t *pml4t;
  uintptr_t *tlp;
  void *addr;
  void *cptr;
  pid_t tid;
  size_t i;
  if (UNLIKELY (!t))
    return NULL;
  pml4t = alloc_virtual_page ();
  if (UNLIKELY (!pml4t))
    goto err0;

  /* Fill new thread structure info */
  t->tid = alloc_pid ();
  if (UNLIKELY (t->tid == -1))
    goto err1;
  t->process = NULL;
  t->state = THREAD_STATE_RUNNING;
  t->args.pml4t = pml4t;
  t->args.stack = thread->args.stack;
  t->args.stack_base = thread->args.stack_base;
  t->args.stack_size = thread->args.stack_size;

  /* Fill PML4T and allocate new thread-local storage PDPT */
  tlp = alloc_virtual_page ();
  if (UNLIKELY (!tlp))
    goto err2;
  memcpy (pml4t, thread->args.pml4t, PAGE_STRUCT_SIZE);
  if (copy)
    {
      size_t i;
      for (i = 0; i < PAGE_STRUCT_ENTRIES / 2; i++)
	{
	  /* Mark allocated pages as copy-on-write */
	  if (pml4t[i] & PAGE_FLAG_PRESENT)
	    {
	      pml4t[i] &= ~PAGE_FLAG_RW;
	      pml4t[i] |= PAGE_FLAG_COW;
	    }
	}
    }
  pml4t[PML4T_INDEX (THREAD_LOCAL_BASE_VMA)] = ((uintptr_t) tlp - KERNEL_VMA)
    | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  for (addr = thread->args.stack_base;
       addr < thread->args.stack_base + thread->args.stack_size;
       addr += PAGE_SIZE)
    {
      uintptr_t page = alloc_page ();
      if (UNLIKELY (!page))
	goto err3;
      memcpy ((void *) PHYS_REL (page),
	      (void *) PHYS_REL (vm_phys_addr (thread->args.pml4t, addr)),
	      PAGE_SIZE);
      if (vm_map_page (pml4t, page, addr, PAGE_FLAG_RW | PAGE_FLAG_USER))
	{
	  free_page (page);
	  goto err3;
	}
    }
  for (i = 0; i < KERNEL_STACK_SIZE; i++)
    {
      uintptr_t page = alloc_page ();
      addr = (void *) (KERNEL_STACK_TOP_VMA - PAGE_SIZE - i);
      if (UNLIKELY (!page))
	goto err3;
      memcpy ((void *) PHYS_REL (page),
	      (void *) PHYS_REL (vm_phys_addr (thread->args.pml4t, addr)),
	      PAGE_SIZE);
      if (vm_map_page (pml4t, page, addr, PAGE_FLAG_RW | PAGE_FLAG_USER))
	{
	  free_page (page);
	  goto err3;
	}
    }
  return t;

 err3:
  free_pdpt (tlp);
 err2:
  free_pid (t->tid);
 err1:
  free_virtual_page (pml4t);
 err0:
  free (t);
  return NULL;
}

/*!
 * Frees the user-space memory allocated to a process. All threads share the
 * same user-mode address space, so this function can be called on any thread
 * belonging to a process.
 *
 * @param thread a thread belonging to the process
 */

void
thread_free_user_mem (struct thread *thread)
{
  size_t i;
  for (i = 0; i < PAGE_STRUCT_ENTRIES / 2; i++)
    {
      if (thread->args.pml4t[i] & PAGE_FLAG_PRESENT)
	{
	  uintptr_t pdpt = ALIGN_DOWN (thread->args.pml4t[i], PAGE_SIZE);
	  free_pdpt ((uintptr_t *) PHYS_REL (pdpt));
	  free_page (pdpt);
	}
    }
}
