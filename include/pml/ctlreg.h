/* ctlreg.h -- This file is part of PML.
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

#ifndef __PML_CTLREG_H
#define __PML_CTLREG_H

#define CR0_PE                  (1 << 0)
#define CR0_MP                  (1 << 1)
#define CR0_EM                  (1 << 2)
#define CR0_TS                  (1 << 3)
#define CR0_ET                  (1 << 4)
#define CR0_NE                  (1 << 5)
#define CR0_WP                  (1 << 16)
#define CR0_AM                  (1 << 18)
#define CR0_NW                  (1 << 29)
#define CR0_CD                  (1 << 30)
#define CR0_PG                  (1 << 31)

#define CR4_VME                 (1 << 0)
#define CR4_PVI                 (1 << 1)
#define CR4_TSD                 (1 << 2)
#define CR4_DE                  (1 << 3)
#define CR4_PSE                 (1 << 4)
#define CR4_PAE                 (1 << 5)
#define CR4_MCE                 (1 << 6)
#define CR4_PGE                 (1 << 7)
#define CR4_PCE                 (1 << 8)
#define CR4_OSFXSR              (1 << 9)
#define CR4_OSXMMEXCPT          (1 << 10)
#define CR4_UMIP                (1 << 11)
#define CR4_LA57                (1 << 12)
#define CR4_VMXE                (1 << 13)
#define CR4_SMXE                (1 << 14)
#define CR4_FSGSBASE            (1 << 16)
#define CR4_PCIDE               (1 << 17)
#define CR4_OSXSAVE             (1 << 18)
#define CR4_SMEP                (1 << 20)
#define CR4_SMAP                (1 << 21)
#define CR4_PKE                 (1 << 22)

#define EFER_SCE                (1 << 0)
#define EFER_DPE                (1 << 1)
#define EFER_SEWBED             (1 << 2)
#define EFER_GEWBED             (1 << 3)
#define EFER_L2D                (1 << 4)
#define EFER_LME                (1 << 8)
#define EFER_LMA                (1 << 10)
#define EFER_NXE                (1 << 11)
#define EFER_SVME               (1 << 12)
#define EFER_LMSLE              (1 << 13)
#define EFER_FFXSR              (1 << 14)
#define EFER_TCE                (1 << 15)

#define XCR0_X87                (1 << 0)
#define XCR0_SSE                (1 << 1)
#define XCR0_AVX                (1 << 2)
#define XCR0_BNDREG             (1 << 3)
#define XCR0_BNDCSR             (1 << 4)
#define XCR0_OPMASK             (1 << 5)
#define XCR0_ZMMHI256           (1 << 6)
#define XCR0_HI16ZMM            (1 << 7)
#define XCR0_PKRU               (1 << 9)
#define XCR0_CETUSER            (1 << 11)
#define XCR0_CETSUPV            (1 << 12)

#endif
