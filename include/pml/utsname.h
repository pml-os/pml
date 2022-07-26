/* utsname.h -- This file is part of PML.
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

#ifndef __PML_UTSNAME_H
#define __PML_UTSNAME_H

#include <pml/syslimits.h>

struct utsname
{
  char sysname[9];
  char nodename[HOST_NAME_MAX + 1];
  char release[9];
  char version[9];
  char machine[9];
};

#endif
