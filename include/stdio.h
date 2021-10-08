/* stdio.h -- This file is part of PML.
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

#ifndef __STDIO_H
#define __STDIO_H

/*! @file */

#include <pml/cdefs.h>
#include <stdarg.h>

/*! Indicates the end of input. */
#define EOF                     (-1)

/*!
 * printf() wrapper used for debugging. The printed message contains
 * the source file and line number of the statement that calls this macro.
 *
 * @param fmt the printf-style format string
 */

#define debug_printf(fmt, ...)					\
  printf ("%s:%d: " fmt, __FILE__, __LINE__,  ## __VA_ARGS__ )

/*!
 * Various boot options that can be set on the command line.
 */

struct boot_options
{
  char *root_device;            /*!< Device to mount as root partition */
};

__BEGIN_DECLS

extern char *command_line;
extern struct boot_options boot_options;

void init_command_line (void);

int putchar (int c);

int printf (const char *__restrict__ fmt, ...);
int sprintf (char *__restrict__ buffer, const char *__restrict__ fmt, ...);
int snprintf (char *__restrict__ buffer, size_t len,
	      const char *__restrict__ fmt, ...);
int vprintf (const char *__restrict__ fmt, va_list args);
int vsprintf (char *__restrict__ buffer, const char *__restrict__ fmt,
	      va_list args);
int vsnprintf (char *__restrict__ buffer, size_t len,
	       const char *__restrict__ fmt, va_list args);

__END_DECLS

#endif
