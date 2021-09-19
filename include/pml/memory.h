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

#ifndef ____MEMORY_H
#define ____MEMORY_H

/* Memory layout on x86_64
   0x0000000000000000-0x00007fffffffffff  128T  User space memory
   0xffff800000000000-0xfffffdfdffffffff ~126T  Reserved kernel memory
   0xfffffdfe00000000-0xfffffdfeffffffff    4G  Virtual address copy region
   0xfffffdff00000000-0xfffffdffffffffff    4G  Thread-local storage
   0xfffffe0000000000-0xffffffffffffffff    2T  Physical memory mappings */

#define COPY_REGION_BASE_VMA    0xfffffdfe00000000
#define THREAD_LOCAL_BASE_VMA   0xfffffdff00000000
#define LOW_PHYSICAL_BASE_VMA   0xfffffe0000000000

#define LOW_MEMORY_LIMIT        0x100000

#define PAGE_STRUCT_ALIGN       4096
#define PAGE_STRUCT_SIZE        4096
#define PAGE_STRUCT_ENTRIES     512

#define PAGE_FLAG_PRESENT       (1 << 0)
#define PAGE_FLAG_RW            (1 << 1)
#define PAGE_FLAG_USER          (1 << 2)
#define PAGE_FLAG_WTHRU         (1 << 3)
#define PAGE_FLAG_NOCACHE       (1 << 4)
#define PAGE_FLAG_ACCESS        (1 << 5)
#define PAGE_FLAG_DIRTY         (1 << 6)
#define PAGE_FLAG_SIZE          (1 << 7)
#define PAGE_FLAG_GLOBAL        (1 << 8)

#define PAGE_SIZE               0x1000
#define LARGE_PAGE_SIZE         0x200000
#define HUGE_PAGE_SIZE          0x40000000

#define KERNEL_VMA              ((uintptr_t) &__kernel_vma)
#define KERNEL_START            ((uintptr_t) &__kernel_start)
#define KERNEL_END              ((uintptr_t) &__kernel_end)

#ifndef __ASSEMBLER__

#include <pml/cdefs.h>

#define __page_align            __attribute__ ((aligned (PAGE_SIZE)))

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
extern uintptr_t kernel_data_pdpt[PAGE_STRUCT_ENTRIES] __page_align;
extern uintptr_t phys_map_pdpt[PAGE_STRUCT_ENTRIES * 4] __page_align;
extern uintptr_t copy_region_pdt[PAGE_STRUCT_ENTRIES * 4] __page_align;
extern uintptr_t kernel_tls_pdt[PAGE_STRUCT_ENTRIES * 4] __page_align;

void vm_init (void);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
