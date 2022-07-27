/* time.h -- This file is part of PML.
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

#ifndef __PML_TIME_H
#define __PML_TIME_H

/*!
 * @file
 * @brief Time structures
 */

#include <pml/types.h>

/*! Represents an elapsed or absolute time with nanosecond resolution. */

struct timespec
{
  time_t tv_sec;                /*!< Seconds elapsed */
  long tv_nsec;                 /*!< Additional nanoseconds elapsed */
};

/*! Represents an elapsed or absolute time with microsecond resolution. */

struct timeval
{
  time_t tv_sec;                /*!< Seconds elapsed */
  suseconds_t tv_usec;          /*!< Additional microseconds elapsed */
};

/*! Represents a timezone. */

struct timezone
{
  int tz_minuteswest;           /*!< Minutes west of Greenwich */
  int tz_dsttime;               /*!< Type of DST correction */
};

#endif
