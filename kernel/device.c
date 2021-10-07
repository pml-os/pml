/* device.c -- This file is part of PML.
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

#include <pml/ata.h>
#include <pml/device.h>
#include <pml/panic.h>
#include <stdlib.h>

/*! Hashmap of all registered special device files on the system. */
struct hashmap *device_map;

/*!
 * Allocates the device hashmap @ref device_map and adds basic device files
 * to it.
 */

void
device_map_init (void)
{
  device_map = hashmap_create ();
  if (UNLIKELY (!device_map))
    panic ("Failed to create device map");
}

/*!
 * Creates block devices for ATA devices. A block device representing the
 * entire disk drive is created, and additional block devices for each MBR
 * partition of the drive are also created.
 */

void
device_ata_init (void)
{
}
