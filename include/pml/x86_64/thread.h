/* thread.h -- This file is part of PML.
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

#ifndef __PML_THREAD_H
#define __PML_THREAD_H

#define THREAD_QUANTUM          20

#define PRIO_MIN                19
#define PRIO_MAX                -20

#define THIS_PROCESS (process_queue.queue[process_queue.front])
#define THIS_THREAD (THIS_PROCESS->threads.queue[THIS_PROCESS->threads.front])

#ifndef __ASSEMBLER__

#include <pml/lock.h>
#include <pml/types.h>

enum
{
  THREAD_STATE_RUNNING,
  THREAD_STATE_BLOCKED
};

struct thread_args
{
  uint64_t *pml4t;              /* Address of PML4T */
  void *stack;                  /* Address of stack pointer */
  void *stack_base;             /* Pointer to bottom of stack */
  size_t stack_size;            /* Size of stack */
};

struct thread
{
  pid_t tid;                    /* Thread ID */
  struct process *process;      /* Process this thread belongs to */
  struct thread_args args;      /* Properties of thread */
  int state;                    /* Thread state */
};

struct thread_queue
{
  struct thread **queue;
  size_t len;
  size_t front;
};

struct thread_list
{
  struct thread *thread;
  struct thread_list *next;
};

struct process
{
  pid_t pid;                    /* Process ID */
  pid_t ppid;                   /* Parent process ID */
  struct thread_queue threads;  /* Process thread queue */
  int priority;                 /* Process priority */
};

struct process_queue
{
  struct process **queue;
  size_t len;
  size_t front;
};

__BEGIN_DECLS

extern lock_t thread_switch_lock;
extern struct process_queue process_queue;

void sched_init (void);
void sched_yield (void);
void sched_yield_to (void *addr) __noreturn;

void thread_save_stack (void *stack);
void thread_switch (void **stack, uintptr_t *pml4t_phys);
struct thread *thread_create (struct thread_args *args);
void thread_free (struct thread *thread);
int thread_attach_process (struct process *process, struct thread *thread);
int thread_clone_stack (struct thread *thread, uintptr_t *pdpt);
int thread_exec (pid_t *tid, int (*func) (void *), void *arg);

void init_pid_allocator (void);
pid_t alloc_pid (void);
void free_pid (pid_t pid);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
