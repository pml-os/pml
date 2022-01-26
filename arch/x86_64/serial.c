/* serial.c -- This file is part of PML.
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

#include <pml/io.h>
#include <pml/serial.h>
#include <stdio.h>

/*! The serial port number to use */
#define SERIAL_PORT             SERIAL_COM1

const struct tty_output serial_output = {
  .write_char = serial_write_char,
  .clear = serial_clear,
  .update_cursor = serial_update_cursor,
  .scroll_down = serial_scroll_down
};

int
serial_write_char (struct tty *tty, size_t x, size_t y, unsigned char c)
{
  while (!(inb (SERIAL_PORT + SERIAL_LINE_STATUS) & 0x20))
    ;
  outb (c, SERIAL_PORT + SERIAL_DATA);
  return 0;
}

int
serial_clear (struct tty *tty)
{
  return -1;
}

int
serial_update_cursor (struct tty *tty)
{
  return -1;
}

int
serial_scroll_down (struct tty *tty)
{
  return -1;
}

/*!
 * Initializes the serial driver using the selected port number.
 */

void
serial_init (void)
{
  outb (0, SERIAL_PORT + SERIAL_INT_ENABLE);
  outb (SERIAL_DLAB, SERIAL_PORT + SERIAL_LINE_CONTROL);
  outb (SERIAL_DEFAULT_BAUD / SERIAL_BAUD, SERIAL_PORT + SERIAL_BAUD_LSB);
  outb (0, SERIAL_PORT + SERIAL_BAUD_MSB);
  outb (0x03, SERIAL_PORT + SERIAL_LINE_CONTROL);
  outb (0xc7, SERIAL_PORT + SERIAL_INT_ID);
  outb (0x0b, SERIAL_PORT + SERIAL_MODEM_CONTROL);
  outb (0x1e, SERIAL_PORT + SERIAL_MODEM_CONTROL);
  outb (0xae, SERIAL_PORT + SERIAL_DATA);
  if (inb (SERIAL_PORT + SERIAL_DATA) != 0xae)
    printf ("serial: received wrong byte on loopback test\n");
  outb (0x0f, SERIAL_PORT + SERIAL_MODEM_CONTROL);
}
