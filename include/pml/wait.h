/* wait.h -- This file is part of PML.
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

#ifndef __PML_WAIT_H
#define __PML_WAIT_H

/*!
 * @file
 * @brief Process wait definitions
 */

#include <pml/resource.h>

#define WNOHANG                 1
#define WUNTRACED               2

#define WIFEXITED(x)            (!((x) & 0xff))
#define WIFSIGNALED(x)          (((x) & 0x7f) > 0 && (((x) & 0x7f) < 0x7f))
#define WIFSTOPPED(x)           (((x) & 0xff) == 0x7f)
#define WEXITSTATUS(x)          (((x) >> 8) & 0xff)
#define WTERMSIG(x)             (((x) >> 8) & 0x7f)
#define WSTOPSIG(x)             WEXITSTATUS (x)
#define WCOREDUMP(x)            ((x) & 0x80)

#endif
