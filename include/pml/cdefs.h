/* cdefs.h -- This file is part of PML.
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

#ifndef __PML_CDEFS_H
#define __PML_CDEFS_H

#include <stddef.h>
#include <stdint.h>

#define __always_inline         __attribute__ ((always_inline))
#define __fallthrough           __attribute__ ((fallthrough))
#define __packed                __attribute__ ((packed))
#define __noreturn              __attribute__ ((noreturn))
#define __hot                   __attribute__ ((hot))
#define __cold                  __attribute__ ((cold))
#define __pure                  __attribute__ ((pure))
#define __alias(x)              __attribute__ ((alias (#x)))

#ifdef __cplusplus
#define __BEGIN_DECLS           extern "C" {
#define __END_DECLS             }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#endif
