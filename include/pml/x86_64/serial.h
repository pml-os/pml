/* serial.h -- This file is part of PML.
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

#ifndef __PML_SERIAL_H
#define __PML_SERIAL_H

/*!
 * @file
 * IBM-PC serial port communication
 */

#include <pml/tty.h>

#define SERIAL_COM1             0x3f8
#define SERIAL_COM2             0x2f8
#define SERIAL_COM3             0x3e8
#define SERIAL_COM4             0x2e8

#define SERIAL_DATA             0
#define SERIAL_BAUD_LSB         0
#define SERIAL_INT_ENABLE       1
#define SERIAL_BAUD_MSB         1
#define SERIAL_INT_ID           2
#define SERIAL_LINE_CONTROL     3
#define SERIAL_MODEM_CONTROL    4
#define SERIAL_LINE_STATUS      5
#define SERIAL_MODEM_STATUS     6
#define SERIAL_SCRATCH          7

#define SERIAL_DLAB             0x80
#define SERIAL_DEFAULT_BAUD     115200
#define SERIAL_BAUD             38400

__BEGIN_DECLS

extern const struct tty_output serial_output;

int serial_write_char (struct tty *tty, size_t x, size_t y, unsigned char c);
int serial_clear (struct tty *tty);
int serial_update_cursor (struct tty *tty);
int serial_scroll_down (struct tty *tty);
void serial_init (void);

__END_DECLS

#endif
