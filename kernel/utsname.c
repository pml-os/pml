/* utsname.c -- This file is part of PML.
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

/*! @file */

#include <pml/utsname.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*!
 * The hostname of the system on some undefined network. This value
 * is meaningless and must be set by the user through the sethostname(2)
 * system call. This value is reported as the node name in the utsname
 * structure.
 */

char hostname[HOST_NAME_MAX + 1];

/*! The length of the host name excluding the null terminating character */
size_t hostname_len;

int
sys_gethostname (char *name, size_t len)
{
  if (hostname_len >= len)
    RETV_ERROR (ENAMETOOLONG, -1);
  strcpy (name, hostname);
  return 0;
}

int
sys_sethostname (const char *name, size_t len)
{
  if (THIS_PROCESS->euid)
    RETV_ERROR (EPERM, -1);
  if (len > HOST_NAME_MAX)
    RETV_ERROR (EINVAL, -1);
  strcpy (hostname, name);
  hostname_len = len;
  return 0;
}

int
sys_uname (struct utsname *buffer)
{
  strncpy (buffer->sysname, "PML", sizeof (buffer->sysname));
  strncpy (buffer->nodename, hostname, sizeof (buffer->nodename));
  strncpy (buffer->release, VERSION, sizeof (buffer->release));
  strncpy (buffer->version, VERSION, sizeof (buffer->version));
  strncpy (buffer->machine, STR (ARCH), sizeof (buffer->machine));
  return 0;
}
