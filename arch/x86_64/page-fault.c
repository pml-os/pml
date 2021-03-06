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

static char *present_msg[] = {"non-present page", "protection violation"};
static char *write_msg[] = {"read access", "write access"};
static char *user_msg[] = {"supervisor mode", "user mode"};
static char *reserved_msg[] = {"", ", reserved write"};
static char *inst_msg[] = {"", ", instruction fetch"};

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
  uintptr_t cr3;
  unsigned int pml4e;
  unsigned int pdpe;
  unsigned int pde;
  unsigned int pte;
  uintptr_t *pml4t;
  uintptr_t *pdpt;
  uintptr_t *pdt;
  uintptr_t *pt;
  siginfo_t info;
  size_t i;
  __asm__ volatile ("mov %%cr2, %0" : "=r" (addr));
  __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));

  /* Assume page faults on the kernel thread are fatal */
  if (!THIS_PROCESS->pid)
    goto fatal;

  /* Check for copy-on-write */
  if (err & PAGE_ERR_USER)
    {
      thread_switch_lock = 1;
      pml4t = THIS_THREAD->args.pml4t;
      pml4e = PML4T_INDEX (addr);
      if (addr >> 48 != !!(pml4e & 0x100) * 0xffff) /* Check sign extension */
	goto signal;
      if (!(pml4t[pml4e] & PAGE_FLAG_PRESENT))
	goto signal;
      if (pml4t[pml4e] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  uintptr_t *np = (uintptr_t *) PHYS_REL (page);
	  uintptr_t *cp =
	    (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
	  if (UNLIKELY (!page))
	    goto signal;
	  memset (np, 0, PAGE_SIZE);
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
	goto signal;
      if (pdpt[pdpe] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  uintptr_t *np = (uintptr_t *) PHYS_REL (page);
	  uintptr_t *cp =
	    (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
	  if (UNLIKELY (!page))
	    goto signal;
	  memset (np, 0, PAGE_SIZE);
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
	goto signal;
      if (pdt[pde] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  uintptr_t *np = (uintptr_t *) PHYS_REL (page);
	  uintptr_t *cp =
	    (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
	  if (UNLIKELY (!page))
	    goto signal;
	  memset (np, 0, PAGE_SIZE);
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
	goto signal;
      if (pt[pte] & PAGE_FLAG_COW)
	{
	  uintptr_t page = alloc_page ();
	  if (UNLIKELY (!page))
	    goto signal;
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

 signal:
  info.si_signo = SIGSEGV;
  info.si_code = err & PAGE_ERR_PRESENT ? SEGV_ACCERR : SEGV_MAPERR;
  info.si_errno = 0;
  info.si_addr = (void *) addr;
  send_signal_thread (THIS_THREAD, SIGSEGV, &info);
  return;

 fatal:
  panic ("CPU exception: page fault\nVirtual address: %p\nInstruction: %p\n"
	 "Attributes: %s, %s, %s%s%s\nCR3: %p\nPID: %d\nTID: %d\n",
	 addr, inst_addr, present_msg[!!(err & PAGE_ERR_PRESENT)],
	 write_msg[!!(err & PAGE_ERR_WRITE)], user_msg[!!(err & PAGE_ERR_USER)],
	 reserved_msg[!!(err & PAGE_ERR_RESERVED)],
	 inst_msg[!!(err & PAGE_ERR_INST)], cr3, THIS_PROCESS->pid,
	 THIS_THREAD->tid);
}
