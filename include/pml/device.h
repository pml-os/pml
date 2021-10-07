/* device.h -- This file is part of PML.
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

#ifndef __PML_DEVICE_H
#define __PML_DEVICE_H

/*!
 * @file
 * @brief Definitions for special device files
 *
 * Device numbering namespace: <table>
 * <tr><th>Major</th><th>Minor</th><th>Description</th></tr>
 * <tr><td>0</td><td>*</td><td>Special character devices</td></tr>
 * <tr><td>1-4</td><td>0</td><td>IDE devices</td></tr>
 * <tr><td>1-4</td><td>1-4</td><td>IDE device partitions</td></tr>
 * </table>
 */

#include <pml/map.h>
#include <pml/types.h>

/*! Types of special device files */

enum device_type
{
  DEVICE_TYPE_BLOCK,            /*!< Block device */
  DEVICE_TYPE_CHAR              /*!< Character device */
};

/*!
 * Basic device structure. This structure should be casted to the correct type
 * based on the value of @ref device.type.
 */

struct device
{
  enum device_type type;        /*!< Device file type */
  dev_t major;                  /*!< Major number */
  dev_t minor;                  /*!< Minor number */
  void *data;                   /*!< Device driver private data */
};

/*!
 * Represents a block device. Block devices perform I/O in blocks of data
 * and support seeking.
 */

struct block_device
{
  struct device device;         /*!< Basic device structure */
  blksize_t block_size;         /*!< Size of a block for I/O */

  /*! Read bytes from an offset in the device file */
  ssize_t (*read) (struct block_device *, void *, size_t, off_t, int);
  /*! Write bytes to an offset in the device file */
  ssize_t (*write) (struct block_device *, const void *, size_t, off_t, int);
};

/*!
 * Represents a character device. Character devices perform I/O in bytes
 * and do not support seeking.
 */

struct char_device
{
  struct device device;         /*!< Basic device structure */

  /*! Read a byte from the device file */
  ssize_t (*read) (struct char_device *, unsigned char *, int);

  /*! Write a byte to the device file */
  ssize_t (*write) (struct char_device *, unsigned char *, int);
};

/*!
 * Structure used as private data for an ATA drive block device.
 */

struct disk_device_data
{
  struct ata_device *device;        /*!< Pointer to ATA device */
  unsigned long lba;                /*!< LBA corresponding to start of device */
  size_t len;                       /*!< Number of bytes accessible to device */
};

/*!
 * Format of an entry in an MBR partition table.
 */

struct mbr_part
{
  unsigned char attr;               /*!< Drive attributes */
  unsigned char chs_start[3];       /*!< CHS address of start of partition */
  unsigned char type;               /*!< Partition type */
  unsigned char chs_end[3];         /*!< CHS address of end of partition */
  uint32_t lba;                     /*!< LBA of start of partition */
  uint32_t sectors;                 /*!< Number of sectors in partition */
};

/*!
 * Format of the master boot record, the first block of a disk partitioned
 * using the MBR format.
 */

struct mbr
{
  unsigned char bootstrap[440];     /*!< MBR bootstrap code */
  uint32_t disk_id;                 /*!< Unique disk ID or signature */
  uint16_t reserved;
  struct mbr_part part_table[4];    /*!< Partition table */
  uint16_t magic;                   /*!< Must be @c 0x55 @c 0xaa */
} __packed;

__BEGIN_DECLS

extern struct hashmap *device_name_map;
extern struct hashmap *device_num_map;

void device_map_init (void);
void device_ata_init (void);

ssize_t ata_device_read (struct block_device *device, void *buffer, size_t len,
			 off_t offset, int block);
ssize_t ata_device_write (struct block_device *device, const void *buffer,
			  size_t len, off_t offset, int block);

__END_DECLS

#endif
