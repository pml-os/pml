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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*! Buffer for reading the master boot record of a drive. */
static struct mbr mbr_buffer;

/*! Hashmap of special device files with device names as keys. */
struct hashmap *device_name_map;

/*! Hashmap of special device files with device numbers as keys. */
struct hashmap *device_num_map;

/*!
 * Allocates the device hashmap @ref device_map and adds basic device files
 * to it.
 */

void
device_map_init (void)
{
  device_name_map = hashmap_create ();
  device_num_map = hashmap_create ();
  if (UNLIKELY (!device_name_map) || UNLIKELY (!device_num_map))
    panic ("Failed to create device map");
}

/*!
 * Registers a new block or character device to the system.
 *
 * @param name the name of the device
 * @param major the major number of the device
 * @param minor the minor number of the device
 * @param type the type of the device (block or character)
 * @return the device structure, or NULL if the allocation failed
 */

struct device *
device_add (const char *name, dev_t major, dev_t minor, enum device_type type)
{
  struct device *device = NULL;
  unsigned long name_key = siphash (name, strlen (name), 0);
  unsigned long num_key = (major << 32) | (minor & 0xffffffff);
  switch (type)
    {
    case DEVICE_TYPE_BLOCK:
      device = (struct device *) calloc (1, sizeof (struct block_device));
      break;
    case DEVICE_TYPE_CHAR:
      device = (struct device *) calloc (1, sizeof (struct char_device));
      break;
    }
  if (UNLIKELY (!device))
    return NULL;
  device->type = type;
  device->major = major;
  device->minor = minor;
  if (hashmap_insert (device_name_map, name_key, device))
    {
      free (device);
      return NULL;
    }
  if (hashmap_insert (device_num_map, name_key, device))
    {
      hashmap_remove (device_name_map, name_key);
      free (device);
      return NULL;
    }
  return device;
}

/*!
 * Creates block devices for ATA devices. A block device representing the
 * entire disk drive is created, and additional block devices for each MBR
 * partition of the drive are also created.
 */

void
device_ata_init (void)
{
  unsigned char i;
  int count = 0;
  for (i = 0; i < 4; i++)
    {
      if (ata_devices[i].exists)
	{
	  char name[] = "sdxxx";
	  struct block_device *device;
	  name[2] = 'a' + count++;
	  name[3] = '\0';
	  device = (struct block_device *) device_add (name, i + 1, 0,
						       DEVICE_TYPE_BLOCK);
	  if (UNLIKELY (!device))
	    {
	      debug_printf ("failed to add device for IDE drive %d\n", i);
	      continue;
	    }

	  device->device.data = ata_devices + i;
	  device->block_size = ATA_SECTOR_SIZE;
	  device->read = ata_device_read;
	  device->write = ata_device_write;

	  if (ata_devices[i].size > 0
	      && !ata_read_sectors (ata_devices[i].channel,
				    ata_devices[i].drive, 1, 0, &mbr_buffer)
	      && mbr_buffer.magic == 0xaa55)
	    {
	      debug_printf ("Found an MBR on drive %d\n", i);
	    }
	}
    }
}
