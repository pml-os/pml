/* pcspk.c -- This file is part of PML.
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
#include <pml/pit.h>

void
pcspk_on (unsigned int freq)
{
  pit_set_freq (2, 440);
  outb (inb (PIT_PORT_PCSPK) | PIT_MODE_SQUARE_WAVE, PIT_PORT_PCSPK);
}

void
pcspk_off (void)
{
  outb (inb (PIT_PORT_PCSPK) & ~PIT_MODE_SQUARE_WAVE, PIT_PORT_PCSPK);
}

void
pcspk_beep (void)
{
  pcspk_on (PCSPK_BEEP_FREQ);
  pit_sleep (PCSPK_BEEP_DURATION);
  pcspk_off ();
}
