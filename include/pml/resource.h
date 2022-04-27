/* resource.h -- This file is part of PML.
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

#ifndef __PML_RESOURCE_H
#define __PML_RESOURCE_H

/*!
 * @file
 * @brief Process priority and system resource definitions
 */

#include <pml/time.h>

#define RUSAGE_SELF             0
#define RUSAGE_CHILDREN         (-1)

#define RLIMIT_CORE             0
#define RLIMIT_CPU              1
#define RLIMIT_DATA             2
#define RLIMIT_FSIZE            3
#define RLIMIT_MEMLOCK          4
#define RLIMIT_NOFILE           5
#define RLIMIT_NPROC            6
#define RLIMIT_RSS              7
#define RLIMIT_STACK            8

#define RLIM_INFINITY           (-1)        /*!< Resource has no limit */

#define PRIO_PROCESS            1
#define PRIO_PGRP               2
#define PRIO_USER               3

/*! Minimum process priority value */
#define PRIO_MIN                19
/*! Maximum process priority value */
#define PRIO_MAX                -20

/*!
 * Contains information about a process's resource usage.
 */

struct rusage
{
  struct timeval ru_utime;      /*!< User time elapsed */
  struct timeval ru_stime;      /*!< System time elapsed */
  long ru_maxrss;               /*!< Maximum resident set size */
  long ru_ixrss;                /*!< Integral shared text memory size */
  long ru_idrss;                /*!< Integral unshared data size */
  long ru_isrss;                /*!< Integral unshared stack size */
  long ru_minflt;               /*!< Number of page reclaims */
  long ru_majflt;               /*!< Number of page faults */
  long ru_nswap;                /*!< Number of swaps */
  long ru_inblock;              /*!< Number of blocking input operations */
  long ru_oublock;              /*!< Number of blocking output operations */
  long ru_msgsnd;               /*!< Number of messages sent */
  long ru_msgrcv;               /*!< Number of messages received */
  long ru_nsignals;             /*!< Number of signals received */
  long ru_nvcsw;                /*!< Voluntary context switches */
  long ru_nivcsw;               /*!< Involuntary context switches */
};

/*!
 * Structure representing a limit for some resource in a process.
 */

struct rlimit
{
  rlim_t rlim_cur;              /*!< Current resource limit value */
  rlim_t rlim_max;              /*!< Max value the limit can be raised to */
};

#endif
