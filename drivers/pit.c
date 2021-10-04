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

/*! @file */

#include <pml/interrupt.h>
#include <pml/io.h>
#include <pml/pit.h>

static volatile unsigned long pit_ticks;

/*!
 * Sets the frequency of a PIT timer channel.
 *
 * @param channel the PIT channel number
 * @param freq the frequency of the timer, in hertz
 */

void
pit_set_freq (unsigned char channel, unsigned int freq)
{
  uint16_t div = PIT_BASE_FREQ / freq;
  outb (pit_command_byte (channel, PIT_ACC_LOW_HIGH, PIT_MODE_SQUARE_WAVE),
	PIT_PORT_COMMAND);
  outb (div & 0xff, PIT_PORT_CHANNEL (channel));
  outb (div >> 8, PIT_PORT_CHANNEL (channel));
}

/*!
 * Suspends execution of the current thread.
 *
 * @param ms milliseconds to suspend
 */

void
pit_sleep (unsigned long ms)
{
  unsigned long start = pit_ticks;
  while (pit_ticks < start + ms)
    ;
}

void
int_pit_tick (void)
{
  pit_ticks++;
  EOI (0);
}
