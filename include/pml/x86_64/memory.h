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
 * @brief Memory management macros and functions
 *
 * Virtual memory layout on x86_64: <table>
 * <tr><th>Start address</th><th>End address</th><th>Size</th>
 * <th>Description</th></tr>
 * <tr><td>@c 0x0000000000000000</td><td>@c 0x00003fffffffffff</td><td>64T</td>
 * <td>User-space static/program memory</td></tr>
 * <tr><td>@c 0x0000400000000000</td><td>@c 0x00007fffffffffff</td><td>64T</td>
 * <td>User-space memory mappings</td></tr>
 * <tr><td>@c 0xffff800000000000</td><td>@c 0xfffffcffffffffff</td>
 * <td>~125T</td><td>Reserved kernel memory</td></tr>
 * <tr><td>@c 0xfffffd0000000000</td><td>@c 0xfffffd7fafffffff</td>
 * <td>~511G</td><td>Thread-local storage</td></tr>
 * <tr><td>@c 0xfffffd7fb0000000</td><td>@c 0xfffffd7fbfffffff</td><td>256M</td>
 * <td>Thread-local kernel data</td></tr>
 * <tr><td>@c 0xfffffd7fc0000000</td><td>@c 0xfffffd7fffffffff</td><td>1G</td>
 * <td>Thread stack space</td></tr>
 * <tr><td>@c 0xfffffd8000000000</td><td>@c 0xfffffd8fffffffff</td><td>64G</td>
 * <td>Pipe buffer mappings</td></tr>
 * <tr><td>@c 0xfffffd9000000000</td><td>@c 0xfffffdffffffffff</td><td>448G</td>
 * <td>Reserved kernel memory</td></tr>
 * <tr><td>@c 0xfffffe0000000000</td><td>@c 0xffffffffffffffff</td><td>2T</td>
 * <td>Physical memory mappings</td></tr>
 * </table>
 * A maximum of 2 TiB of physical memory is supported. PML will not be able
 * to access physical memory beyond the 2 TiB address (@ref PHYS_ADDR_LIMIT).
 */

/* Address space definitions */

/*! Base virtual address of user-space memory mappings */
#define USER_MMAP_BASE_VMA      0x0000400000000000
/*! Virtual address of the top of user-space memory */
#define USER_MEM_TOP_VMA        0x0000800000000000
/*! Base virtual address of thread-local storage */
#define THREAD_LOCAL_BASE_VMA   0xfffffd0000000000
/*! Base virtual address of thread-local kernel data */
#define THREAD_KERNEL_DATA_VMA  0xfffffd7fb0000000
/*! Base virtual address of process stack */
#define PROCESS_STACK_BASE_VMA  0xfffffd7fc0000000
/*! Virtual address of the top of the syscall stack */
#define SYSCALL_STACK_TOP_VMA   0xfffffd7fd0000000
/*! Virtual address of the top of the kernel-mode interrupt stack */
#define INTERRUPT_STACK_TOP_VMA 0xfffffd7fe0000000
/*! Virtual address of the top of the user-mode process stack */
#define PROCESS_STACK_TOP_VMA   0xfffffd8000000000
/*! Base virtual address of pipe buffer area */
#define PIPE_BUFFER_BASE_VMA    0xfffffd8000000000
/*! Top virtual address of pipe buffer area */
#define PIPE_BUFFER_TOP_VMA     0xfffffd9000000000
/*! Base of physical memory map */
#define LOW_PHYSICAL_BASE_VMA   0xfffffe0000000000

/*! First memory address in upper memory */
#define LOW_MEMORY_LIMIT        0x0000000000100000
/*! Last byte of user-space memory */
#define USER_MEMORY_LIMIT       0x00007fffffffffff
/*! Maximum addressible physical address supported */
#define PHYS_ADDR_LIMIT         0x0000020000000000

/* Thread-local kernel data addresses */

/*! Address where the siginfo structure for a process is passed */
#define SIGINFO_VMA             0xfffffd7fb0000000
/*! Address of the signal trampoline */
#define SIGNAL_TRAMPOLINE_VMA   0xfffffd7fb0001000

/* Paging definitions */

/*! Required alignment of page structures */
#define PAGE_STRUCT_ALIGN       4096
/*! Size of page structures */
#define PAGE_STRUCT_SIZE        4096
/*! Number of 8-byte entries in page structures */
#define PAGE_STRUCT_ENTRIES     512

#define PAGE_FLAG_PRESENT       (1 << 0)  /*!< Page present */
#define PAGE_FLAG_RW            (1 << 1)  /*!< Read and write access */
#define PAGE_FLAG_USER          (1 << 2)  /*!< User-accessible page */
#define PAGE_FLAG_WTHRU         (1 << 3)  /*!< Write-through page caching */
#define PAGE_FLAG_NOCACHE       (1 << 4)  /*!< Prevent TLB from caching page */
#define PAGE_FLAG_ACCESS        (1 << 5)  /*!< Set when page is accessed */
#define PAGE_FLAG_DIRTY         (1 << 6)  /*!< Set when page is written to */
#define PAGE_FLAG_SIZE          (1 << 7)  /*!< Use larger page size */
#define PAGE_FLAG_GLOBAL        (1 << 8)  /*!< Global page */

#define PAGE_FLAG_SWAP          (1 << 1)  /*!< Fetch page from swap space */
#define PAGE_FLAG_COW           (1 << 9)  /*!< Copy page on write */

#define PAGE_ERR_PRESENT        (1 << 0)  /*!< Page-protection violation */
#define PAGE_ERR_WRITE          (1 << 1)  /*!< Write access */
#define PAGE_ERR_USER           (1 << 2)  /*!< User mode access */
#define PAGE_ERR_RESERVED       (1 << 3)  /*!< Reserved write */
#define PAGE_ERR_INST           (1 << 4)  /*!< Instruction fetch */

/*! Standard page size (4 kilobytes) */
#define PAGE_SIZE               0x1000
/*! Large page size (2 megabytes), used when PDT.S is set */
#define LARGE_PAGE_SIZE         0x200000
/*! Huge page size (1 gigabyte), used when PDPT.S is set */
#define HUGE_PAGE_SIZE          0x40000000

/*! Address of system memory map */
#define MMAP_ADDR               0xfffffe0000009000

#ifdef __ASSEMBLER__

#define KERNEL_VMA              __kernel_vma
#define KERNEL_START            __kernel_start
#define KERNEL_END              __kernel_end

#else

#include <pml/cdefs.h>

/*! Offset in virtual memory of physical memory map */
#define KERNEL_VMA              ((uintptr_t) &__kernel_vma)
/*! Start of kernel text segment in virtual memory */
#define KERNEL_START            ((uintptr_t) &__kernel_start)
/*! End of kernel data segment in virtual memory */
#define KERNEL_END              ((uintptr_t) &__kernel_end)

/*!
 * Calculates the index in a PML4T corresponding to a virtual address.
 *
 * @param v the virtual address as an integer or pointer type
 * @return the PML4T index
 */

#define PML4T_INDEX(v)          (((uintptr_t) (v) >> 39) & 0x1ff)

/*!
 * Calculates the index in a PDPT corresponding to a virtual address.
 *
 * @param v the virtual address as an integer or pointer type
 * @return the PDPT index
 */

#define PDPT_INDEX(v)           (((uintptr_t) (v) >> 30) & 0x1ff)

/*!
 * Calculates the index in a PDT corresponding to a virtual address.
 *
 * @param v the virtual address as an integer or pointer type
 * @return the PDT index
 */

#define PDT_INDEX(v)            (((uintptr_t) (v) >> 21) & 0x1ff)

/*!
 * Calculates the index in a PT corresponding to a virtual address.
 *
 * @param v the virtual address as an integer or pointer type
 * @return the PT index
 */

#define PT_INDEX(v)             (((uintptr_t) (v) >> 12) & 0x1ff)

/*!
 * Relocates a 64-bit physical address into a virtual address.
 *
 * @param x the address to relocate
 * @return the virtual address as the same type as the input
 */

#define PHYS_REL(x) ((__typeof__ (x)) ((uintptr_t) (x) + LOW_PHYSICAL_BASE_VMA))

/*!
 * Relocates a 32-bit physical address into a virtual address. This function
 * is the only function that will work for relocating low memory physical
 * addresses represented by integer literals.
 *
 * @param x the address to relocate
 * @return the virtual address as a 64-bit integer
 */

#define PHYS32_REL(x) (PHYS_REL ((uintptr_t) (x)))

/*! Page-align a variable */
#define __page_align            __attribute__ ((aligned (PAGE_SIZE)))

/*!
 * Metadata of a page for the physical page frame allocator.
 */

struct page_meta
{
  /*! Number of references to this page, zero means the page is not allocated */
  unsigned int count;
};

/*!
 * Represents a region of accessible physical memory. This structure is used
 * to generate a memory map of the system on boot.
 */

struct mem_region
{
  uint64_t base;                /*!< First physical address of memory region */
  uint64_t len;                 /*!< Length of memory region */
};

/*!
 * Represents a memory map of the system.
 */

struct mem_map
{
  struct mem_region *regions;   /*!< Array of memory region structures */
  size_t count;                 /*!< Number of memory regions */
  size_t curr;                  /*!< Current memory region */
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
extern uintptr_t alloc_space_pdpt[PAGE_STRUCT_ENTRIES] __page_align;
extern uintptr_t phys_map_pdpt[PAGE_STRUCT_ENTRIES * 4] __page_align;

extern struct page_meta *phys_alloc_table;
extern uintptr_t next_phys_addr;
extern uintptr_t total_phys_mem;
extern struct mem_map mmap;

uintptr_t physical_addr (void *addr);
uintptr_t vm_phys_addr (uintptr_t *pml4t, void *addr);
int vm_map_page (uintptr_t *pml4t, uintptr_t phys_addr, void *addr,
		 unsigned int flags);
int vm_unmap_page (uintptr_t *pml4t, void *addr);
void vm_next_page (void);
void vm_unmap_user_mem (uintptr_t *pml4t);
void vm_init (void);
void mark_resv_mem_alloc (void);

void ref_pt (uintptr_t *pt);
void ref_pdt (uintptr_t *pdt);
void ref_pdpt (uintptr_t *pdpt);
void free_pt (uintptr_t *pt);
void free_pdt (uintptr_t *pdt);
void free_pdpt (uintptr_t *pdpt);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
