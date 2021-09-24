/* pit.c -- This file is part of PML.
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

#include <pml/io.h>
#include <pml/pit.h>

void
pit_set_freq (unsigned char channel, unsigned int freq)
{
  uint16_t div = PIT_BASE_FREQ / freq;
  outb (pit_command_byte (channel, PIT_ACC_LOW_HIGH, PIT_MODE_SQUARE_WAVE),
	PIT_PORT_COMMAND);
  outb (div & 0xff, PIT_PORT_CHANNEL (channel));
  outb (div >> 8, PIT_PORT_CHANNEL (channel));
}
