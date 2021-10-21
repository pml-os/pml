/* exit.c -- This file is part of PML.
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

#include <pml/process.h>
#include <pml/syscall.h>

/*!
 * Index into the process queue of a process that recently terminated. The
 * scheduler will free a process listed in this variable on the next tick.
 */

unsigned int exit_process;

/*!
 * Status of a recently-terminated process. Processes terminated with a signal
 * will have a status equal to the signal number plus 128.
 */

int exit_status;

void
sys_exit (int status)
{
  thread_switch_lock = 1;
  exit_process = process_queue.front;
  exit_status = status;
  thread_switch_lock = 0;
  sched_yield ();
  __builtin_unreachable ();
}
