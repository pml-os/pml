/* hpet.h -- This file is part of PML.
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

#ifndef __PML_HPET_H
#define __PML_HPET_H

#include <pml/cdefs.h>
#include <pml/types.h>

#define HPET_REG_CAP_ID             0x000
#define HPET_REG_CONFIG             0x010
#define HPET_REG_INT_STATUS         0x020
#define HPET_REG_COUNTER_VALUE      0x0f0
#define HPET_REG_TIMER_CONFIG(x)    (0x100 + 0x20 * (x))
#define HPET_REG_TIMER_VALUE(x)     (0x108 + 0x20 * (x))
#define HPET_REG_TIMER_INT_ROUTE(x) (0x110 + 0x20 * (x))
#define HPET_REG(x)                 (*((uint64_t *) (hpet_addr + (x))))

#define HPET_CAP_CLOCK_PERIOD(x)    ((x) >> 32)
#define HPET_CAP_VENDOR_ID(x)       (((x) >> 16) & 0xffff)
#define HPET_CAP_LEGACY_RT(x)       (!!((x) & (1 << 15)))
#define HPET_CAP_COUNT_SIZE(x)      (!!((x) & (1 << 13)))
#define HPET_CAP_TIMER_COUNT(x)     ((x >> 8) & 0xf)
#define HPET_CAP_REVISION(x)        ((x) & 0xff)

#define HPET_CONFIG_LEGACY_RT       (1 << 1)
#define HPET_CONFIG_ENABLE          (1 << 0)

#define HPET_TIMER_CAN_ROUTE(x, n)  (((x) >> 32) & (1 << (n)))
#define HPET_TIMER_SUP_FSB_INT(x)   (!!((x) & (1 << 15)))
#define HPET_TIMER_USE_FSB_INT(x)   (!!((x) & (1 << 14)))
#define HPET_TIMER_INT_ROUTE(x)     (((x) & 0x1f) << 9)
#define HPET_TIMER_32BIT            (1 << 8)
#define HPET_TIMER_VAL_SET          (1 << 6)
#define HPET_TIMER_SIZE             (1 << 5)
#define HPET_TIMER_SUP_PERIODIC     (1 << 4)
#define HPET_TIMER_USE_PERIODIC     (1 << 3)
#define HPET_TIMER_INT_ENABLE       (1 << 2)
#define HPET_TIMER_LEVEL_TRIGGER    (1 << 1)

__BEGIN_DECLS

extern uintptr_t hpet_addr;
extern int hpet_active;
extern unsigned long hpet_resolution;

clock_t hpet_nanotime (void);

__END_DECLS

#endif
