/* strcmp.c -- This file is part of PML.
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
strcmp (const char *a, const char *b)
{
  if (ALIGNED (a, long) && ALIGNED (b, long))
    {
      unsigned long *la = (unsigned long *) a;
      unsigned long *lb = (unsigned long *) b;
      while (*la == *lb)
	{
	  if (LONG_NULL (*la))
	    return 0;
	  la++;
	  lb++;
	}
      a = (const char *) la;
      b = (const char *) lb;
    }

  while (*a != '\0' && *a == *b)
    {
      a++;
      b++;
    }
  if (*((unsigned char *) a) > *((unsigned char *) b))
    return 1;
  if (*((unsigned char *) a) < *((unsigned char *) b))
    return -1;
  return 0;
}

int
strncmp (const char *a, const char *b, size_t len)
{
  if (len == 0)
    return 0;
  if (ALIGNED (a, long) && ALIGNED (b, long))
    {
      unsigned long *la = (unsigned long *) a;
      unsigned long *lb = (unsigned long *) b;
      while (len >= sizeof (long) && *la == *lb)
	{
	  len -= sizeof (long);
	  if (len == 0 || LONG_NULL (*la))
	    return 0;
	  la++;
	  lb++;
	}
      a = (const char *) la;
      b = (const char *) lb;
    }

  while (len-- > 0 && *a == *b)
    {
      if (len == 0 || *a == '\0')
	return 0;
      a++;
      b++;
    }
  if (*((unsigned char *) a) > *((unsigned char *) b))
    return 1;
  if (*((unsigned char *) a) < *((unsigned char *) b))
    return -1;
  return 0;
}
