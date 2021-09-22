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
#define DEFAULT_STACK_VMA       0xfffffcfffffffff0

#define PRIO_MIN                19
#define PRIO_MAX                -20

#ifndef __ASSEMBLER__

#include <pml/lock.h>
#include <pml/types.h>

struct thread
{
  pid_t pid;
  pid_t ppid;
  pid_t tid;
  uint64_t *pml4t;
  void *stack;
  int priority;
};

__BEGIN_DECLS

extern lock_t thread_switch_lock;
extern struct thread *current_thread;

void sched_init (void);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
