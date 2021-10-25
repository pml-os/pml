/* kbd.h -- This file is part of PML.
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

#ifndef __PML_KBD_H
#define __PML_KBD_H

/*!
 * @file
 * @brief Keyboard driver definitions
 */

#include <pml/tty.h>

#define PS2KBD_PORT_DATA        0x60
#define PS2KBD_PORT_STATUS      0x64
#define PS2KBD_PORT_COMMAND     0x64

#define PS2_KSC_ESC             0x01
#define PS2_KSC_BACKSP          0x0e
#define PS2_KSC_ENTER           0x1c
#define PS2_KSC_LCTRL           0x1d
#define PS2_KSC_LSHIFT          0x2a
#define PS2_KSC_RSHIFT          0x36
#define PS2_KSC_LALT            0x38
#define PS2_KSC_CAPSLK          0x3a
#define PS2_KSC_NUMLK           0x45
#define PS2_KSC_SCRLLK          0x46
#define PS2_KSC_EXTENDED        0xe0

/*! Map of PS/2 keyboard scancodes to characters on a US QWERTY keyboard */
#define PS2_QWERTY_LAYOUT						\
  {									\
    0, '\33', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', \
    '\37', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', \
    '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', \
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				\
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'	\
  }

/*! Map of normal characters to shift characters on a US QWERTY keyboard */
#define PS2_QWERTY_SHIFTMAP						\
  {									\
    ['-'] = '_',							\
    ['='] = '+',							\
    [','] = '<',							\
    ['.'] = '>',							\
    ['/'] = '?',							\
    [';'] = ':',							\
    ['\''] = '"',							\
    ['['] = '{',							\
    [']'] = '}',							\
    ['\\'] = '|',							\
    ['`'] = '~',							\
    ['0'] = ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',		\
    ['a'] = 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', \
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' \
  }

__BEGIN_DECLS

void kbd_recv_key (int scancode);

__END_DECLS

#endif
