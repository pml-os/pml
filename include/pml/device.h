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
 */

#include <pml/cdefs.h>
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
  ssize_t read (struct block_device *, void *, size_t, off_t, int);
  /*! Write bytes to an offset in the device file */
  ssize_t write (struct block_device *, const void *, size_t, off_t, int);
};

/*!
 * Represents a character device. Character devices perform I/O in bytes
 * and do not support seeking.
 */

struct char_device
{
  struct device device;         /*!< Basic device structure */

  /*! Read a byte from the device file */
  ssize_t read (struct char_device *, unsigned char *, int);

  /*! Write a byte to the device file */
  ssize_t write (struct char_device *, unsigned char *, int);
};

__BEGIN_DECLS

__END_DECLS

#endif
