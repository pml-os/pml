/* signal.h -- This file is part of PML.
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

#ifndef __PML_SIGNAL_H
#define __PML_SIGNAL_H

/*! 
 * @file
 * @brief POSIX signal definitions
 */

#include <pml/types.h>

#define SIGHUP                  1           /*!< Hangup */
#define SIGINT                  2           /*!< Interrupt */
#define SIGQUIT                 3           /*!< Quit */
#define SIGILL                  4           /*!< Illegal instruction */
#define SIGTRAP                 5           /*!< Trace/breakpoint trap */
#define SIGABRT                 6           /*!< Aborted */
#define SIGIOT                  SIGABRT     /*!< Input/output trap */
#define SIGBUS                  7           /*!< Bus error */
#define SIGFPE                  8           /*!< Floating point exception */
#define SIGKILL                 9           /*!< Killed */
#define SIGUSR1                 10          /*!< User defined signal 1 */
#define SIGSEGV                 11          /*!< Segmentation fault */
#define SIGUSR2                 12          /*!< User defined signal 2 */
#define SIGPIPE                 13          /*!< Broken pipe */
#define SIGALRM                 14          /*!< Alarm clock */
#define SIGTERM                 15          /*!< Terminated */
#define SIGSTKFLT               16          /*!< Stack fault */
#define SIGCHLD                 17          /*!< Child exited */
#define SIGCLD                  SIGCHLD     /*!< Synonym for @ref SIGCHLD */
#define SIGCONT                 18          /*!< Continued */
#define SIGSTOP                 19          /*!< Stopped (signal) */
#define SIGTSTP                 20          /*!< Stopped */
#define SIGTTIN                 21          /*!< Stopped (tty input) */
#define SIGTTOU                 22          /*!< Stopped (tty output) */
#define SIGURG                  23          /*!< Urgent I/O condition */
#define SIGXCPU                 24          /*!< CPU time limit exceeded */
#define SIGXFSZ                 25          /*!< File size limit exceeded */
#define SIGVTALRM               26          /*!< Virtual timer expired */
#define SIGPROF                 27          /*!< Profiling timer expired */
#define SIGWINCH                28          /*!< Window changed */
#define SIGIO                   29          /*!< I/O possible */
#define SIGPOLL                 SIGIO       /*!< Pollable event */
#define SIGPWR                  30          /*!< Power failure */
#define SIGSYS                  31          /*!< Bad system call */
#define SIGUNUSED               SIGSYS      /*!< Synonym for @ref SIGSYS */

#define NSIG                    64          /*!< Number of signals */

/*!
 * Signal number of the first real-time signal exposed by the kernel.
 * User-space threading libraries may internally use a few real-time signals,
 * so the real definition of SIGRTMIN is not guaranteed to be this value.
 */
#define __pml_SIGRTMIN          32
#ifdef PML_KERNEL
#define SIGRTMIN                __pml_SIGRTMIN
#endif

/* Signal handler flags */

#define SA_NOCLDSTOP            (1 << 0)
#define SA_NOCLDWAIT            (1 << 1)
#define SA_SIGINFO              (1 << 2)
#define SA_ONSTACK              (1 << 3)
#define SA_RESTART              (1 << 4)
#define SA_NODEFER              (1 << 5)
#define SA_RESETHAND            (1 << 6)
#define SA_NOMASK               SA_NODEFER
#define SA_ONESHOT              SA_RESETHAND

/* Signal codes */

#define SI_USER                 0x100
#define SI_KERNEL               0x101
#define SI_QUEUE                0x102
#define SI_TIMER                0x103
#define SI_ASYNCIO              0x104
#define SI_TKILL                0x105

/* Reason for signal generation */

#define ILL_ILLOPC              0x200
#define ILL_ILLOPN              0x201
#define ILL_ILLADDR             0x202
#define ILL_ILLTRP              0x203
#define ILL_PRVOPC              0x204
#define ILL_PRVREG              0x205
#define ILL_COPROC              0x206
#define ILL_BADSTK              0x207

#define FPE_INTDIV              0x300
#define FPE_INTOVF              0x301
#define FPE_FLTDIV              0x302
#define FPE_FLTOVF              0x303
#define FPE_FLTUND              0x304
#define FPE_FLTRES              0x305
#define FPE_FLTINV              0x306
#define FPE_FLTSUB              0x307

#define SEGV_MAPERR             0x400
#define SEGV_ACCERR             0x401

#define BUS_ADRALN              0x500
#define BUS_ADRERR              0x501
#define BUS_OBJERR              0x502

#define TRAP_BRKPT              0x600
#define TRAP_TRACE              0x601

#define CLD_EXITED              0x700
#define CLD_KILLED              0x701
#define CLD_DUMPED              0x702
#define CLD_TRAPPED             0x703
#define CLD_STOPPED             0x704
#define CLD_CONTINUED           0x705

#define POLL_IN                 0x800
#define POLL_OUT                0x801
#define POLL_MSG                0x802
#define POLL_ERR                0x803
#define POLL_PRI                0x804
#define POLL_HUP                0x805

/* sigprocmask(2) actions */

#define SIG_BLOCK               0
#define SIG_UNBLOCK             1
#define SIG_SETMASK             2

/* sigaltstack(2) flags */

#define SS_ONSTACK              (1 << 0)
#define SS_DISABLE              (1 << 1)

/* Signal handler special values */

#define SIG_DFL                 ((sighandler_t) 0)  /*!< Default action */
#define SIG_IGN                 ((sighandler_t) 1)  /*!< Ignore the signal */

/*! Returned by @ref sys_signal when an error occurs */
#define SIG_ERR                 ((sighandler_t) -1)

/*! Type of signal handler functions for signal(2) */
typedef void (*sighandler_t) (int);
/*! Alias of @ref sighandler_t */
typedef sighandler_t sig_t;
/*! Atomic type that can be accessed in a signal handler */
typedef int sig_atomic_t;
/*! Type with bits representing a mask of blocked signals */
typedef unsigned long sigset_t;

union sigval
{
  int sival_int;
  void *sival_ptr;
};

struct __siginfo_t
{
  int si_signo;
  int si_code;
  int si_errno;
  union
  {
    struct
    {
      pid_t si_pid;
      uid_t si_uid;
    } __kill;
    struct
    {
      int si_timerid;
      int si_overrun;
      union sigval si_value;
    } __timer;
    struct
    {
      pid_t si_pid;
      uid_t si_uid;
      union sigval si_value;
    } __rt;
    struct
    {
      pid_t si_pid;
      uid_t si_uid;
      int si_status;
      clock_t si_utime;
      clock_t si_stime;
    } __child;
    void *si_addr;
    struct
    {
      long si_band;
      int si_fd;
    } __poll;
  } __data;
};

typedef struct __siginfo_t siginfo_t;

#define si_pid                  __data.__kill.si_pid
#define si_uid                  __data.__kill.si_uid
#define si_timerid              __data.__timer.si_timerid
#define si_overrun              __data.__timer.si_overrun
#define si_status               __data.__child.si_status
#define si_utime                __data.__child.si_utime
#define si_stime                __data.__child.si_stime
#define si_value                __data.__rt.si_value
#define si_addr                 __data.si_addr
#define si_band                 __data.__poll.si_band
#define si_fd                   __data.__poll.si_fd

struct sigaction
{
  sighandler_t sa_handler;
  void (*sa_sigaction) (int, siginfo_t *, void *);
  unsigned long sa_flags;
  sigset_t sa_mask;
};

struct __stack_t
{
  void *ss_sp;
  int ss_flags;
  size_t ss_size;
};

typedef struct __stack_t stack_t;

#endif
