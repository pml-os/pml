/* lock.c -- This file is part of PML.
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

#include <pml/lock.h>
#include <pml/thread.h>

/*!
 * Acquires a spinlock. This function will block until the spinlock is free.
 *
 * @param l a pointer to the spinlock object
 */

void
spinlock_acquire (lock_t *l)
{
  while (__sync_lock_test_and_set (l, 1))
    {
      while (*l)
	;
    }
}

/*!
 * Releases a spinlock.
 *
 * @param l a pointer to the spinlock object
 */

void
spinlock_release (lock_t *l)
{
  __sync_synchronize ();
  *l = 0;
}

/*!
 * Allocates a semaphore.
 *
 * @param init_count initial count on the semaphore
 * @return the created semaphore object, or NULL if the allocation failed
 */

struct semaphore *
semaphore_create (lock_t init_count)
{
  struct semaphore *sem = malloc (sizeof (struct semaphore));
  if (UNLIKELY (!sem))
    return NULL;
  sem->lock = init_count;
  sem->blocked = NULL;
  return sem;
}

/*!
 * Frees a semaphore and unblocks all threads waiting for it.
 *
 * @param sem the semaphore to free
 */

void
semaphore_free (struct semaphore *sem)
{
  /* Unblock all threads blocked by the semaphore */
  struct thread_list *list = sem->blocked;
  while (list != NULL)
    {
      struct thread_list *next = list->next;
      list->thread->state = THREAD_STATE_RUNNING;
      free (list);
      list = next;
    }
  free (sem);
}

/*!
 * Signals a semaphore.
 *
 * @param sem the semaphore to signal
 */

void
semaphore_signal (struct semaphore *sem)
{
  __sync_synchronize ();
  __atomic_fetch_add (&sem->lock, 1, __ATOMIC_SEQ_CST);
}

/*!
 * Waits for a semaphore.
 *
 * @todo block the calling thread
 * @param sem the semaphore to wait
 */

void
semaphore_wait (struct semaphore *sem)
{
  while (!__atomic_load_n (&sem->lock, __ATOMIC_SEQ_CST))
    ;
  __atomic_fetch_sub (&sem->lock, 1, __ATOMIC_SEQ_CST);
}
