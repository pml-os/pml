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

#include <pml/ata.h>
#include <pml/map.h>
#include <pml/types.h>

/*!
 * Creates a single value representing a device's major and minor numbers.
 *
 * @param major major number of device
 * @param minor minor number of device
 * @return device number as a @ref dev_t value
 */

#define makedev(major, minor)					\
  ((dev_t) ((((major) & 0xffff) << 16) | ((minor) & 0xffff)))

/*!
 * Determines the major number of a device ID.
 *
 * @param dev the device ID
 * @return the major number
 */

#define major(dev) (((dev) >> 16) & 0xffff)

/*!
 * Determines the minor number of a device ID.
 *
 * @param dev the device ID
 * @return the minor number
 */

#define minor(dev) ((dev) & 0xffff)

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
  char *name;                   /*!< Name of device */
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

  /*!
   * Read bytes from a block device file.
   *
   * @param dev the device to read from
   * @param buffer the buffer to store the read data
   * @param len number of bytes to read
   * @param offset offset in file to start reading from
   * @param block whether to block during I/O
   * @return number of bytes read, or -1 on failure
   */
  ssize_t (*read) (struct block_device *dev, void *buffer, size_t len,
		   off_t offset, int block);

  /*!
   * Write bytes to a block device file.
   *
   * @param dev the device to write to
   * @param buffer the buffer containing the data to write
   * @param len number of bytes to write
   * @param offset offset in file to start writing to
   * @param block whether to block during I/O
   * @return number of bytes written, or -1 on failure
   */
  ssize_t (*write) (struct block_device *dev, const void *buffer, size_t len,
		    off_t offset, int block);
};

/*!
 * Represents a character device. Character devices perform I/O in bytes
 * and do not support seeking.
 */

struct char_device
{
  struct device device;         /*!< Basic device structure */

  /*!
   * Read a byte from a character device file.
   *
   * @param dev the device to read from
   * @param c pointer to store character read
   * @param block whether to block during I/O
   * @return <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>1</td><td>Byte was successfully read</td></tr>
   * <tr><td>0</td><td>Couldn't read at this time and not blocking</td></tr>
   * <tr><td>-1</td><td>Read error</td></tr>
   * </table>
   */
  ssize_t (*read) (struct char_device *dev, unsigned char *c, int block);

  /*!
   * Write a byte to a character device file.
   *
   * @param dev the device to write to
   * @param c character to write
   * @param block whether to block during I/O
   * @return <table>
   * <tr><th>Value</th><th>Description</th></tr>
   * <tr><td>1</td><td>Byte was successfully written</td></tr>
   * <tr><td>0</td><td>Couldn't write at this time and not blocking</td></tr>
   * <tr><td>-1</td><td>Write error</td></tr>
   * </table>
   */
  ssize_t (*write) (struct char_device *dev, unsigned char c, int block);
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

extern struct strmap *device_name_map;
extern struct hashmap *device_num_map;

void device_map_init (void);
void device_ata_init (void);

ssize_t ata_device_read (struct block_device *device, void *buffer, size_t len,
			 off_t offset, int block);
ssize_t ata_device_write (struct block_device *device, const void *buffer,
			  size_t len, off_t offset, int block);

__END_DECLS

#endif
