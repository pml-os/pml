/* multiboot.c -- This file is part of PML.
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

#include <acpi/acpi.h>
#include <pml/memory.h>
#include <pml/multiboot.h>
#include <pml/panic.h>
#include <stdio.h>
#include <stdlib.h>

static struct mb_mmap_tag *multiboot_mmap;
static size_t mmap_curr_region;
static size_t mmap_region_count;

/* Checks if the next physical address is in a memory hole according to
   the memory map given by Multiboot2. */

void
vm_skip_holes (void)
{
  struct mb_mmap_entry *entry = multiboot_mmap->entries + mmap_curr_region;
  if (next_phys_addr >= entry->base + entry->len)
    {
      /* Move to the next region. If we reach the end of all memory regions
	 there is no more physical memory left. */
      if (UNLIKELY (++mmap_curr_region == mmap_region_count))
	panic ("Physical memory exhausted");
      next_phys_addr =
	ALIGN_UP (multiboot_mmap->entries[mmap_curr_region].base, PAGE_SIZE);
    }
}

void
multiboot_init (uintptr_t addr)
{
  struct mb_tag *tag;
  size_t i;
  printf ("Initializing boot parameters\n");
  for (tag = (struct mb_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct mb_tag *) ((uintptr_t) tag + ((tag->size + 7) & ~7)))
    {
      switch (tag->type)
	{
	case MULTIBOOT_TAG_TYPE_CMDLINE:
	  printf ("Boot command line: %s\n",
		  ((struct mb_str_tag *) tag)->string);
	  break;
	case MULTIBOOT_TAG_TYPE_MMAP:
	  multiboot_mmap = (struct mb_mmap_tag *) tag;
	  printf ("System memory map:\n");
	  for (i = 0; i < multiboot_mmap->tag.size -
		 sizeof (struct mb_mmap_tag); i += multiboot_mmap->entry_size)
	    {
	      struct mb_mmap_entry *entry =
		(struct mb_mmap_entry *) ((char *) multiboot_mmap->entries + i);
	      if (entry->type == 1)
		{
		  mmap_region_count++;
		  printf ("  %p-%p (%H)\n", (char *) entry->base,
			  (char *) entry->base + entry->len, entry->len);
		}
	    }
	  break;
	case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
	  printf ("Boot loader name: %s\n",
		  ((struct mb_str_tag *) tag)->string);
	  break;
	case MULTIBOOT_TAG_TYPE_ACPI_OLD:
	case MULTIBOOT_TAG_TYPE_ACPI_NEW:
	  acpi_rsdp = &((struct mb_acpi_rsdp_tag *) tag)->rsdp;
	  break;
	}
    }
  if (UNLIKELY (!multiboot_mmap))
    panic ("No memory map provided by boot loader");
}
