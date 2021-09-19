/* memmove.c -- This file is part of PML.
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

void *
memmove (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  long *ad;
  const long *as;
  if (s < d && d < s + len)
    {
      s += len;
      d += len;
      while (len-- > 0)
	*--d = *--s;
    }
  else
    {
      if (len >= sizeof (long) * 4 && ALIGNED (s, long) && ALIGNED (d, long))
	{
	  ad = (long *) d;
	  as = (const long *) s;
	  while (len >= sizeof (long) * 4)
	    {
	      *ad++ = *as++;
	      *ad++ = *as++;
	      *ad++ = *as++;
	      *ad++ = *as++;
	      len -= sizeof (long) * 4;
	    }
	  while (len > sizeof (long))
	    {
	      *ad++ = *as++;
	      len -= sizeof (long);
	    }
	  d = (char *) ad;
	  s = (const char *) as;
	}

      while (len-- > 0)
	*d++ = *s++;
    }
  return dest;
}
