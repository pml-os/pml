/* strchr.c -- This file is part of PML.
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
strchr (const char *str, int c)
{
  const unsigned char *s = (const unsigned char *) str;
  unsigned char ch = c;
  unsigned long *aligned;
  unsigned long mask;
  unsigned long i;

  if (!ch)
    {
      while (!ALIGNED (s, sizeof (long)))
	{
	  if (!*s)
	    return (char *) s;
	  s++;
	}
      aligned = (unsigned long *) s;
      while (!LONG_NULL (*aligned))
	aligned++;
      s = (const unsigned char *) aligned;
      while (*s)
	s++;
      return (char *) s;
    }

  while (!ALIGNED (s, sizeof (long)))
    {
      if (!*s)
	return NULL;
      if (*s == ch)
	return (char *) s;
      s++;
    }

  mask = ch;
  for (i = 8; i < sizeof (long) * 8; i <<= 1)
    mask = (mask << i) | mask;
  aligned = (unsigned long *) s;
  while (!LONG_NULL (*aligned) && !LONG_CHAR (*aligned, mask))
    aligned++;
  s = (unsigned char *) aligned;
  while (*s && *s != ch)
    s++;
  return *s == ch ? (char *) s : NULL;
}

char *
strrchr (const char *str, int c)
{
  const char *last = NULL;
  if (c)
    {
      while ((str = strchr (str, c)))
	{
	  last = str;
	  str++;
	}
    }
  else
    last = strchr (str, c);
  return (char *) last;
}
