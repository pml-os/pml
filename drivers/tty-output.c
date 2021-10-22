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

/*! @file */

#include <pml/tty.h>
#include <stdio.h>

struct tty kernel_tty;
struct tty *current_tty = &kernel_tty;

int
putchar (int c)
{
  return tty_putchar (current_tty, c);
}

/*!
 * Writes a character to a terminal.
 *
 * @param tty the terminal to write to
 * @param c the character to write, which will be converted to a byte
 */

int
tty_putchar (struct tty *tty, int c)
{
  int wrap = 0;
  if (c == '\0')
    return c;
  switch (c)
    {
    case '\n':
      tty->x = 0;
      __fallthrough;
    case '\v':
    case '\f':
      wrap = 1;
      break;
    case '\r':
      tty->x = 0;
      tty->output->update_cursor (tty);
      return c;
    case '\t':
      tty->x |= 7;
      break;
    default:
      if (tty->output->write_char (tty, tty->x,
					   tty->y, c))
	return EOF;
    }

  if (!wrap && ++tty->x == tty->width)
    {
      tty->x = 0;
      wrap = 1;
    }

  if (wrap && ++tty->y == tty->height)
    {
      if (tty->output->scroll_down (tty))
	return EOF;
      tty->y--;
    }
  tty->output->update_cursor (tty);
  return c;
}
