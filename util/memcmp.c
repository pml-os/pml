/* memcmp.c -- This file is part of PML.
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

int
memcmp (const void *a, const void *b, size_t len)
{
  unsigned char *ca = (unsigned char *) a;
  unsigned char *cb = (unsigned char *) b;
  unsigned long *la;
  unsigned long *lb;

  if (len >= sizeof (long) && ALIGNED (ca, long) && ALIGNED (cb, long))
    {
      la = (unsigned long *) a;
      lb = (unsigned long *) b;
      while (len >= sizeof (long))
	{
	  if (*la != *lb)
	    break;
	  la++;
	  lb++;
	  len -= sizeof (long);
	}
      ca = (unsigned char *) la;
      cb = (unsigned char *) lb;
    }

  while (len-- > 0)
    {
      if (*ca < *cb)
	return -1;
      if (*ca > *cb)
	return 1;
      ca++;
      cb++;
    }
  return 0;
}
