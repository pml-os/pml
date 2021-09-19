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

#include <pml/multiboot.h>
#include <stdio.h>

void
multiboot_init (uintptr_t addr)
{
  printf ("Initializing boot parameters\n");
  for (struct mb_tag *tag = (struct mb_tag *) (addr + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct mb_tag *) ((uintptr_t) tag + ((tag->size + 7) & ~7)))
    {
      switch (tag->type)
	{
	case MULTIBOOT_TAG_TYPE_CMDLINE:
	  printf ("Boot command line: %s\n",
		  ((struct mb_str_tag *) tag)->string);
	  break;
	case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
	  printf ("Boot loader name: %s\n",
		  ((struct mb_str_tag *) tag)->string);
	  break;
	}
    }
}
