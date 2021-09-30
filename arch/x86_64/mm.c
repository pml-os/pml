/* mm.c -- This file is part of PML.
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

#include <pml/memory.h>
#include <stdlib.h>
#include <string.h>

extern void *boot_stack;

static uintptr_t kernel_stack_pdt[PAGE_STRUCT_ENTRIES] __page_align;
static uintptr_t kernel_stack_pt[PAGE_STRUCT_ENTRIES] __page_align;

uintptr_t kernel_pml4t[PAGE_STRUCT_ENTRIES];
uintptr_t kernel_thread_local_pdpt[PAGE_STRUCT_ENTRIES];
uintptr_t phys_map_pdpt[PAGE_STRUCT_ENTRIES * 4];

struct page_stack phys_page_stack;
uintptr_t next_phys_addr;
uintptr_t total_phys_mem;

/* Returns the physical address of the virtual address, or zero if the
   virtual address is not mapped to a physical address.
   @pml4t: the PML4T to use to lookup virtual address translations
   @addr: the virtual address to lookup */

uintptr_t
vm_phys_addr (uintptr_t *pml4t, void *addr)
{
  uintptr_t v = (uintptr_t) addr;
  unsigned int pml4e;
  unsigned int pdpe;
  unsigned int pde;
  unsigned int pte;
  uintptr_t *pdpt;
  uintptr_t *pdt;
  uintptr_t *pt;

  pml4e = (v >> 39) & 0x1ff;
  if (v >> 48 != !!(pml4e & 0x100) * 0xffff) /* Check sign extension */
    return 0;
  if (!(pml4t[pml4e] & PAGE_FLAG_PRESENT))
    return 0;

  pdpt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
  pdpe = (v >> 30) & 0x1ff;
  if (!(pdpt[pdpe] & PAGE_FLAG_PRESENT))
    return 0;
  if (pdpt[pdpe] & PAGE_FLAG_SIZE)
    return ALIGN_DOWN (pdpt[pdpe], HUGE_PAGE_SIZE) | (v & (HUGE_PAGE_SIZE - 1));

  pdt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
  pde = (v >> 21) & 0x1ff;
  if (!(pdt[pde] & PAGE_FLAG_PRESENT))
    return 0;
  if (pdt[pde] & PAGE_FLAG_SIZE)
    return ALIGN_DOWN (pdt[pde], LARGE_PAGE_SIZE) | (v & (LARGE_PAGE_SIZE - 1));

  pt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
  pte = (v >> 12) & 0x1ff;
  if (!(pt[pte] & PAGE_FLAG_PRESENT))
    return 0;
  return ALIGN_DOWN (pt[pte], PAGE_SIZE) | (v & (PAGE_SIZE - 1));
}

/* Allocates a page frame and returns its physical address. */

uintptr_t
alloc_page (void)
{
  uintptr_t addr;
  if (phys_page_stack.ptr > phys_page_stack.base)
    {
      addr = *--phys_page_stack.ptr;
      memset ((void *) PHYS_REL (addr), 0, PAGE_SIZE);
    }
  else
    {
      addr = next_phys_addr - LOW_PHYSICAL_BASE_VMA;
      next_phys_addr += PAGE_SIZE;
    }
  return addr;
}

/* Frees the page frame containing the given physical address. The address
   does not need to be page-aligned.
   @addr: the physical address to free */

void
free_page (uintptr_t addr)
{
  if (!addr)
    return;
  addr = ALIGN_DOWN (addr, PAGE_SIZE);
  *phys_page_stack.ptr++ = addr;
}

/* Frees all physical memory contained in a page table. The page table
   itself is not freed.
   @pt: the page table to free */

void
free_pt (uintptr_t *pt)
{
  size_t i;
  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
    {
      if (pt[i] & PAGE_FLAG_PRESENT)
	free_page (pt[i]);
      pt[i] = 0;
    }
}

/* Frees all physical memory contained in a page directory table. The
   page directory table itself is not freed, but any page tables it contains
   are freed.
   @pdt: the page directory table to free */

void
free_pdt (uintptr_t *pdt)
{
  size_t i;
  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
    {
      if ((pdt[i] & PAGE_FLAG_PRESENT) && !(pdt[i] & PAGE_FLAG_SIZE))
	{
	  uintptr_t pt_phys = ALIGN_DOWN (pdt[i], PAGE_SIZE);
	  uintptr_t *pt = (uintptr_t *) PHYS_REL (pt_phys);
	  free_pt (pt);
	  free_page (pt_phys);
	}
      pdt[i] = 0;
    }
}

/* Frees all physical memory contained in a page directory pointer table (PDPT).
   The structure itself is not freed, but any page tables or directories it
   contains are freed.
   @pdpt: the PDPT to free */

void
free_pdpt (uintptr_t *pdpt)
{
  size_t i;
  for (i = 0; i < PAGE_STRUCT_ENTRIES; i++)
    {
      if ((pdpt[i] & PAGE_FLAG_PRESENT) && !(pdpt[i] & PAGE_FLAG_SIZE))
	{
	  uintptr_t pdt_phys = ALIGN_DOWN (pdpt[i], PAGE_SIZE);
	  uintptr_t *pdt = (uintptr_t *) PHYS_REL (pdt_phys);
	  free_pdt (pdt);
	  free_page (pdt_phys);
	}
      pdpt[i] = 0;
    }
}

/* Initializes the kernel virtual address space. See <pml/x86_64/memory.h>
   for a description of the layout of the virtual address space on x86_64. */

void
vm_init (void)
{
  uintptr_t addr;
  size_t i;

  /* Map physical memory region */
  for (i = 0; i < 4; i++)
    kernel_pml4t[i + 508] =
      ((uintptr_t) (phys_map_pdpt + i * PAGE_STRUCT_ENTRIES) - KERNEL_VMA)
      | PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
  for (addr = 0, i = 0; i < PAGE_STRUCT_ENTRIES * 4;
       addr += HUGE_PAGE_SIZE, i++)
    phys_map_pdpt[i] = addr | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_SIZE;

  /* Map kernel stack to correct address space */
  kernel_pml4t[507] = ((uintptr_t) kernel_thread_local_pdpt - KERNEL_VMA)
    | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  kernel_thread_local_pdpt[511] = ((uintptr_t) kernel_stack_pdt - KERNEL_VMA)
    | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  kernel_stack_pdt[511] = ((uintptr_t) kernel_stack_pt - KERNEL_VMA)
    | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;
  for (i = 0; i < 4; i++)
    kernel_stack_pt[i + 508] =
      ((uintptr_t) &boot_stack + i * PAGE_SIZE - KERNEL_VMA)
      | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER;

  /* Apply the new page structures */
  vm_set_cr3 ((uintptr_t) kernel_pml4t - KERNEL_VMA);

  next_phys_addr = ALIGN_UP (KERNEL_END, PAGE_SIZE);
  phys_page_stack.base = (uintptr_t *) next_phys_addr;
  phys_page_stack.ptr = phys_page_stack.base;
  next_phys_addr += ALIGN_UP (total_phys_mem / 512, PAGE_SIZE);
}
