/* strcpy.c -- This file is part of PML.
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
strcpy (char *__restrict__ dest, const char *__restrict__ src)
{
  char *save_dest = dest;
  long *aligned_dest;
  const long *aligned_src;
  if (ALIGNED (src, sizeof (long)) && ALIGNED (dest, sizeof (long)))
    {
      aligned_dest = (long *) dest;
      aligned_src = (const long *) src;
      while (!LONG_NULL (*aligned_src))
	*aligned_dest++ = *aligned_src++;
      dest = (char *) aligned_dest;
      src = (const char *) aligned_src;
    }
  while ((*dest++ = *src++))
    ;
  return save_dest;
}

char *
strncpy (char *__restrict__ dest, const char *__restrict__ src, size_t len)
{
  char *save_dest = dest;
  long *aligned_dest;
  const long *aligned_src;
  if (ALIGNED (src, sizeof (long)) && ALIGNED (dest, sizeof (long)))
    {
      aligned_dest = (long *) dest;
      aligned_src = (const long *) src;
      while (len >= sizeof (long) && !LONG_NULL (*aligned_src))
	{
	  len -= sizeof (long);
	  *aligned_dest++ = *aligned_src++;
	}
      dest = (char *) aligned_dest;
      src = (const char *) aligned_src;
    }
  while (len > 0)
    {
      len--;
      if (!(*dest++ = *src++))
	break;
    }
  while (len-- > 0)
    *dest++ = '\0';
  return save_dest;
}
