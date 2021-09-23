/* lock.h -- This file is part of PML.
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

#ifndef __PML_LOCK_H
#define __PML_LOCK_H

#include <pml/cdefs.h>

typedef volatile int lock_t;

struct thread_list;

struct semaphore
{
  lock_t lock;                  /* The actual locked object */
  struct thread_list *blocked;  /* Threads blocked for acquiring mutex */
};

__BEGIN_DECLS

void spinlock_acquire (lock_t *l);
void spinlock_release (lock_t *l);

void semaphore_signal (struct semaphore *sem);
void semaphore_wait (struct semaphore *sem);

__END_DECLS

#endif
