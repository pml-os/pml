/* cpuid.h -- This file is part of PML.
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

#ifndef __PML_CPUID_H
#define __PML_CPUID_H

/* Page 1, EDX */

#define CPUID_FPU               (1 << 0)
#define CPUID_VME               (1 << 1)
#define CPUID_DE                (1 << 2)
#define CPUID_PSE               (1 << 3)
#define CPUID_TSC               (1 << 4)
#define CPUID_MSR               (1 << 5)
#define CPUID_PAE               (1 << 6)
#define CPUID_MCE               (1 << 7)
#define CPUID_CX8               (1 << 8)
#define CPUID_APIC              (1 << 9)
#define CPUID_SEP               (1 << 11)
#define CPUID_MTRR              (1 << 12)
#define CPUID_PGE               (1 << 13)
#define CPUID_MCA               (1 << 14)
#define CPUID_CMOV              (1 << 15)
#define CPUID_PAT               (1 << 16)
#define CPUID_PSE36             (1 << 17)
#define CPUID_PSN               (1 << 18)
#define CPUID_CLFSH             (1 << 19)
#define CPUID_DS                (1 << 21)
#define CPUID_ACPI              (1 << 22)
#define CPUID_MMX               (1 << 23)
#define CPUID_FXSR              (1 << 24)
#define CPUID_SSE               (1 << 25)
#define CPUID_SSE2              (1 << 26)
#define CPUID_SS                (1 << 27)
#define CPUID_HTT               (1 << 28)
#define CPUID_TM                (1 << 29)
#define CPUID_IA64              (1 << 30)
#define CPUID_PBE               (1 << 31)

/* Page 1, ECX */

#define CPUID_SSE3              (1 << 0)
#define CPUID_PCLMULQDQ         (1 << 1)
#define CPUID_DTES64            (1 << 2)
#define CPUID_MONITOR           (1 << 3)
#define CPUID_DSCPL             (1 << 4)
#define CPUID_VMX               (1 << 5)
#define CPUID_SMX               (1 << 6)
#define CPUID_EST               (1 << 7)
#define CPUID_TM2               (1 << 8)
#define CPUID_SSSE3             (1 << 9)
#define CPUID_CNXTID            (1 << 10)
#define CPUID_SDBG              (1 << 11)
#define CPUID_FMA               (1 << 12)
#define CPUID_CX16              (1 << 13)
#define CPUID_XTPR              (1 << 14)
#define CPUID_PDCM              (1 << 15)
#define CPUID_PCID              (1 << 17)
#define CPUID_DCA               (1 << 18)
#define CPUID_SSE41             (1 << 19)
#define CPUID_SSE42             (1 << 20)
#define CPUID_X2APIC            (1 << 21)
#define CPUID_MOVBE             (1 << 22)
#define CPUID_POPCNT            (1 << 23)
#define CPUID_TSCDEADLINE       (1 << 24)
#define CPUID_AES               (1 << 25)
#define CPUID_XSAVE             (1 << 26)
#define CPUID_OSXSAVE           (1 << 27)
#define CPUID_AVX               (1 << 28)
#define CPUID_F16C              (1 << 29)
#define CPUID_RDRND             (1 << 30)
#define CPUID_HYPERVISOR        (1 << 31)

#endif
