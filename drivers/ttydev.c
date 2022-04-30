/* ttydev.c -- This file is part of PML.
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

#include <pml/device.h>
#include <pml/tty.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

ssize_t
tty_device_read (struct char_device *dev, unsigned char *c, int block)
{
  struct tty *tty = dev->device.data;
  if (block)
    {
      while (!tty->input.size)
	;
    }
  if (tty->input.size)
    {
      *c = tty->input.buffer[tty->input.start];
      tty->input.size--;
      if (++tty->input.start == TTY_INPUT_BUFFER_SIZE)
	tty->input.start = 0;
      return *c == '\n' ? 2 : 1;
    }
  else
    return 0;
}

ssize_t
tty_device_write (struct char_device *dev, unsigned char c, int block)
{
  struct tty *tty = dev->device.data;
  return tty_putchar (tty, c) == EOF ? -1 : 1;
}

/*!
 * Initializes the terminal device /dev/console.
 */

void
tty_device_init (void)
{
  struct char_device *device =
    (struct char_device *) device_add ("console", 0, 0, DEVICE_TYPE_CHAR);
  if (UNLIKELY (!device))
    {
      printf ("tty: failed to allocate /dev/console\n");
      return;
    }
  device->device.data = current_tty;
  device->read = tty_device_read;
  device->write = tty_device_write;
}
