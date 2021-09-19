/* tty-output.c -- This file is part of PML.
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

#include <pml/tty.h>
#include <stdio.h>

struct tty kernel_tty;
struct tty *current_tty = &kernel_tty;

int
putchar (int c)
{
  int wrap = 0;
  if (c == '\0')
    return c;
  switch (c)
    {
    case '\n':
      current_tty->x = 0;
      __fallthrough;
    case '\v':
    case '\f':
      wrap = 1;
      break;
    case '\r':
      current_tty->x = 0;
      current_tty->output->update_cursor (current_tty);
      return c;
    case '\t':
      current_tty->x |= 7;
      break;
    default:
      if (current_tty->output->write_char (current_tty, current_tty->x,
					   current_tty->y, c))
	return EOF;
    }

  if (!wrap && ++current_tty->x == current_tty->width)
    {
      current_tty->x = 0;
      wrap = 1;
    }

  if (wrap && ++current_tty->y == current_tty->height)
    {
      if (current_tty->output->scroll_down (current_tty))
	return EOF;
      current_tty->y--;
    }
  current_tty->output->update_cursor (current_tty);
  return c;
}
