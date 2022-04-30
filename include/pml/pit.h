/* pit.h -- This file is part of PML.
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

#ifndef __PML_PIT_H
#define __PML_PIT_H

#include <pml/cdefs.h>

#define PIT_BASE_FREQ           1193182

#define PIT_ACC_LATCH_COUNT     0
#define PIT_ACC_LOW             1
#define PIT_ACC_HIGH            2
#define PIT_ACC_LOW_HIGH        3

#define PIT_MODE_INT_COUNT      0
#define PIT_MODE_HW_ONE_SHOT    1
#define PIT_MODE_RATE           2
#define PIT_MODE_SQUARE_WAVE    3
#define PIT_MODE_SW_STROBE      4
#define PIT_MODE_HW_STROBE      5

#define PIT_PORT_CHANNEL(x)     ((x) + 0x40)
#define PIT_PORT_COMMAND        0x43
#define PIT_PORT_PCSPK          0x61

#define PCSPK_BEEP_DURATION     50

static inline unsigned char
pit_command_byte (unsigned char channel, unsigned char acc, unsigned char mode)
{
  return ((channel & 3) << 6) | ((acc & 3) << 4) | ((mode & 7) << 1);
}

__BEGIN_DECLS

void pit_set_freq (unsigned char channel, unsigned int freq);
void pit_sleep (unsigned long ms);

void pcspk_on (unsigned int freq);
void pcspk_off (void);
void pcspk_beep (unsigned int freq);

__END_DECLS

#endif
