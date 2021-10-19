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

/*! Number of file descriptors in system file descriptor table */
#define SYSTEM_FD_TABLE_SIZE    65536

/*! Minimum process priority value */
#define PRIO_MIN                19
/*! Maximum process priority value */
#define PRIO_MAX                -20

/*! Expands to a pointer to the currently running process */
#define THIS_PROCESS (process_queue.queue[process_queue.front])

/*! Expands to a pointer to the currently running thread */
#define THIS_THREAD (THIS_PROCESS->threads.queue[THIS_PROCESS->threads.front])

/*!
 * Represents an entry in the system file descriptor table. This structure
 * stores the underlying vnode corresponding to an open file as well as other
 * info exposed through the syscall API like file offsets, access mode, etc.
 */

struct fd
{
  struct vnode *vnode;          /*!< Vnode of file */
  char *path;                   /*!< Absolute path to file */
  off_t offset;                 /*!< Current file offset */
  int flags;                    /*!< Flags used to open file */
  /*! Number of process file descriptors holding a reference */
  size_t count;
};

/*!
 * File descriptor table stored by each process. Contains an array of indexes
 * into the system file descriptor table, which can be used to access a file's
 * vnode and other information.
 */

struct fd_table
{
  struct fd **table;            /*!< File descriptors */
  size_t curr;                  /*!< Index in table to start searches */
  size_t size;                  /*!< Number of entries in table */
  size_t max_size;              /*!< Soft limit on number of file descriptors */
};

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
  struct fd_table fds;          /*!< File descriptor table */
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
extern struct fd *system_fd_table;

void init_pid_allocator (void);
pid_t alloc_pid (void);
void free_pid (pid_t pid);

int alloc_fd (void);
void free_fd (int fd);

struct process *process_alloc (int priority);
void process_free (struct process *process);
int process_enqueue (struct process *process);
struct process *process_fork (struct thread **t, int copy);
pid_t process_get_pid (struct process *process);

void user_mode (void *addr) __noreturn;

__END_DECLS

#endif