/* ps2kbd.c -- This file is part of PML.
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

/*! @file */

#include <pml/kbd.h>
#include <stdio.h>

/*! Whether a shift key is being held */
#define SHIFT_DOWN (kbd_pressed[PS2_KSC_LSHIFT] || kbd_pressed[PS2_KSC_RSHIFT])

/*! Whether a control key is being held */
#define CTRL_DOWN (kbd_pressed[PS2_KSC_LCTRL])

/*! The scancode mapping corresponding to a standard US QWERTY keyboard */
static char kbd_layout[128] = PS2_QWERTY_LAYOUT;

/*! Character map of shift characters on a US QWERTY keyboard */
static char kbd_shiftmap[128] = PS2_QWERTY_SHIFTMAP;

/*! Map of ESC character sequences to ASCII control characters */

static char kbd_ctrl_kmap[128] = {
  [' '] = 000,
  ['a'] = 001, 002, 003, 004, 005, 006, 007, 010, 011, 012, 013, 014,
  015, 016, 017, 020, 021, 022, 023, 024, 025, 026, 027, 030, 031, 032,
  ['['] = 033, 034, 035,
  ['~'] = 036,
  ['?'] = 037
};

/*!
 * Array of boolean values representing whether the key matching a scancode
 * is currently pressed.
 */

static unsigned char kbd_pressed[128];

/*! Set if the 0xe0 scancode was just read */
static int key_extended;

void
kbd_recv_key (int scancode)
{
  if (key_extended)
    {
      /* Currently we ignore all extended scancodes */
      key_extended = 0;
      return;
    }
  if (scancode == PS2_KSC_EXTENDED)
    {
      key_extended = 1;
      return;
    }
  if (scancode > 0x80)
    {
      kbd_pressed[scancode - 0x80] = 0;
      return;
    }
  kbd_pressed[scancode] = 1;

  if (kbd_layout[scancode])
    {
      unsigned char c = kbd_layout[scancode];
      if (SHIFT_DOWN && kbd_shiftmap[c])
	c = kbd_shiftmap[c];
      if (CTRL_DOWN && kbd_ctrl_kmap[c])
	c = kbd_ctrl_kmap[c];
      tty_recv (&kernel_tty, c);
      putchar (c);
    }
}
