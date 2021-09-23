/* asm.h -- This file is part of PML.
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

#ifndef __PML_ASM_H
#define __PML_ASM_H

#ifdef HAVE_CONFIG_H
#include <pml-config.h>
#endif

#define ASM_FUNC_BEGIN(name)    .type name, @function; name
#define ASM_FUNC_END(name)      .size name, . - name

#endif
