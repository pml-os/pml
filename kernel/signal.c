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

/*!
 * Sends a signal to a thread.
 *
 * @param thread the thread to signal
 * @param sig the signal number
 * @return zero on success
 */

int
send_signal_thread (struct thread *thread, int sig, const siginfo_t *info)
{
  /* Don't queue another of the same signal if one is already pending */
  if (sigismember (&thread->sigpending, sig))
    return 0;

  /* Add the signal to the pending signal mask */
  sigaddset (&thread->sigpending, sig);
  memcpy (thread->siginfo + sig, info, sizeof (siginfo_t));
  return 0;
}

/*!
 * Sends a signal to a thread in a process that is ready to receive the signal.
 *
 * @param process the process to signal
 * @param sig the signal number
 * @return zero on success
 */

int
send_signal (struct process *process, int sig, const siginfo_t *info)
{
  /* Send the signal to a thread without the signal blocked */
  size_t i;
  for (i = 0; process->threads.len; i++)
    {
      struct thread *thread = process->threads.queue[i];
      if (thread->state == THREAD_STATE_RUNNING
	  && sigismember (&thread->sigblocked, sig))
	return send_signal_thread (thread, sig, info);
    }

  /* Send the signal to any running thread */
  for (i = 0; process->threads.len; i++)
    {
      struct thread *thread = process->threads.queue[i];
      if (thread->state == THREAD_STATE_RUNNING)
	return send_signal_thread (thread, sig, info);
    }

  /* Send the signal to the first thread as a last resort */
  return send_signal_thread (process->threads.queue[0], sig, info);
}

sighandler_t
sys_signal (int sig, sighandler_t handler)
{
  struct sigaction old;
  struct sigaction act;
  if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
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
  if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
    RETV_ERROR (EINVAL, -1);
  handler = THIS_PROCESS->sighandlers + sig;
  if (old_act)
    memcpy (old_act, handler, sizeof (struct sigaction));
  if (act)
    memcpy (handler, act, sizeof (struct sigaction));
  return 0;
}
