/* page-fault.c -- This file is part of PML.
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

#include <pml/interrupt.h>
#include <pml/panic.h>
#include <pml/process.h>

/*!
 * Handles a page fault. This function will perform necessary copying-on-writes
 * and deliver a fatal kernel panic if the exception cannot be handled.
 *
 * @todo implement signal throwing
 * @param err the error code pushed by the page fault exception
 */

void
int_page_fault (unsigned long err)
{
  uintptr_t addr;
  __asm__ volatile ("mov %%cr2, %0" : "=r" (addr));

  /* Assume page faults on the kernel thread are fatal */
  if (!THIS_PROCESS->pid)
    goto fatal;

 fatal:
  panic ("CPU exception: page fault\nVirtual address: %p\n", addr);
}
