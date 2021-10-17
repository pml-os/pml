/* process.h -- This file is part of PML.
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

#ifndef __PML_PROCESS_H
#define __PML_PROCESS_H

/*!
 * @file
 * @brief Process definitions
 */

#include <pml/lock.h>
#include <pml/thread.h>

/*! Minimum process priority value */
#define PRIO_MIN                19
/*! Maximum process priority value */
#define PRIO_MAX                -20

/*! Expands to a pointer to the currently running process */
#define THIS_PROCESS (process_queue.queue[process_queue.front])

/*! Expands to a pointer to the currently running thread */
#define THIS_THREAD (THIS_PROCESS->threads.queue[THIS_PROCESS->threads.front])

/*!
 * Represents a process. Processes have a unique ID and also store their
 * parent process's ID. Each process is assigned a priority, but currently
 * process priorities are not implemented and are ignored.
 */

struct process
{
  pid_t pid;                    /*!< Process ID */
  pid_t ppid;                   /*!< Parent process ID */
  pid_t pgid;                   /*!< Process group ID */
  pid_t sid;                    /*!< Session ID */
  uid_t uid;                    /*!< Real user ID */
  uid_t euid;                   /*!< Effective user ID */
  gid_t gid;                    /*!< Real group ID */
  gid_t egid;                   /*!< Effective group ID */
  struct vnode *cwd;            /*!< Current working directory */
  char *cwd_path;               /*!< Absolute path to CWD */
  struct thread_queue threads;  /*!< Process thread queue */
  int priority;                 /*!< Process priority */
};

/*!
 * Queue of processes, used by the scheduler to schedule the next process.
 */

struct process_queue
{
  struct process **queue;
  size_t len;
  size_t front;
};

__BEGIN_DECLS

extern struct process_queue process_queue;
extern lock_t thread_switch_lock;

void init_pid_allocator (void);
pid_t alloc_pid (void);
void free_pid (pid_t pid);

struct process *process_alloc (int priority);
void process_free (struct process *process);
int process_enqueue (struct process *process);
struct process *process_fork (struct thread **t);
pid_t process_get_pid (struct process *process);

__END_DECLS

#endif
