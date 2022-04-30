/* vgatext.c -- This file is part of PML.
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

#include <pml/io.h>
#include <pml/vgatext.h>
#include <string.h>

static uint16_t *vga_text_buffer = (uint16_t *) VGA_TEXT_BUFFER;
static uint16_t kernel_tty_screen[VGA_TEXT_SCREEN_SIZE];

const struct tty_output vga_text_output = {
  .write_char = vga_text_write_char,
  .clear = vga_text_clear,
  .update_cursor = vga_text_update_cursor,
  .scroll_down = vga_text_scroll_down
};

int
vga_text_write_char (struct tty *tty, size_t x, size_t y, unsigned char c)
{
  unsigned int index = vga_text_index (x, y);
  uint16_t entry = vga_text_entry (c, tty->color);
  ((uint16_t *) tty->screen)[index] = entry;
  if (tty == current_tty)
    vga_text_buffer[index] = entry;
  return 0;
}

int
vga_text_clear (struct tty *tty)
{
  uint16_t *buffer = tty->screen;
  size_t i;
  for (i = 0; i < VGA_TEXT_SCREEN_SIZE; i++)
    buffer[i] = vga_text_entry (' ', tty->color);
  if (tty == current_tty)
    memcpy (vga_text_buffer, tty->screen, VGA_TEXT_SCREEN_SIZE * 2);
  return 0;
}

int
vga_text_update_cursor (struct tty *tty)
{
  uint16_t pos = vga_text_index (tty->x, tty->y);
  outb (0x0f, VGA_PORT_INDEX);
  outb (pos & 0xff, VGA_PORT_DATA);
  outb (0x0e, VGA_PORT_INDEX);
  outb (pos >> 8, VGA_PORT_DATA);
  return 0;
}

int
vga_text_scroll_down (struct tty *tty)
{
  size_t i;
  memmove (current_tty->screen, current_tty->screen + VGA_TEXT_SCREEN_WIDTH * 2,
	   (VGA_TEXT_SCREEN_SIZE - VGA_TEXT_SCREEN_WIDTH) * 2);
  for (i = 0; i < VGA_TEXT_SCREEN_WIDTH; i++)
    tty->output->write_char (tty, i, VGA_TEXT_SCREEN_HEIGHT - 1, ' ');
  if (tty == current_tty)
    memcpy (vga_text_buffer, tty->screen, VGA_TEXT_SCREEN_SIZE * 2);
  return 0;
}

void
vga_text_init (void)
{
  current_tty->color = VGA_TEXT_DEFAULT_COLOR;
  current_tty->width = VGA_TEXT_SCREEN_WIDTH;
  current_tty->height = VGA_TEXT_SCREEN_HEIGHT;
  current_tty->screen = kernel_tty_screen;
  current_tty->output = &vga_text_output;
  current_tty->output->clear (current_tty);
  current_tty->termios.c_iflag = VGA_TEXT_DEFAULT_IFLAG;
  current_tty->termios.c_oflag = VGA_TEXT_DEFAULT_OFLAG;
  current_tty->termios.c_cflag = VGA_TEXT_DEFAULT_CFLAG;
  current_tty->termios.c_lflag = VGA_TEXT_DEFAULT_LFLAG;
  current_tty->termios.c_cc[VINTR] = '\3';
  current_tty->termios.c_cc[VQUIT] = '\34';
  current_tty->termios.c_cc[VERASE] = '\37';
  current_tty->termios.c_cc[VKILL] = '\25';
  current_tty->termios.c_cc[VEOF] = '\4';
  current_tty->termios.c_cc[VTIME] = 0;
  current_tty->termios.c_cc[VMIN] = 1;
  current_tty->termios.c_cc[VSTART] = '\21';
  current_tty->termios.c_cc[VSTOP] = '\23';
  current_tty->termios.c_cc[VSUSP] = '\32';
  current_tty->termios.c_cc[VEOL] = 0xff;
  current_tty->termios.c_cc[VREPRINT] = '\22';
  current_tty->termios.c_cc[VDISCARD] = '\17';
  current_tty->termios.c_cc[VWERASE] = '\27';
  current_tty->termios.c_cc[VLNEXT] = '\26';
  current_tty->termios.c_cc[VEOL2] = 0xff;
}
