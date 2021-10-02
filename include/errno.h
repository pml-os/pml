/* errno.h -- This file is part of PML.
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

#ifndef __ERRNO_H
#define __ERRNO_H

/*!
 * @file
 * @brief Kernel error constants and macros
 */

#include <pml/cdefs.h>
#include <pml/errno.h>

/*!
 * Sets the kernel @c errno value and returns from the calling function.
 * This macro is designed to be called from a function returning @c void.
 *
 * @param e the error value
 */

#define RET_ERROR(e) do				\
    {						\
      errno = (e);				\
      return;					\
    }						\
  while (0)

/*!
 * Sets the kernel @c errno value and returns the given value from the
 * calling function. This macro is designed to be called from a function
 * that returns a value.
 *
 * @param e the error value
 * @param r the return value
 */

#define RETV_ERROR(e, r) do			\
    {						\
      errno = (e);				\
      return (r);				\
    }						\
  while (0)

__BEGIN_DECLS

extern int errno;

__END_DECLS

#endif
