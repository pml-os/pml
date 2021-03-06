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

/*!
 * @file
 * @brief Macros and functions for threading support
 */

/*! Milliseconds of CPU time given to each thread */
#define THREAD_QUANTUM          20

#ifndef __ASSEMBLER__

#include <pml/vfs.h>
#include <pml/signal.h>

enum
{
  THREAD_STATE_RUNNING,         /*!< Thread is unblocked */
  THREAD_STATE_BLOCKED,         /*!< Thread is waiting for a semaphore */
  THREAD_STATE_IO               /*!< Thread is waiting for an I/O operation */
};

/*! Arguments used to create a new thread. */

struct thread_args
{
  uint64_t *pml4t;              /*!< Address of PML4T */
  void *stack;                  /*!< Address of stack pointer */
  void *stack_base;             /*!< Pointer to bottom of stack */
  size_t stack_size;            /*!< Size of stack */
};

/*!
 * Represents a queue of siginfo information for real-time signals.
 */

struct rtsig_queue
{
  siginfo_t *queue;             /*!< Signal information array */
  size_t len;                   /*!< Number of queued signals */
};

/*!
 * Represents a thread. Processes can have multiple threads, which share
 * the same process ID but have unique thread IDs. Threads have individual
 * page structures and stacks.
 */

struct thread
{
  pid_t tid;                    /*!< Thread ID */
  struct process *process;      /*!< Process this thread belongs to */
  struct thread_args args;      /*!< Properties of thread */
  int state;                    /*!< Thread state */
  int error;                    /*!< Thread-local error number (errno) */
  int sig;                      /*!< Number of queued signals */
  sigset_t sigblocked;          /*!< Mask of blocked signals */
  sigset_t sigpending;          /*!< Mask of pending blocked signals */
  sigset_t sigready;            /*!< Mask of signals ready to be handled */
  siginfo_t siginfo[SIGRTMIN];  /*!< Signal information array */
  struct rtsig_queue rtqueue[NSIG - SIGRTMIN]; /*!< Real-time signal queues */
  void *handler;                /*!< Signal handler ready to be executed */
  int hflags;                   /*!< Signal handler flags */
  int hsig;                     /*!< Signal number being handled */
  sigset_t hmask;               /*!< Signal mask requested by the handler */

  /*!
   * Set if the thread is currently executing a 'slow' system call. Slow
   * system calls may be interrupted by signals.
   *
   * @see SLOW_SYSCALL_BEGIN
   * @see SLOW_SYSCALL_END
   */
  volatile int slow_syscall;
};

/*!
 * Queue of threads, used by processes to keep track of their threads
 * and to schedule the next thread within a process.
 */

struct thread_queue
{
  struct thread **queue;
  size_t len;
  size_t front;
};

/*!
 * Linked list of threads, used by semaphores to keep track of which threads
 * are blocked waiting for them.
 */

struct thread_list
{
  struct thread *thread;
  struct thread_list *next;
};

__BEGIN_DECLS

extern unsigned int exit_process;

void sched_init (void);
void sched_exec (void *addr, char *const *argv, char *const *envp) __noreturn;
void sched_yield (void);
void sched_yield_to (void *addr) __noreturn;
void user_mode (void *addr) __noreturn;

struct thread *this_thread (void) __pure;
void thread_get_args (struct thread *thread, uintptr_t *pml4t, void **stack);
void thread_save_stack (struct thread *thread, void *stack);
void thread_switch (void **stack, uintptr_t *pml4t_phys);
struct thread *thread_create (struct thread_args *args);
int thread_alloc_tl_kernel_data (struct thread *thread);
void thread_free (struct thread *thread);
int thread_attach_process (struct process *process, struct thread *thread);
struct thread *thread_clone (struct thread *thread, int copy);
void thread_unmap_user_mem (struct thread *thread);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
