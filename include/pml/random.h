/* random.h -- This file is part of PML.
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

#ifndef __PML_RANDOM_H
#define __PML_RANDOM_H

#define GRND_RANDOM             (1 << 0)
#define GRND_NONBLOCK           (1 << 1)

#ifdef PML_KERNEL

#include <pml/cdefs.h>

__BEGIN_DECLS

void add_entropy (const void *data, size_t len);
void get_entropy (void *data, size_t len);
void random_init (void);

__END_DECLS

#endif

#endif
