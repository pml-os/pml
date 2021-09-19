/* strdup.c -- This file is part of PML.
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
strdup (const char *str)
{
  size_t len = strlen (str) + 1;
  char *ptr = malloc (len);
  if (LIKELY (ptr))
    memcpy (ptr, str, len);
  return ptr;
}

char *
strndup (const char *str, size_t len)
{
  size_t str_len = strlen (str);
  char *ptr;
  if (str_len < len)
    len = str_len;
  ptr = malloc (len + 1);
  if (LIKELY (ptr))
    {
      memcpy (ptr, str, len);
      ptr[len] = '\0';
    }
  return ptr;
}
