/* cmdline.c -- This file is part of PML.
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

/*! @file */

#include <pml/panic.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*! Command line string. */
char *command_line;

/*!
 * Boot options, obtained from command line and parsed with
 * init_command_line().
 */

struct boot_options boot_options;

/*!
 * Parses the command line given to the kernel.
 */

void
init_command_line (void)
{
  char *ptr = command_line;
  char *end;
  while (1)
    {
      char *arg = NULL;
      while (isspace (*ptr))
	ptr++;
      if (!*ptr)
	break;
      for (end = ptr; !isspace (*end); end++)
	{
	  if (*end == '=')
	    arg = end;
	}
      if (arg)
	*arg++ = '\0';
      *end = '\0';
      if (!strcmp (ptr, "root"))
	{
	  if (UNLIKELY (!arg))
	    panic ("Boot option `root' requires an argument");
	  boot_options.root_device = arg;
	}
      ptr = end + 1;
    }
}
