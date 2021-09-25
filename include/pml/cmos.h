/* cmos.h -- This file is part of PML.
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

#ifndef __PML_CMOS_H
#define __PML_CMOS_H

#define CMOS_PORT_INDEX         0x70
#define CMOS_PORT_DATA          0x71

#define CMOS_REG_SECONDS        0x00
#define CMOS_REG_MINUTES        0x02
#define CMOS_REG_HOURS          0x04
#define CMOS_REG_WEEDAY         0x06
#define CMOS_REG_DAY_OF_MONTH   0x07
#define CMOS_REG_MONTH          0x08
#define CMOS_REG_YEAR           0x09
#define CMOS_REG_STATUS_A       0x0a
#define CMOS_REG_STATUS_B       0x0b
#define CMOS_REG_STATUS_C       0x0c
#define CMOS_REG_CENTURY        0x32

#define CMOS_STATUS_B_12H       (1 << 1)
#define CMOS_STATUS_B_BCD       (1 << 2)

#ifndef __ASSEMBLER__

#include <pml/io.h>
#include <pml/types.h>

static inline unsigned char
cmos_read_register (unsigned char reg)
{
  outb (reg, CMOS_PORT_INDEX);
  return inb (CMOS_PORT_DATA);
}

__BEGIN_DECLS

time_t cmos_read_real_time (void);
void cmos_enable_rtc_int (void);
void cmos_rtc_finish_irq (void);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
