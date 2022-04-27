/* resource.c -- This file is part of PML.
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

#include <pml/syscall.h>
#include <errno.h>
#include <string.h>

int
sys_getrusage (int who, struct rusage *rusage)
{
  switch (who)
    {
    case RUSAGE_SELF:
      memcpy (rusage, &THIS_PROCESS->self_rusage, sizeof (struct rusage));
      return 0;
    case RUSAGE_CHILDREN:
      memcpy (rusage, &THIS_PROCESS->child_rusage, sizeof (struct rusage));
      return 0;
    default:
      RETV_ERROR (EINVAL, -1);
    }
}
