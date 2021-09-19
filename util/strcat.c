/* strcat.c -- This file is part of PML.
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

char *
strcat (char *__restrict__ dest, const char *__restrict__ src)
{
  char *save_dest = dest;
  if (ALIGNED (dest, sizeof (long)))
    {
      unsigned long *aligned = (unsigned long *) dest;
      while (!LONG_NULL (*aligned))
	aligned++;
      dest = (char *) aligned;
    }
  while (*dest)
    dest++;
  strcpy (dest, src);
  return save_dest;
}

char *
strncat (char *__restrict__ dest, const char *__restrict__ src, size_t len)
{
  char *save_dest = dest;
  if (ALIGNED (dest, sizeof (long)))
    {
      unsigned long *aligned = (unsigned long *) dest;
      while (!LONG_NULL (*aligned))
	aligned++;
      dest = (char *) aligned;
    }
  while (*dest)
    dest++;
  while (len-- > 0 && (*dest++ = *src++))
    {
      if (!len)
	*dest = '\0';
    }
  return save_dest;
}
