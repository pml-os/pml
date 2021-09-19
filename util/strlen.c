/* strlen.c -- This file is part of PML.
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

#include <stdlib.h>
#include <string.h>

size_t
strlen (const char *str)
{
  const char *ptr = str;
  unsigned long *ls;
  while (!ALIGNED (ptr, long))
    {
      if (!*ptr)
	return ptr - str;
      ptr++;
    }

  ls = (unsigned long *) ptr;
  while (!LONG_NULL (*ls))
    ls++;

  ptr = (char *) ls;
  while (*ptr)
    ptr++;
  return ptr - str;
}

size_t
strnlen (const char *str, size_t len)
{
  const char *ptr = str;
  while (len-- > 0 && *ptr)
    ptr++;
  return ptr - str;
}
