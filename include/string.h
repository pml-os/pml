/* string.h -- This file is part of PML.
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

#ifndef __STRING_H
#define __STRING_H

#include <pml/cdefs.h>

__BEGIN_DECLS

void *memcpy (void *__restrict__ dest, const void *__restrict__ src,
	      size_t len);
void *mempcpy (void *__restrict__ dest, const void *__restrict__ src,
	       size_t len);
void *memmove (void *dest, const void *src, size_t len);
void *memset (void *ptr, int c, size_t len);
int memcmp (const void *a, const void *b, size_t len) __pure;
void *memchr (const void *ptr, int c, size_t len) __pure;
void *memrchr (const void *ptr, int c, size_t len) __pure;

int strcmp (const char *a, const char *b) __pure;
int strncmp (const char *a, const char *b, size_t len) __pure;
size_t strlen (const char *str) __pure;
size_t strnlen (const char *str, size_t len) __pure;
char *strdup (const char *str);
char *strndup (const char *str, size_t len);
char *strcpy (char *__restrict__ dest, const char *__restrict__ src);
char *strncpy (char *__restrict__ dest, const char *__restrict__ src,
	       size_t len);
char *strcat (char *__restrict__ dest, const char *__restrict__ src);
char *strncat (char *__restrict__ dest, const char *__restrict__ src,
	       size_t len);
char *strchr (const char *str, int c) __pure;
char *strrchr (const char *str, int c) __pure;
char *strstr (const char *haystack, const char *needle) __pure;

__END_DECLS

#endif
