/* vt100.h -- This file is part of PML.
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

#ifndef __PML_VT100_H
#define __PML_VT100_H

/*!
 * @file
 * @brief VT100 terminal emulator definitions
 */

#include <pml/tty.h>

/*! VT100 character sets */

enum vt100_char_set
{
  VT100_G0_UK,
  VT100_G1_UK,
  VT100_G0_US,
  VT100_G1_US,
  VT100_G0_SPEC,
  VT100_G1_SPEC,
  VT100_G0_ALT,
  VT100_G1_ALT,
  VT100_G0_ALT_SPEC,
  VT100_G1_ALT_SPEC
};

/*! 
 * VT100 escape sequence states. These constants are used to store a
 * sequence of escape characters currently being parsed.
 */

enum vt100_state
{
  VT100_STATE_ESC = 1,          /*!< @c ^[ sequence */
  VT100_STATE_CSI,              /*!< @c ^[[ sequence */
  VT100_STATE_LP,               /*!< @c ^[( sequence */
  VT100_STATE_RP,               /*!< @c ^[) sequence */
  VT100_STATE_BQ                /*!< @c ^[[? sequence */
};

__BEGIN_DECLS

int vt100_emu_handle (struct tty *tty, unsigned char c);

__END_DECLS

#endif
