/* strstr.c -- This file is part of PML.
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

#include <string.h>

/* TODO This is a simple but inefficient implementation of strstr(),
   use a faster implementation */

char *
strstr (const char *haystack, const char *needle)
{
  size_t len = strlen (needle);
  while (*haystack)
    {
      if (!memcmp (haystack, needle, len))
	return (char *) haystack;
      haystack++;
    }
  return NULL;
}
