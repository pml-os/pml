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

/** @file */

#include <pml/interrupt.h>
#include <pml/panic.h>

/*!
 * Handles a page fault. This function will perform necessary copying-on-writes
 * and deliver a fatal kernel panic if the exception cannot be handled.
 *
 * @param err the error code pushed by the page fault exception
 */

void
int_page_fault (unsigned long err)
{
  void *addr;
  __asm__ volatile ("mov %%cr2, %0" : "=r" (addr));
  panic ("CPU exception: page fault\n"
	 "Virtual address: %p\n", addr);
}
