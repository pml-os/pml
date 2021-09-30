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

/* Virtual memory layout on x86_64

   0x0000000000000000-0x00007fffffffffff  128T  User space memory
   0xffff800000000000-0xfffffcffffffffff ~126T  Reserved kernel memory
   0xfffffd8000000000-0xfffffdffbfffffff  511G  Thread-local storage
   0xfffffdffc0000000-0xfffffdffffffffff    1G  Thread stack space
   0xfffffe0000000000-0xffffffffffffffff    2T  Physical memory mappings

   A maximum of 2 TiB of physical memory is supported. PML will not be able
   to access physical memory beyond the 2 TiB address (PHYS_ADDR_LIMIT). */

/* Virtual addresses of memory regions */

#define THREAD_LOCAL_BASE_VMA   0xfffffd8000000000
#define PROCESS_STACK_BASE_VMA  0xfffffdffc0000000
#define PROCESS_STACK_TOP_VMA   0xfffffdfffffffff8
#define LOW_PHYSICAL_BASE_VMA   0xfffffe0000000000

/* Memory limits */

#define LOW_MEMORY_LIMIT        0x0000000000100000
#define PHYS_ADDR_LIMIT         0x0000020000000000

/* Page structure constants */

#define PAGE_STRUCT_ALIGN       4096
#define PAGE_STRUCT_SIZE        4096
#define PAGE_STRUCT_ENTRIES     512

/* Page structure entry flags */

#define PAGE_FLAG_PRESENT       (1 << 0)
#define PAGE_FLAG_RW            (1 << 1)
#define PAGE_FLAG_USER          (1 << 2)
#define PAGE_FLAG_WTHRU         (1 << 3)
#define PAGE_FLAG_NOCACHE       (1 << 4)
#define PAGE_FLAG_ACCESS        (1 << 5)
#define PAGE_FLAG_DIRTY         (1 << 6)
#define PAGE_FLAG_SIZE          (1 << 7)
#define PAGE_FLAG_GLOBAL        (1 << 8)

/* Page structure entry flags for non-present pages, used in the
   page fault handler to determine the action to take when encountering
   a non-present page. The copy-on-write flag is for read-only pages
   that should be copied upon write. */

#define PAGE_NP_FLAG_AOA        (1 << 1) /* Allocate on access */
#define PAGE_NP_FLAG_SWAP       (1 << 2) /* Swap from swap partition */
#define PAGE_NP_FLAG_COW        (1 << 9) /* Copy-on-write */

/* Page sizes */

#define PAGE_SIZE               0x1000
#define LARGE_PAGE_SIZE         0x200000
#define HUGE_PAGE_SIZE          0x40000000

#ifndef __ASSEMBLER__

#include <pml/cdefs.h>

#define KERNEL_VMA              ((uintptr_t) &__kernel_vma)
#define KERNEL_START            ((uintptr_t) &__kernel_start)
#define KERNEL_END              ((uintptr_t) &__kernel_end)

#define PHYS_REL(x) ((__typeof__ (x)) ((uintptr_t) (x) + LOW_PHYSICAL_BASE_VMA))

#define __page_align            __attribute__ ((aligned (PAGE_SIZE)))

struct page_stack
{
  uintptr_t *base;
  uintptr_t *ptr;
};

__always_inline static inline void
vm_clear_tlb (void)
{
  __asm__ volatile ("mov %%cr3, %%rax\nmov %%rax, %%cr3" ::: "memory");
}

__always_inline static inline void
vm_clear_page (void *addr)
{
  __asm__ volatile ("invlpg (%0)" :: "r" (addr) : "memory");
}

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
void vm_skip_holes (void);
void vm_init (void);

void free_pt (uintptr_t *pt);
void free_pdt (uintptr_t *pdt);
void free_pdpt (uintptr_t *pdpt);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
