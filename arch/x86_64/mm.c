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

/*! @file */

#include <pml/alloc.h>
#include <pml/memory.h>
#include <pml/process.h>
#include <errno.h>
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

/*! 
 * Returns the physical address of the virtual address, or zero if the
 * virtual address is not mapped to a physical address.
 *
 * @param addr the virtual address to lookup
 * @return the physical address of the virtual address
 */

uintptr_t
physical_addr (void *addr)
{
  return vm_phys_addr (THIS_THREAD->args.pml4t, addr);
}

/*! 
 * Returns the physical address of the virtual address, or zero if the
 * virtual address is not mapped to a physical address.
 *
 * @param pml4t the PML4T to use to lookup virtual address translations
 * @param addr the virtual address to lookup
 * @return the physical address of the virtual address
 */

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

  pml4e = PML4T_INDEX (v);
  if (v >> 48 != !!(pml4e & 0x100) * 0xffff) /* Check sign extension */
    return 0;
  if (!(pml4t[pml4e] & PAGE_FLAG_PRESENT))
    return 0;

  pdpt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
  pdpe = PDPT_INDEX (v);
  if (!(pdpt[pdpe] & PAGE_FLAG_PRESENT))
    return 0;
  if (pdpt[pdpe] & PAGE_FLAG_SIZE)
    return ALIGN_DOWN (pdpt[pdpe], HUGE_PAGE_SIZE) | (v & (HUGE_PAGE_SIZE - 1));

  pdt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
  pde = PDT_INDEX (v);
  if (!(pdt[pde] & PAGE_FLAG_PRESENT))
    return 0;
  if (pdt[pde] & PAGE_FLAG_SIZE)
    return ALIGN_DOWN (pdt[pde], LARGE_PAGE_SIZE) | (v & (LARGE_PAGE_SIZE - 1));

  pt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
  pte = PT_INDEX (v);
  if (!(pt[pte] & PAGE_FLAG_PRESENT))
    return 0;
  return ALIGN_DOWN (pt[pte], PAGE_SIZE) | (v & (PAGE_SIZE - 1));
}

/*!
 * Maps the page at the virtual address to a physical address. The
 * virtual address must not be in a large page, and the physical address
 * does not need to be page-aligned.
 *
 * @param pml4t the address space to perform the mapping
 * @param phys_addr the physical address to be mapped
 * @param addr the virtual address to map the physical address to
 * @param flags extra page flags
 * @return zero on success
 */

int
vm_map_page (uintptr_t *pml4t, uintptr_t phys_addr, void *addr,
	     unsigned int flags)
{
  uintptr_t v = (uintptr_t) addr;
  unsigned int pml4e;
  unsigned int pdpe;
  unsigned int pde;
  unsigned int pte;
  uintptr_t *pdpt;
  uintptr_t *pdt;
  uintptr_t *pt;

  pml4e = PML4T_INDEX (v);
  if (v >> 48 != !!(pml4e & 0x100) * 0xffff) /* Check sign extension */
    RETV_ERROR (EFAULT, -1);
  if (!(pml4t[pml4e] & PAGE_FLAG_PRESENT))
    {
      pml4t[pml4e] = alloc_page ();
      if (UNLIKELY (!pml4t[pml4e]))
	return -1;
      pml4t[pml4e] |= PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER | flags;
    }

  pdpt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
  pdpe = PDPT_INDEX (v);
  if (!(pdpt[pdpe] & PAGE_FLAG_PRESENT))
    {
      pdpt[pdpe] = alloc_page ();
      if (UNLIKELY (!pdpt[pdpe]))
	return -1;
      pdpt[pdpe] |= PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER | flags;
    }
  if (pdpt[pdpe] & PAGE_FLAG_SIZE)
    RETV_ERROR (EINVAL, -1);

  pdt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
  pde = PDT_INDEX (v);
  if (!(pdt[pde] & PAGE_FLAG_PRESENT))
    {
      pdt[pde] = alloc_page ();
      if (UNLIKELY (!pdt[pde]))
	return -1;
      pdt[pde] |= PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_USER | flags;
    }
  if (pdt[pde] & PAGE_FLAG_SIZE)
    RETV_ERROR (EINVAL, -1);

  pt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
  pte = PT_INDEX (v);
  pt[pte] = ALIGN_DOWN (phys_addr, PAGE_SIZE) | PAGE_FLAG_PRESENT | flags;
  return 0;
}

/*!
 * Removes an existing virtual address mapping from an address space. The
 * virtual address given does not need to be page-aligned, and if it is
 * in a large page the entire large page will be unmapped.
 *
 * @param pml4t the address space to perform the unmapping
 * @param addr the virtual address of the page to unmap
 * @return zero on success or if no mapping existed
 */

int
vm_unmap_page (uintptr_t *pml4t, void *addr)
{
  uintptr_t v = (uintptr_t) addr;
  unsigned int pml4e;
  unsigned int pdpe;
  unsigned int pde;
  unsigned int pte;
  uintptr_t *pdpt;
  uintptr_t *pdt;
  uintptr_t *pt;

  pml4e = PML4T_INDEX (v);
  if (v >> 48 != !!(pml4e & 0x100) * 0xffff) /* Check sign extension */
    RETV_ERROR (EFAULT, -1);
  if (!(pml4t[pml4e] & PAGE_FLAG_PRESENT))
    return 0;

  pdpt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pml4t[pml4e], PAGE_SIZE));
  pdpe = PDPT_INDEX (v);
  if (!(pdpt[pdpe] & PAGE_FLAG_PRESENT))
    return 0;
  if (pdpt[pdpe] & PAGE_FLAG_SIZE)
    {
      pdpt[pdpe] = 0;
      return 0;
    }

  pdt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdpt[pdpe], PAGE_SIZE));
  pde = PDT_INDEX (v);
  if (!(pdt[pde] & PAGE_FLAG_PRESENT))
    return 0;
  if (pdt[pde] & PAGE_FLAG_SIZE)
    {
      pdt[pde] = 0;
      return 0;
    }

  pt = (uintptr_t *) PHYS_REL (ALIGN_DOWN (pdt[pde], PAGE_SIZE));
  pte = PT_INDEX (v);
  pt[pte] = 0;
  return 0;
}

/*!
 * Allocates a page frame and returns its physical address.
 *
 * @return the physical address of the new page frame, or 0 if the allocation
 * failed
 */

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
  vm_skip_holes ();
  return addr;
}

/*!
 * Frees the page frame containing the given physical address. The address
 * does not need to be page-aligned.
 *
 * @param addr the physical address to free
 */

void
free_page (uintptr_t addr)
{
  if (!addr)
    return;
  addr = ALIGN_DOWN (addr, PAGE_SIZE);
  *phys_page_stack.ptr++ = addr;
}

/*!
 * Allocates a page frame and returns a pointer to the data in the virtual
 * address space.
 *
 * @return a pointer to the page in virtual memory, or NULL if the allocation
 * failed
 */

void *
alloc_virtual_page (void)
{
  uintptr_t addr = alloc_page ();
  if (UNLIKELY (!addr))
    return NULL;
  return (void *) PHYS_REL (addr);
}

/*!
 * Frees a pointer allocated with alloc_virtual_page().
 *
 * @param ptr the pointer to free
 */

void
free_virtual_page (void *ptr)
{
  if (ptr)
    free_page ((uintptr_t) ptr - KERNEL_VMA);
}

/*!
 * Frees all physical memory contained in a page table. The page table
 * itself is not freed.
 *
 * @param pt the page table to free
 */

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

/*!
 * Frees all physical memory contained in a page directory table. The
 * page directory table itself is not freed, but any page tables it contains
 * are freed.
 *
 * @param pdt the page directory table to free
 */

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

/*!
 * Frees all physical memory contained in a page directory pointer table (PDPT).
 * The structure itself is not freed, but any page tables or directories it
 * contains are freed.
 *
 * @param pdpt the PDPT to free
 */

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

/*!
 * Initializes the kernel virtual address space.
 */

void
vm_init (void)
{
  uintptr_t addr;
  size_t i;

  /* Map physical memory region */
  for (i = 0; i < 4; i++)
    kernel_pml4t[i + 508] =
      ((uintptr_t) (phys_map_pdpt + i * PAGE_STRUCT_ENTRIES) - KERNEL_VMA)
      | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_GLOBAL;
  for (addr = 0, i = 0; i < PAGE_STRUCT_ENTRIES * 4;
       addr += HUGE_PAGE_SIZE, i++)
    phys_map_pdpt[i] = addr | PAGE_FLAG_PRESENT | PAGE_FLAG_RW
      | PAGE_FLAG_SIZE | PAGE_FLAG_GLOBAL;

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

int
sys_brk (void *addr)
{
  void *i;
  if (addr < THIS_PROCESS->brk.base)
    RETV_ERROR (ENOMEM, -1);
  else if (addr > THIS_PROCESS->brk.base + THIS_PROCESS->brk.max)
    RETV_ERROR (ENOMEM, -1);
  else if (addr < THIS_PROCESS->brk.curr)
    {
      for (i = ALIGN_UP (addr, PAGE_SIZE); i < THIS_PROCESS->brk.curr;
	   i += PAGE_SIZE)
	{
	  uintptr_t phys_addr = physical_addr (addr);
	  if (phys_addr)
	    free_page (phys_addr);
	  vm_unmap_page (THIS_THREAD->args.pml4t, addr);
	}
      THIS_PROCESS->brk.curr = ALIGN_UP (addr, PAGE_SIZE);
    }
  else
    {
      for (i = THIS_PROCESS->brk.curr; i < ALIGN_UP (addr, PAGE_SIZE);
	   i += PAGE_SIZE)
	{
	  uintptr_t phys_addr = alloc_page ();
	  if (!phys_addr)
	    RETV_ERROR (ENOMEM, -1);
	  if (vm_map_page (THIS_THREAD->args.pml4t, phys_addr, i,
			   PAGE_FLAG_RW | PAGE_FLAG_USER))
	    RETV_ERROR (ENOMEM, -1);
	}
      THIS_PROCESS->brk.curr = ALIGN_UP (addr, PAGE_SIZE);
    }
  return 0;
}

void *
sys_sbrk (ptrdiff_t incr)
{
  void *ptr = THIS_PROCESS->brk.curr;
  if (sys_brk (THIS_PROCESS->brk.curr + incr))
    return (void *) -1;
  return ptr;
}
