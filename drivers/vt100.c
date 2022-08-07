/* vt100.c -- This file is part of PML.
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

#include <pml/vgatext.h>
#include <pml/vt100.h>
#include <ctype.h>
#include <stdlib.h>

static void
vt100_set_dec (struct tty *tty)
{
  tty_reset_state (tty);
}

static void
vt100_reset_dec (struct tty *tty)
{
  tty_reset_state (tty);
}

static void
vt100_set_sgr (struct tty *tty)
{
  size_t i;
  for (i = 0; i <= tty->state_curr; i++)
    {
      switch (tty->state_buf[i])
	{
	case 0:
	  tty->flags &= ~TTY_FLAG_REVERSE_VIDEO;
	  tty->color = VGA_TEXT_DEFAULT_COLOR;
	  break;
	case 7:
	  tty->flags |= TTY_FLAG_REVERSE_VIDEO;
	  break;
	case 30:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_BLACK);
	  break;
	case 31:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_RED);
	  break;
	case 32:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_GREEN);
	  break;
	case 33:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_YELLOW);
	  break;
	case 34:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_BLUE);
	  break;
	case 35:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_MAGENTA);
	  break;
	case 36:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_CYAN);
	  break;
	case 37:
	  tty->color = vga_color_set_fg (tty->color, VGA_TEXT_WHITE);
	  break;
	case 40:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_BLACK);
	  break;
	case 41:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_RED);
	  break;
	case 42:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_GREEN);
	  break;
	case 43:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_YELLOW);
	  break;
	case 44:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_BLUE);
	  break;
	case 45:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_MAGENTA);
	  break;
	case 46:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_CYAN);
	  break;
	case 47:
	  tty->color = vga_color_set_bg (tty->color, VGA_TEXT_WHITE);
	  break;
	}
    }
  tty_reset_state (tty);
}

static void
vt100_set_cursor_pos (struct tty *tty)
{
  if (tty->state_buf[0] > 0)
    tty->state_buf[0]--;
  if (tty->state_buf[0] >= VGA_TEXT_SCREEN_HEIGHT)
    tty->state_buf[0] = VGA_TEXT_SCREEN_HEIGHT - 1;
  if (tty->state_buf[1] > 0)
    tty->state_buf[1]--;
  if (tty->state_buf[1] >= VGA_TEXT_SCREEN_WIDTH)
    tty->state_buf[1] = VGA_TEXT_SCREEN_WIDTH - 1;
  tty->y = tty->state_buf[0];
  tty->x = tty->state_buf[1];
  tty->output->update_cursor (tty);
  tty_reset_state (tty);
}

static void
vt100_clear_screen (struct tty *tty)
{
  tty->output->update_screen (tty);
  tty_reset_state (tty);
}

static void
vt100_clear_line (struct tty *tty)
{
  tty->output->update_screen (tty);
  tty_reset_state (tty);
}

static void
vt100_set_charset (struct tty *tty, int set)
{
  /* TODO Implement multiple character sets */
  tty_reset_state (tty);
}

static void
vt100_handle_escaped_char (struct tty *tty, unsigned char c)
{
  switch (tty->state)
    {
    case VT100_STATE_ESC:
      switch (c)
	{
	case '[':
	  DO_RET (tty->state = VT100_STATE_CSI);
	case '(':
	  DO_RET (tty->state = VT100_STATE_LP);
	case ')':
	  DO_RET (tty->state = VT100_STATE_RP);
	case '=':
	  DO_RET (tty_set_alt_keypad (tty, 1));
	case '>':
	  DO_RET (tty_set_alt_keypad (tty, 0));
	}
      break;
    case VT100_STATE_CSI:
      switch (c)
	{
	case '?':
	  DO_RET (tty->state = VT100_STATE_BQ);
	case 'h':
	  DO_RET (vt100_set_dec (tty));
	case 'l':
	  DO_RET (vt100_reset_dec (tty));
	case 'm':
	  DO_RET (vt100_set_sgr (tty));
	case 'r':
	  DO_RET (tty_reset_state (tty));
	case 'H':
	  DO_RET (vt100_set_cursor_pos (tty));
	case 'J':
	  DO_RET (vt100_clear_screen (tty));
	case 'K':
	  DO_RET (vt100_clear_line (tty));
	case ';':
	  if (++tty->state_curr == sizeof (tty->state_buf))
	    tty_reset_state (tty); /* Too much data */
	  return;
	default:
	  if (isdigit (c))
	    DO_RET (tty_add_digit_char (tty, c));
	}
      break;
    case VT100_STATE_LP:
      switch (c)
	{
	case 'A':
	  DO_RET (vt100_set_charset (tty, VT100_G0_UK));
	case 'B':
	  DO_RET (vt100_set_charset (tty, VT100_G0_US));
	case '0':
	  DO_RET (vt100_set_charset (tty, VT100_G0_SPEC));
	case '1':
	  DO_RET (vt100_set_charset (tty, VT100_G0_ALT));
	case '2':
	  DO_RET (vt100_set_charset (tty, VT100_G0_ALT_SPEC));
	}
      break;
    case VT100_STATE_RP:
      switch (c)
	{
	case 'A':
	  DO_RET (vt100_set_charset (tty, VT100_G1_UK));
	case 'B':
	  DO_RET (vt100_set_charset (tty, VT100_G1_US));
	case '0':
	  DO_RET (vt100_set_charset (tty, VT100_G1_SPEC));
	case '1':
	  DO_RET (vt100_set_charset (tty, VT100_G1_ALT));
	case '2':
	  DO_RET (vt100_set_charset (tty, VT100_G1_ALT_SPEC));
	}
      break;
    case VT100_STATE_BQ:
      switch (c)
	{
	default:
	  if (isdigit (c))
	    DO_RET (tty_add_digit_char (tty, c));
	}
      break;
    }
  tty_reset_state (tty); /* Invalid character, stop parsing */
}

int
vt100_emu_handle (struct tty *tty, unsigned char c)
{
  if (tty->state)
    vt100_handle_escaped_char (tty, c);
  else if (c == '\33')
    tty->state = VT100_STATE_ESC;
  else
    return -1;
  return 0;
}
