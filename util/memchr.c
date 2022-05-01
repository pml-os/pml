/* memchr.c -- This file is part of PML.
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
memchr (const void *ptr, int c, size_t len)
{
  const unsigned char *s = (const unsigned char *) ptr;
  unsigned char ch = c;
  unsigned long *aligned;
  unsigned long mask;
  unsigned long i;

  while (!ALIGNED (s, sizeof (long)))
    {
      if (!len--)
	return NULL;
      if (*s == ch)
	return (void *) s;
      s++;
    }

  if (len >= sizeof (long))
    {
      aligned = (unsigned long *) s;
      mask = (ch << 8) | ch;
      mask = (mask << 16) | mask;
      for (i = 32; i < sizeof (long) * 8; i <<= 1)
	mask = (mask << i) | mask;

      while (len >= sizeof (long))
	{
	  if (LONG_CHAR (*aligned, mask))
	    break;
	  len -= sizeof (long);
	  aligned++;
	}
      s = (unsigned char *) aligned;
    }

  while (len--)
    {
      if (*s == ch)
	return (void *) s;
      s++;
    }
  return NULL;
}

void *
memrchr (const void *ptr, int c, size_t len)
{
  const unsigned char *s = (const unsigned char *) ptr + len - 1;
  unsigned char ch = c;
  unsigned long *aligned;
  unsigned long mask;
  unsigned int i;

  while (!ALIGNED (s, sizeof (long)))
    {
      if (!len--)
	return NULL;
      if (*s == ch)
	return (void *) s;
      s--;
    }

  if (len >= sizeof (long))
    {
      aligned = (unsigned long *) (s - sizeof (long) + 1);
      mask = (ch << 8) | ch;
      mask = (mask << 16) | mask;
      for (i = 32; i < sizeof (long) * 8; i <<= 1)
	mask = (mask << i) | mask;

      while (len >= sizeof (long))
	{
	  if (LONG_CHAR (*aligned, mask))
	    break;
	  len -= sizeof (long);
	  aligned--;
	}
      s = (unsigned char *) aligned + sizeof (long) - 1;
    }

  while (len--)
    {
      if (*s == ch)
	return (void *) s;
      s--;
    }
  return NULL;
}
