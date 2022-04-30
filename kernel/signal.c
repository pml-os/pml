/* signal.c -- This file is part of PML.
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

#include <pml/memory.h>
#include <pml/syscall.h>
#include <errno.h>
#include <string.h>

static int
sigemptyset (sigset_t *set)
{
  memset (set, 0, sizeof (sigset_t));
  return 0;
}

static int
sigfillset (sigset_t *set)
{
  memset (set, 0xff, sizeof (sigset_t));
  return 0;
}

static int
sigaddset (sigset_t *set, int sig)
{
  *set |= 1UL << (NSIG - sig);
  return 0;
}

static int
sigdelset (sigset_t *set, int sig)
{
  *set &= ~(1UL << (NSIG - sig));
  return 0;
}

static int
sigismember (const sigset_t *set, int sig)
{
  return !!(*set & (1UL << (NSIG - sig)));
}

static clock_t
convert_time (struct timeval *t)
{
  return t->tv_sec * 1000000 + t->tv_usec;
}

void
handle_signal (int sig)
{
  struct sigaction *handler = THIS_PROCESS->sighandlers + sig - 1;
  int exit = sig == SIGKILL;
  int stop = sig == SIGSTOP;
  if (!exit && !(handler->sa_flags & SA_SIGINFO)
      && handler->sa_handler == SIG_IGN)
    return; /* Ignore signal */

  switch (sig)
    {
    case SIGABRT:
    case SIGALRM:
    case SIGBUS:
    case SIGFPE:
    case SIGHUP:
    case SIGILL:
    case SIGINT:
    case SIGIO:
    case SIGPIPE:
    case SIGPROF:
    case SIGPWR:
    case SIGQUIT:
    case SIGSEGV:
    case SIGSTKFLT:
    case SIGSYS:
    case SIGTERM:
    case SIGTRAP:
    case SIGUSR1:
    case SIGUSR2:
    case SIGVTALRM:
    case SIGXCPU:
    case SIGXFSZ:
      if (!(handler->sa_flags & SA_SIGINFO) && handler->sa_handler == SIG_DFL)
	exit = 1; /* Default action is to terminate process */
      break;
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
      if (!(handler->sa_flags & SA_SIGINFO) && handler->sa_handler == SIG_DFL)
	stop = 1; /* Default action is to stop process */
      break;
    }

  if (exit)
    {
      pid_t ppid = THIS_PROCESS->ppid;
      struct process *pproc;
      siginfo_t cinfo;
      if (!ppid)
	goto kill;
      pproc = lookup_pid (ppid);
      if (UNLIKELY (!pproc))
	goto kill;

      cinfo.si_signo = SIGCHLD;
      cinfo.si_code = CLD_KILLED;
      cinfo.si_errno = 0;
      cinfo.si_pid = THIS_PROCESS->pid;
      cinfo.si_uid = THIS_PROCESS->uid;
      cinfo.si_status = sig;
      cinfo.si_utime = convert_time (&THIS_PROCESS->self_rusage.ru_utime);
      cinfo.si_stime = convert_time (&THIS_PROCESS->self_rusage.ru_stime);
      send_signal (pproc, SIGCHLD, &cinfo);

    kill:
      process_kill (PROCESS_WAIT_SIGNALED, sig);
    }
  else if (stop)
    {
      /* TODO Implement stop */
      return;
    }

  THIS_THREAD->handler = handler->sa_flags & SA_SIGINFO ?
    (void *) handler->sa_sigaction : (void *) handler->sa_handler;
}

/*!
 * Determines a signal ready to be handled on the current thread, if any.
 *
 * @return a signal number, or zero if no signal is ready to be handled
 */

int
poll_signal (void)
{
  siginfo_t *info = (siginfo_t *) SIGINFO_VMA;
  int i;
  if (!THIS_THREAD->sig)
    return 0;
  for (i = 1; i <= NSIG; i++)
    {
      if (sigismember (&THIS_THREAD->sigready, i)
	  && !sigismember (&THIS_THREAD->sigblocked, i))
	{
	  if (i < SIGRTMIN)
	    {
	      sigdelset (&THIS_THREAD->sigready, i);
	      memcpy (info, THIS_THREAD->siginfo + i - 1, sizeof (siginfo_t));
	    }
	  else
	    {
	      struct rtsig_queue *queue = THIS_THREAD->rtqueue + i - SIGRTMIN;
	      if (!--queue->len)
		sigdelset (&THIS_THREAD->sigready, i);
	      memcpy (info, queue->queue, sizeof (siginfo_t));
	      memmove (queue->queue, queue->queue + 1,
		       sizeof (siginfo_t) * queue->len);
	    }
	  THIS_THREAD->sig--;
	  return i;
	}
    }
  return 0;
}

/*!
 * Fetches the function pointer of a loaded signal handler. This function
 * is meant to be called from assembly code.
 *
 * @return the address of a loaded signal handler
 */

void *
signal_handler (void)
{
  return THIS_THREAD->handler;
}

/*!
 * Fetches and resets the signal handler for the current thread. 
 * This function is called once the handler is already loaded and ready to 
 * be executed so future interrupts do not reload the signal handler.
 *
 * @return the address of the loaded signal handler
 */

void *
poll_signal_handler (void)
{
  void *addr = THIS_THREAD->handler;
  THIS_THREAD->handler = NULL;
  return addr;
}

/*!
 * Sends a signal to a thread.
 *
 * @param thread the thread to signal
 * @param sig the signal number
 */

void
send_signal_thread (struct thread *thread, int sig, const siginfo_t *info)
{
  /* If a non-real-time signal was already queued, don't queue it again */
  if (sigismember (&thread->sigready, sig) && sig < SIGRTMIN)
    return;

  /* If the signal is blocked, add it to the pending signal mask */
  if (sigismember (&thread->sigblocked, sig))
    sigaddset (&thread->sigpending, sig);

  /* Mark the signal as delivered and copy the signal info structure */
  sigaddset (&thread->sigready, sig);
  if (sig < SIGRTMIN)
    memcpy (thread->siginfo + sig - 1, info, sizeof (siginfo_t));
  else
    {
      struct rtsig_queue *queue = thread->rtqueue + sig - SIGRTMIN;
      siginfo_t *buffer =
	realloc (queue->queue, sizeof (siginfo_t) * ++queue->len);
      if (UNLIKELY (!buffer))
	return; /* Not enough memory to queue signal, give up */
      queue->queue = buffer;
      memcpy (queue->queue + queue->len - 1, info, sizeof (siginfo_t));
    }
  thread->sig++;
}

/*!
 * Sends a signal to a thread in a process that is ready to receive the signal.
 *
 * @param process the process to signal
 * @param sig the signal number
 */

void
send_signal (struct process *process, int sig, const siginfo_t *info)
{
  /* Send the signal to a thread without the signal blocked */
  size_t i;
  for (i = 0; i < process->threads.len; i++)
    {
      struct thread *thread = process->threads.queue[i];
      if (thread->state == THREAD_STATE_RUNNING
	  && !sigismember (&thread->sigpending, sig)
	  && sigismember (&thread->sigblocked, sig))
	{
	  send_signal_thread (thread, sig, info);
	  return;
	}
    }

  /* Send the signal to a thread without the signal pending */
  for (i = 0; i < process->threads.len; i++)
    {
      struct thread *thread = process->threads.queue[i];
      if (thread->state == THREAD_STATE_RUNNING
	  && !sigismember (&thread->sigpending, sig))
	{
	  send_signal_thread (thread, sig, info);
	  return;
	}
    }

  /* Send the signal to any running thread */
  for (i = 0; i < process->threads.len; i++)
    {
      struct thread *thread = process->threads.queue[i];
      if (thread->state == THREAD_STATE_RUNNING)
	{
	  send_signal_thread (thread, sig, info);
	  return;
	}
    }

  /* Send the signal to the first thread as a last resort */
  send_signal_thread (process->threads.queue[0], sig, info);
}

sighandler_t
sys_signal (int sig, sighandler_t handler)
{
  struct sigaction old;
  struct sigaction act;
  if (sig <= 0 || sig > NSIG || sig == SIGKILL || sig == SIGSTOP)
    RETV_ERROR (EINVAL, SIG_ERR);
  act.sa_handler = handler;
  act.sa_sigaction = NULL;
  act.sa_flags = 0;
  memset (&act.sa_mask, 0, sizeof (sigset_t));
  if (sys_sigaction (sig, &act, &old) == -1)
    return SIG_ERR;
  return old.sa_handler;
}

int
sys_sigaction (int sig, const struct sigaction *act, struct sigaction *old_act)
{
  struct sigaction *handler;
  if (sig <= 0 || sig > NSIG || sig == SIGKILL || sig == SIGSTOP)
    RETV_ERROR (EINVAL, -1);
  handler = THIS_PROCESS->sighandlers + sig - 1;
  if (old_act)
    memcpy (old_act, handler, sizeof (struct sigaction));
  if (act)
    memcpy (handler, act, sizeof (struct sigaction));
  return 0;
}

int
sys_kill (pid_t pid, int sig)
{
  struct process *process;
  siginfo_t info;
  if (sig <= 0 || sig > NSIG)
    RETV_ERROR (EINVAL, -1);
  if (pid == -1)
    RETV_ERROR (ENOSYS, -1); /* TODO Send signal to all non-system processes */
  else if (pid <= 0)
    {
      /* Send signal to all processes with the same process group number */
      size_t i;
      for (i = 1; i < process_queue.len; i++)
	{
	  if (process_queue.queue[i]->pgid == -pid)
	    {
	      int ret = sys_kill (process_queue.queue[i]->pid, sig);
	      if (ret == -1)
		return ret;
	    }
	}
    }

  process = lookup_pid (pid);
  if (!process)
    RETV_ERROR (ESRCH, -1);
  if (THIS_PROCESS->euid && THIS_PROCESS->euid != process->euid)
    RETV_ERROR (EPERM, -1);

  info.si_signo = sig;
  info.si_code = SI_USER;
  info.si_errno = 0;
  info.si_pid = THIS_PROCESS->pid;
  info.si_uid = THIS_PROCESS->uid;
  send_signal (process, sig, &info);
  return 0;
}
