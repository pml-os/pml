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

#include <pml/alloc.h>
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/panic.h>
#include <pml/process.h>
#include <string.h>

/*!
 * Handles a page fault. This function will perform necessary copying-on-writes
 * and deliver a fatal kernel panic if the exception cannot be handled.
 *
 * @todo implement signal throwing
 * @param err the error code pushed by the page fault exception
 * @param inst_addr the address of the instruction that generated the page fault
 */

void
int_page_fault (unsigned long err, uintptr_t inst_addr)
{
  uintptr_t addr;
  unsigned int pml4e;
  unsigned int pdpe;
  unsigned int pde;
  unsigned int pte;
  uintptr_t *pml4t;
  uintptr_t *pdpt;
  uintptr_t *pdt;
  uintptr_t *pt;
  size_t i;
  __asm__ volatile ("mov %%cr2, %0" : "=r" (addr));

  /* Assume page faults on the kernel thread are fatal */
  if (!THIS_PROCESS->pid)
    goto fatal;

  /* Check if the error was caused by an attempted write to a read-only
     page that needs to be copied-on-write */
  if (err & 4)
    {
      thread_switch_lock = 1;
      pml4t = THIS_THREAD->args.pml4t;
      pml4e = PML4T_INDEX (addr);
      if (addr >> 48 != !!(pml4e & 0x100) * 0xffff) /* Check sign extension */
	goto fatal;
      if (!(pml4t[pml4e] & PAGE_FLAG_PRESENT))
	goto fatal;
      if (pml4t[pml4e] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  uintptr_t *np = (uintptr_t *) PHYS_REL (page);
	  uintptr_t *cp =
	    (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
	  if (UNLIKELY (!page))
	    goto fatal;
	  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
	    {
	      if (cp[i] & PAGE_FLAG_PRESENT)
		{
		  np[i] = cp[i] | PAGE_FLAG_COW;
		  np[i] &= ~PAGE_FLAG_RW;
		}
	    }
	  free_page (pml4t[pml4e]);
	  pml4t[pml4e] = page | PAGE_FLAG_RW | (pml4t[pml4e] & (PAGE_SIZE - 1));
	  pml4t[pml4e] &= ~PAGE_FLAG_COW;
	}

      pdpt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
      pdpe = PDPT_INDEX (addr);
      if ((pdpt[pdpe] & PAGE_FLAG_SIZE) || !(pdpt[pdpe] & PAGE_FLAG_PRESENT))
	goto fatal;
      if (pdpt[pdpe] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  uintptr_t *np = (uintptr_t *) PHYS_REL (page);
	  uintptr_t *cp =
	    (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
	  if (UNLIKELY (!page))
	    goto fatal;
	  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
	    {
	      if (cp[i] & PAGE_FLAG_PRESENT)
		{
		  np[i] = cp[i] | PAGE_FLAG_COW;
		  np[i] &= ~PAGE_FLAG_RW;
		}
	    }
	  free_page (pdpt[pdpe]);
	  pdpt[pdpe] = page | PAGE_FLAG_RW | (pdpt[pdpe] & (PAGE_SIZE - 1));
	  pdpt[pdpe] &= ~PAGE_FLAG_COW;
	}

      pdt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
      pde = PDT_INDEX (addr);
      if ((pdt[pde] & PAGE_FLAG_SIZE) || !(pdt[pde] & PAGE_FLAG_PRESENT))
	goto fatal;
      if (pdt[pde] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  uintptr_t *np = (uintptr_t *) PHYS_REL (page);
	  uintptr_t *cp =
	    (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
	  if (UNLIKELY (!page))
	    goto fatal;
	  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
	    {
	      if (cp[i] & PAGE_FLAG_PRESENT)
		{
		  np[i] = cp[i] | PAGE_FLAG_COW;
		  np[i] &= ~PAGE_FLAG_RW;
		}
	    }
	  free_page (pdt[pde]);
	  pdt[pde] = page | PAGE_FLAG_RW | (pdt[pde] & (PAGE_SIZE - 1));
	  pdt[pde] &= ~PAGE_FLAG_COW;
	}

      pt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
      pte = PT_INDEX (addr);
      if (!(pt[pte] & PAGE_FLAG_PRESENT))
	goto fatal;
      if (pt[pte] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  if (UNLIKELY (!page))
	    goto fatal;
	  memcpy ((void *) PHYS_REL (page),
		  (void *) PHYS_REL (ALIGN_DOWN (pt[pte], PAGE_SIZE)),
		  PAGE_SIZE);
	  free_page (pt[pte]);
	  pt[pte] = page | PAGE_FLAG_RW | (pt[pte] & (PAGE_SIZE - 1));
	  pt[pte] &= ~PAGE_FLAG_COW;
	  vm_clear_page ((void *) ALIGN_DOWN (addr, PAGE_SIZE));
	  thread_switch_lock = 0;
	  return;
	}
    }

 fatal:
  panic ("CPU exception: page fault\nVirtual address: %p\nInstruction: %p",
	 addr, inst_addr);
}
