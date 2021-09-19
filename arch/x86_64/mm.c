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

uintptr_t kernel_pml4t[PAGE_STRUCT_ENTRIES];
uintptr_t kernel_data_pdpt[PAGE_STRUCT_ENTRIES];
uintptr_t phys_map_pdpt[PAGE_STRUCT_ENTRIES * 4];
uintptr_t copy_region_pdt[PAGE_STRUCT_ENTRIES * 4];
uintptr_t kernel_tls_pdt[PAGE_STRUCT_ENTRIES * 4];

void
vm_init (void)
{
  uintptr_t addr;
  size_t i;
  for (i = 0; i < 4; i++)
    kernel_pml4t[i + 508] =
      ((uintptr_t) (phys_map_pdpt + i * PAGE_STRUCT_ENTRIES) - KERNEL_VMA)
      | PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
  kernel_pml4t[507] = ((uintptr_t) kernel_data_pdpt - KERNEL_VMA)
    | PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
  for (addr = 0, i = 0; i < PAGE_STRUCT_ENTRIES * 4;
       addr += HUGE_PAGE_SIZE, i++)
    phys_map_pdpt[i] = addr | PAGE_FLAG_PRESENT | PAGE_FLAG_RW | PAGE_FLAG_SIZE;
  for (i = 0; i < 4; i++)
    {
      kernel_data_pdpt[i + 504] =
	((uintptr_t) (copy_region_pdt + i * PAGE_STRUCT_ENTRIES) - KERNEL_VMA)
	| PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
      kernel_data_pdpt[i + 508] =
	((uintptr_t) (kernel_tls_pdt + i * PAGE_STRUCT_ENTRIES) - KERNEL_VMA)
	| PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
    }
  vm_set_cr3 ((uintptr_t) kernel_pml4t - KERNEL_VMA);
}
