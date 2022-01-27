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

/*! @file */

#include <pml/acpi.h>
#include <pml/memory.h>
#include <pml/multiboot.h>
#include <pml/panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uintptr_t rel_addr = MULTIBOOT_REL_ADDR;

/*!
 * Parses the Multiboot2 structure.
 *
 * @param addr the address of the Multiboot2 structure
 */

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
	  {
	    char *str = (char *) ((struct mb_str_tag *) tag)->string;
	    size_t len = strlen (str) + 1;
	    memcpy ((void *) rel_addr, str, len);
	    command_line = PHYS_REL ((void *) rel_addr);
	    rel_addr += len;
	    printf ("Boot command line: %s\n", command_line);
	    break;
	  }
	case MULTIBOOT_TAG_TYPE_MMAP:
	  {
	    struct mb_mmap_tag *mb_mmap = (struct mb_mmap_tag *) PHYS_REL (tag);
	    mmap.regions = (struct mem_region *) MMAP_ADDR;
	    printf ("System memory map:\n");
	    for (i = 0; i < mb_mmap->tag.size -
		   sizeof (struct mb_mmap_tag); i += mb_mmap->entry_size)
	      {
		struct mb_mmap_entry *entry =
		  (struct mb_mmap_entry *) ((char *) mb_mmap->entries + i);
		if (entry->type == 1)
		  {
		    printf ("  %p-%p (%H)\n", (char *) entry->base,
			    (char *) entry->base + entry->len, entry->len);
		    mmap.regions[mmap.count].base = entry->base;
		    mmap.regions[mmap.count].len = entry->len;
		    mmap.count++;
		    total_phys_mem += entry->len;
		  }
	      }
	    break;
	  }
	case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
	  printf ("Boot loader name: %s\n",
		  ((struct mb_str_tag *) tag)->string);
	  break;
	case MULTIBOOT_TAG_TYPE_ACPI_OLD:
	case MULTIBOOT_TAG_TYPE_ACPI_NEW:
	  memcpy ((void *) rel_addr, &((struct mb_acpi_rsdp_tag *) tag)->rsdp,
		  sizeof (struct acpi_rsdp));
	  acpi_rsdp = PHYS_REL ((void *) rel_addr);
	  rel_addr += sizeof (struct acpi_rsdp);
	  break;
	}
    }
  if (UNLIKELY (!mmap.regions))
    panic ("No memory map provided by boot loader");
}
