/* time.c -- This file is part of PML.
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

#include <pml/hpet.h>
#include <errno.h>
#include <stdlib.h>

time_t real_time;

time_t
time (time_t *t)
{
  time_t tt = real_time + hpet_nanotime () / 1000000000;
  if (t)
    *t = tt;
  return tt;
}

int
sys_gettimeofday (struct timeval *tv, struct timezone *tz)
{
  clock_t us = hpet_nanotime () / 1000;
  if (!tv)
    return 0;
  tv->tv_sec = real_time + us / 1000000;
  tv->tv_usec = us % 1000000;
  return 0;
}

int
sys_settimeofday (const struct timeval *tv, const struct timezone *tz)
{
  if (THIS_PROCESS->euid)
    RETV_ERROR (EPERM, -1);
  if (!tv)
    return 0;
  if (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000)
    RETV_ERROR (EINVAL, -1);
  real_time = tv->tv_sec - hpet_nanotime () / 1000000000;
  return 0;
}
