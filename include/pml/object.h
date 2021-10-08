/* object.h -- This file is part of PML.
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

#ifndef __PML_OBJECT_H
#define __PML_OBJECT_H

/*!
 * @file
 * @brief Reference-counted objects
 */

#include <stdlib.h>

/*!
 * Marks a structure definition as a reference-counted object.
 */

#define REF_COUNT               unsigned int __ref_count

/*!
 * Increments the reference count of a pointer to a reference-counted object.
 * If the object is NULL, it is allocated with a call to calloc() and will have
 * a reference count of one. The object passed to this macro will be evaluated
 * more than once, so it should not have any side effects.
 *
 * @param x the object
 * @return the reference count of the object
 */

#define REF_OBJECT(x)							\
  (((x) || ((x) = calloc (1, sizeof (*(x))))) && ++(x)->__ref_count)

/*!
 * Decrements the reference count of a pointer to a reference-counted object.
 * If the object has no remaining references, it is freed by a call to free().
 * The object passed to this macro will be evaluated more than once, so it
 * should not have any side effects.
 *
 * @param x the object
 * @return the reference count of the object
 */

#define UNREF_OBJECT(x)					\
  (--(x)->__ref_count ? (x)->__ref_count : free (x), 0)

#endif