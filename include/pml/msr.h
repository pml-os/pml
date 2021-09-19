/* msr.h -- This file is part of PML.
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

#ifndef __PML_MSR_H
#define __PML_MSR_H

#define MSR_EFER                0xc0000080
#define MSR_STAR                0xc0000081
#define MSR_LSTAR               0xc0000082
#define MSR_CSTAR               0xc0000083
#define MSR_SFMASK              0xc0000084
#define MSR_FSBASE              0xc0000100

#ifndef __ASSEMBLER__

#include <pml/cdefs.h>

__always_inline static inline void
msr_read (uint32_t msr, uint32_t *low, uint32_t *high)
{
  __asm__ volatile ("rdmsr" : "=a" (*low), "=d" (*high) : "c" (msr));
}

__always_inline static inline void
msr_write (uint32_t msr, uint32_t low, uint32_t high)
{
  __asm__ volatile ("wrmsr" :: "a" (low), "d" (high), "c" (msr));
}

#endif /* !__ASSEMBLER__ */

#endif
