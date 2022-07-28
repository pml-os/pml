/* sysmacros.h -- This file is part of PML.
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

#ifndef __PML_SYSMACROS_H
#define __PML_SYSMACROS_H

/*!
 * @file
 * @brief Helper macros for device code
 */

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

#endif
