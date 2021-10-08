/* entry.c -- This file is part of PML.
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

#include <pml/ata.h>
#include <pml/device.h>
#include <pml/thread.h>
#include <pml/vfs.h>
#include <stdio.h>
#include <stdlib.h>

static void
splash (void)
{
  printf ("\n\nWelcome to PML 0.1\nCopyright (C) 2021 XNSC\n"
	  "System time: %ld\n", real_time);
}

void
kentry (void)
{
  ata_init ();
  device_map_init ();
  device_ata_init ();
  mount_root ();

  splash ();
  init_pid_allocator ();
}
