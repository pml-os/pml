/* memory.h -- This file is part of PML.
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

#ifndef __PML_MEMORY_H
#define __PML_MEMORY_H

/*!
 * @file memory.h
 * Virtual memory layout on x86_64
 * @code{.unparsed}
 * 0x0000000000000000-0x00007fffffffffff  128T  User space memory
 * 0xffff800000000000-0xfffffcffffffffff ~126T  Reserved kernel memory
 * 0xfffffd8000000000-0xfffffdffbfffffff  511G  Thread-local storage
 * 0xfffffdffc0000000-0xfffffdffffffffff    1G  Thread stack space
 * 0xfffffe0000000000-0xffffffffffffffff    2T  Physical memory mappings
 * @endcode
 * A maximum of 2 TiB of physical memory is supported. PML will not be able
 * to access physical memory beyond the 2 TiB address (@ref PHYS_ADDR_LIMIT).
 */

/*! Base virtual address of thread-local storage */
#define THREAD_LOCAL_BASE_VMA   0xfffffd8000000000
/*! Base virtual address of process stack */
#define PROCESS_STACK_BASE_VMA  0xfffffdffc0000000
/*! Starting virtual address of the stack */
#define PROCESS_STACK_TOP_VMA   0xfffffdfffffffff8
/*! Base of physical memory map */
#define LOW_PHYSICAL_BASE_VMA   0xfffffe0000000000

/*! First memory address in upper memory */
#define LOW_MEMORY_LIMIT        0x0000000000100000
/*! Maximum addressible physical address supported */
#define PHYS_ADDR_LIMIT         0x0000020000000000

/*! Required alignment of page structures */
#define PAGE_STRUCT_ALIGN       4096
/*! Size of page structures */
#define PAGE_STRUCT_SIZE        4096
/*! Number of 8-byte entries in page structures */
#define PAGE_STRUCT_ENTRIES     512

/*! Page present */
#define PAGE_FLAG_PRESENT       (1 << 0)
/*! Read and write access */
#define PAGE_FLAG_RW            (1 << 1)
/*! User-accessible page */
#define PAGE_FLAG_USER          (1 << 2)
/*! Write-through page caching */
#define PAGE_FLAG_WTHRU         (1 << 3)
/*! Prevent TLB from caching page */
#define PAGE_FLAG_NOCACHE       (1 << 4)
/*! Set when page is accessed */
#define PAGE_FLAG_ACCESS        (1 << 5)
/*! Set when page is written to */
#define PAGE_FLAG_DIRTY         (1 << 6)
/*! Use larger page size */
#define PAGE_FLAG_SIZE          (1 << 7)
/*! Prevent TLB from invalidating page */
#define PAGE_FLAG_GLOBAL        (1 << 8)

/*! Allocate page on access */
#define PAGE_NP_FLAG_AOA        (1 << 1)
/*! Fetch page from swap space */
#define PAGE_NP_FLAG_SWAP       (1 << 2)
/*! Copy page on write */
#define PAGE_NP_FLAG_COW        (1 << 9)

/*! Standard page size (4 kilobytes) */
#define PAGE_SIZE               0x1000
/*! Large page size (2 megabytes), used when PDT.S is set */
#define LARGE_PAGE_SIZE         0x200000
/*! Huge page size (1 gigabyte), used when PDPT.S is set */
#define HUGE_PAGE_SIZE          0x40000000

#ifndef __ASSEMBLER__

#include <pml/cdefs.h>

/*! Offset in virtual memory of physical memory map */
#define KERNEL_VMA              ((uintptr_t) &__kernel_vma)
/*! Start of kernel text segment in virtual memory */
#define KERNEL_START            ((uintptr_t) &__kernel_start)
/*! End of kernel data segment in virtual memory */
#define KERNEL_END              ((uintptr_t) &__kernel_end)

/*!
 * Relocates a 64-bit physical address into a virtual address.
 *
 * @param x the address to relocate
 * @return the virtual address
 */

#define PHYS_REL(x) ((__typeof__ (x)) ((uintptr_t) (x) + LOW_PHYSICAL_BASE_VMA))

/*!
 * Relocates a 32-bit physical address into a virtual address.
 *
 * @param x the address to relocate
 * @return the virtual address
 */

#define PHYS32_REL(x) (PHYS_REL ((uintptr_t) (x)))

/*! Page-align a variable */
#define __page_align            __attribute__ ((aligned (PAGE_SIZE)))

struct page_stack
{
  uintptr_t *base;
  uintptr_t *ptr;
};

/*!
 * Clears all entries in the TLB by reloading the CR3 register.
 */

__always_inline static inline void
vm_clear_tlb (void)
{
  __asm__ volatile ("mov %%cr3, %%rax\nmov %%rax, %%cr3" ::: "memory");
}

/*!
 * Invalidates a single page in the TLB.
 *
 * @param addr address of page to invalidate
 */

__always_inline static inline void
vm_clear_page (void *addr)
{
  __asm__ volatile ("invlpg (%0)" :: "r" (addr) : "memory");
}

/*!
 * Changes the PML4T to use for the current address space.
 *
 * @param addr address of new PML4T
 */

__always_inline static inline void
vm_set_cr3 (uintptr_t addr)
{
  __asm__ volatile ("mov %0, %%cr3" :: "r" (addr) : "memory");
}

__BEGIN_DECLS

extern void *__kernel_vma;
extern void *__kernel_start;
extern void *__kernel_end;

extern uintptr_t kernel_pml4t[PAGE_STRUCT_ENTRIES] __page_align;
extern uintptr_t kernel_thread_local_pdpt[PAGE_STRUCT_ENTRIES] __page_align;
extern uintptr_t phys_map_pdpt[PAGE_STRUCT_ENTRIES * 4] __page_align;

extern struct page_stack phys_page_stack;
extern uintptr_t next_phys_addr;
extern uintptr_t total_phys_mem;

uintptr_t vm_phys_addr (uintptr_t *pml4t, void *addr);
int vm_map_page (uintptr_t *pml4t, uintptr_t phys_addr, void *addr,
		 unsigned int flags);
void vm_skip_holes (void);
void vm_init (void);

void free_pt (uintptr_t *pt);
void free_pdt (uintptr_t *pdt);
void free_pdpt (uintptr_t *pdpt);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
