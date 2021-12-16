/* strtoul.c -- This file is part of PML.
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

#define _SYS_CDEFS_H_

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

enum
{
  positive,
  negative
};

unsigned long
strtoul (const char *__restrict__ str, char **__restrict__ end, int base)
{
  unsigned long converted = 0;
  unsigned long index;
  unsigned long sign;
  const char *start = str;
  unsigned long ret = 0;
  int err = 0;
  while (isspace (*str) || *str == '\t')
    str++;

  if (*str == '-')
    {
      sign = negative;
      str++;
    }
  else if (*str == '+')
    {
      str++;
      sign = positive;
    }
  else
    sign = positive;
  if (base == 0)
    {
      if (*str == '0')
	{
	  if (tolower (*++str) == 'x')
	    {
	      base = 16;
	      str++;
	    }
	  else
	    base = 8;
	}
      else
	base = 10;
    }
  else if (base < 2 || base > 36)
    goto end;

  if (base == 8 && *str == '0')
    str++;
  if (base == 16 && *str == '0' && tolower (*++str) == 'x')
    str++;

  while (*str)
    {
      if (isdigit (*str))
	index = (unsigned long) (*str - '0');
      else
	{
	  index = (unsigned long) toupper (*str);
	  if (isupper (index))
	    index = index - 'A' + 10;
	  else
	    goto end;
	}
      if (index >= (unsigned long) base)
	goto end;
      if (ret > (ULONG_MAX - (unsigned long) index) / (unsigned long) base)
	{
	  err = 1;
	  ret = 0;
	}
      else
	{
	  ret *= base;
	  ret += index;
	  converted = 1;
	}
      str++;
    }

 end:
  if (end)
    {
      if (converted == 0 && ret == 0 && str != NULL)
	*end = (char *) start;
      else
	*end = (char *) str;
    }
  if (err)
    ret = ULONG_MAX;
  if (sign == negative)
    ret = (ULONG_MAX - ret) + 1;
  return ret;
}
