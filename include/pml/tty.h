/* tty.h -- This file is part of PML.
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

#ifndef __PML_TTY_H
#define __PML_TTY_H

#include <pml/cdefs.h>

struct tty;

struct tty_output
{
  int (*write_char) (struct tty *, size_t, size_t, unsigned char);
  int (*clear) (struct tty *);
  int (*update_cursor) (struct tty *);
  int (*scroll_down) (struct tty *);
};

struct tty
{
  unsigned char color;
  size_t width;
  size_t height;
  size_t x;
  size_t y;
  void *screen;
  const struct tty_output *output;
};

__BEGIN_DECLS

extern struct tty kernel_tty;
extern struct tty *current_tty;

__END_DECLS

#endif
