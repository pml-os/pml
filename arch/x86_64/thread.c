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

#include <pml/alloc.h>
#include <pml/memory.h>
#include <pml/thread.h>
#include <stdlib.h>
#include <string.h>

static struct thread kernel_thread;
static struct process kernel_process;

lock_t thread_switch_lock;
struct process_queue process_queue;

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

/* Updates the stack pointer of the curren thread. */

void
thread_save_stack (void *stack)
{
  THIS_THREAD->args.stack = stack;
}

/* Switches to the next thread.
   TODO Support process priorities */

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

/* Creates a new thread of the current process. The new thread will begin
   execution at address @entry with the specified stack parameters. */

int
thread_new (struct thread_args *args)
{
  struct thread *thread;
  struct thread **queue;
  thread_switch_lock = 1;

  thread = malloc (sizeof (struct thread));
  if (UNLIKELY (!thread))
    goto err0;

  thread->tid = alloc_pid ();
  if (UNLIKELY (!thread->tid))
    goto err0;
  thread->process = THIS_PROCESS;
  thread->args.pml4t = args->pml4t;
  thread->args.stack = args->stack;
  thread->args.stack_base = args->stack_base;
  thread->args.stack_size = args->stack_size;
  thread->state = THREAD_STATE_RUNNING;

  /* Add thread to the process thread queue */
  queue = realloc (THIS_PROCESS->threads.queue,
		   sizeof (struct thread *) * ++THIS_PROCESS->threads.len);
  if (UNLIKELY (!queue))
    goto err1;
  THIS_PROCESS->threads.queue = queue;
  queue[THIS_PROCESS->threads.len - 1] = thread;
  thread_switch_lock = 0;
  return 0;

 err1:
  free_pid (thread->tid);
 err0:
  free (thread);
  thread_switch_lock = 0;
  return -1;
}

/* Clones the current thread and begins execution at @entry. */

int
thread_clone (void *entry)
{
  struct thread_args args;
  uintptr_t pml4t_phys;
  uint64_t *pml4t;
  uintptr_t stack_pdpt;

  /* Allocate a new PML4T */
  pml4t_phys = alloc_page ();
  if (UNLIKELY (!pml4t_phys))
    return -1;
  pml4t = (uint64_t *) PHYS_REL (pml4t_phys);
  memcpy (pml4t, THIS_THREAD->args.pml4t, PAGE_STRUCT_SIZE);

  /* Duplicate the stack */
  if (thread_dup_stack (&stack_pdpt))
    goto err0;
  pml4t[505] = stack_pdpt | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  args.pml4t = pml4t;
  args.stack = THIS_THREAD->args.stack;
  args.stack_base = THIS_THREAD->args.stack_base;
  args.stack_size = THIS_THREAD->args.stack_size;
  return thread_new (&args);

 err0:
  free_page (pml4t_phys);
  return 0;
}

/* Duplicates the current thread's stack. @result is set to the physical 
   address of the new PDPT. */

int
thread_dup_stack (uintptr_t *result)
{
  uintptr_t pdpt_phys = alloc_page ();
  uintptr_t pdt_phys;
  uint64_t *pdpt;
  uint64_t *pdt;
  uintptr_t addr;
  uintptr_t stack_end;
  uintptr_t i = 0;
  if (UNLIKELY (!pdpt_phys))
    return -1;
  pdpt = (uint64_t *) PHYS_REL (pdpt_phys);
  memcpy (pdpt, kernel_stack_pdpt, PAGE_STRUCT_SIZE);

  pdt_phys = alloc_page ();
  if (UNLIKELY (!pdt_phys))
    goto err0;
  pdt = (uint64_t *) PHYS_REL (pdt_phys);
  pdpt[511] = pdt_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;

  addr = (uintptr_t) THIS_THREAD->args.stack_base;
  stack_end = addr + THIS_THREAD->args.stack_size;
  for (; addr < stack_end; addr += PAGE_SIZE)
    {
      unsigned int pde = (addr >> 21) & 0x1ff;
      unsigned int pte = (addr >> 12) & 0x1ff;
      uint64_t *pt;
      void *page;
      if (!pdt[pde])
	{
	  /* Allocate a new page directory table */
	  pdt[pde] = alloc_page ();
	  if (UNLIKELY (!pdt[pde]))
	    goto err1;
	  pdt[pde] |= PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
	}

      /* Allocate a new page and copy the corresponding stack contents */
      pt = (uint64_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
      pt[pte] = alloc_page ();
      if (UNLIKELY (!pt[pte]))
	goto err1;
      pt[pte] |= PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
      page = (void *) PHYS_REL (ALIGN_DOWN (pt[pte], PAGE_SIZE));
      memcpy (page, (void *) addr, PAGE_SIZE);
    }
  *result = pdpt_phys;
  return 0;

 err1:
  /* Memory allocation failed in loop, free all previously allocated pages */
  while (i < addr)
    {
      unsigned int pde = (addr >> 21) & 0x1ff;
      if (pdt[pde])
	{
	  uint64_t *pt =
	    (uint64_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
	  int j;
	  for (j = 0; j < PAGE_STRUCT_ENTRIES; j++)
	    {
	      if (pt[j])
		free_page (pt[j]);
	    }
	  free_page (pdt[pde]);
	  pdt[pde] = 0;
	}
      i = ALIGN_UP (i, LARGE_PAGE_SIZE);
    }
  free_page (pdt_phys);
 err0:
  free_page (pdpt_phys);
  return -1;
}
