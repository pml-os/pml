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
#include <pml/mman.h>
#include <pml/resource.h>
#include <pml/syslimits.h>
#include <pml/thread.h>

/*! Number of file descriptors in system file descriptor table */
#define SYSTEM_FD_TABLE_SIZE    65536
/*! Default maximum size of process data segment */
#define DATA_SEGMENT_MAX        0x10000000000
/*! Size of per-process kernel-mode stack */
#define KERNEL_STACK_SIZE       16384

/*! Expands to a pointer to the currently running process */
#define THIS_PROCESS (process_queue.queue[process_queue.front])
/*! Expands to a pointer to the currently running thread */
#define THIS_THREAD (THIS_PROCESS->threads.queue[THIS_PROCESS->threads.front])

#define PROCESS_WAIT_RUNNING    0       /*!< The process is running */
#define PROCESS_WAIT_EXITED     1       /*!< The process exited normally */
#define PROCESS_WAIT_SIGNALED   2       /*!< The process was signaled */
#define PROCESS_WAIT_STOPPING   3       /*!< The process is stopping */
#define PROCESS_WAIT_STOPPED    4       /*!< The process was stopped */

/*!
 * Marks the beginning of a region in a syscall function where the syscall
 * may be interrupted by a signal and return @ref EINTR.
 */

#define SLOW_SYSCALL_BEGIN      (THIS_THREAD->slow_syscall = 1)

/*!
 * Marks the end of a region in a syscall function where the syscall
 * may be interrupted by a signal and return @ref EINTR.
 */

#define SLOW_SYSCALL_END        (THIS_THREAD->slow_syscall = 0)

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
 * Stores information about the program data segment.
 */

struct brk
{
  void *base;                   /*!< Original end of program data segment */
  void *curr;                   /*!< Current end of program data segment */
  size_t max;                   /*!< Maximum size of program data segment */
};

/*!
 * Contains information about a child process.
 */

struct child_info
{
  pid_t pid;                    /*!< Process ID of child process */
};

/*!
 * Contains a list of the IDs of all processes that are children of a process.
 */

struct child_table
{
  size_t len;                   /*!< Number of children */
  struct child_info *info;      /*!< Array of child process info structures */
};

/*!
 * Contains information about a reaped process.
 */

struct wait_state
{
  pid_t pid;                    /*!< Process ID of child process */
  pid_t pgid;                   /*!< Process group of child process */
  struct rusage rusage;         /*!< Resource usage information */
  int status;                   /*!< Process execution status */
  int code;                     /*!< Exit code or signal number */
};

/*!
 * Contains a list of states of child processes that have terminated.
 */

struct wait_queue
{
  size_t len;                   /*!< Number of processes */
  struct wait_state *states;    /*!< Array of wait states */
};

/*!
 * Stores info about an allocated region in the user-space half of an
 * address space.
 */

struct mmap
{
  uintptr_t base;               /*!< Virtual address of memory region base */
  size_t len;                   /*!< Length of memory region */
  int prot;                     /*!< Memory protection of region */
  struct fd *file;              /*!< Vnode of mapped file */
  int fd;                       /*!< File descriptor number of mapped file */
  off_t offset;                 /*!< File offset corresponding to start */
  int flags;                    /*!< Mapping flags */
};

/*!
 * Represents a table of memory regions allocated to a process.
 */

struct mmap_table
{
  struct mmap *table;           /*!< Array of memory region structures */
  size_t len;                   /*!< Number of memory regions */
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
  uid_t suid;                   /*!< Saved user ID */
  gid_t gid;                    /*!< Real group ID */
  gid_t egid;                   /*!< Effective group ID */
  gid_t sgid;                   /*!< Saved group ID */
  gid_t sup_gids[NGROUPS_MAX];  /*!< Supplementary group IDs */
  size_t nsup_gids;             /*!< Number of supplementary group IDs */
  mode_t umask;                 /*!< File creation mode mask */
  struct vnode *cwd;            /*!< Current working directory */
  struct thread_queue threads;  /*!< Process thread queue */
  int priority;                 /*!< Process priority */
  struct fd_table fds;          /*!< File descriptor table */
  struct mmap_table mmaps;      /*!< Memory regions allocated to process */
  struct brk brk;               /*!< Program break */
  struct child_table children;  /*!< Child process list */
  struct wait_queue waits;      /*!< Queue of reaped processes */
  struct rusage self_rusage;    /*!< Resource usage of process */
  struct rusage child_rusage;   /*!< Resource usage of terminated children */
  struct sigaction sighandlers[NSIG];   /*!< Signal handler array */
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
void map_pid_process (pid_t pid, struct process *process);
void unmap_pid (pid_t pid);
struct process *lookup_pid (pid_t pid);

int alloc_procfd (void);
int alloc_fd (void);
void free_fd (int fd);
void free_procfd (int fd);
void free_altprocfd (struct process *process, int fd);
void fill_fd (int fd, int sysfd, struct vnode *vp, int flags);
struct fd *file_fd (int fd);

struct process *process_alloc (int priority);
void process_free (struct process *process);
void process_exit (unsigned int index, int status);
int process_enqueue (struct process *process);
struct process *process_fork (struct thread **t, int copy);
pid_t process_get_pid (struct process *process);
void process_fill_wait (struct process *process, int mode, int status);
void process_kill (int mode, int status) __noreturn;

int sigemptyset (sigset_t *set);
int sigfillset (sigset_t *set);
int sigaddset (sigset_t *set, int sig);
int sigdelset (sigset_t *set, int sig);
int sigismember (const sigset_t *set, int sig);
void signal_trampoline (void) __noreturn;
void handle_signal (int sig);
int poll_signal (void);
void *signal_handler (void);
void *poll_signal_handler (int *sig, sigset_t *mask);
int slow_syscall (void);
void update_signal_mask (sigset_t mask);
void send_signal_thread (struct thread *thread, int sig, const siginfo_t *info);
void send_signal (struct process *process, int sig, const siginfo_t *info);

pid_t __fork (int copy);

__END_DECLS

#endif
