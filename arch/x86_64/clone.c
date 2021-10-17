/* clone.c -- This file is part of PML.
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

#include <pml/process.h>
#include <errno.h>
#include "syscall.h"

pid_t
sys_clone (int (*func) (void *), void *arg)
{
  RETV_ERROR (ENOSYS, -1);
}

pid_t
sys_getpid (void)
{
  return THIS_PROCESS->pid;
}

pid_t
sys_getppid (void)
{
  return THIS_PROCESS->ppid;
}
