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
#include <pml/vt100.h>
#include <string.h>

static uint16_t *vga_text_buffer = (uint16_t *) VGA_TEXT_BUFFER;
static uint16_t kernel_tty_screen[VGA_TEXT_SCREEN_SIZE];
static char kernel_tty_tabs[VGA_TEXT_SCREEN_SIZE];

static unsigned int vga_erase_max_chars[] = {
  [TTY_TC_CHAR] = 1,
  [TTY_TC_TAB] = 8,
  [TTY_TC_CONTROL] = 2
};

const struct tty_output vga_text_output = {
  .write_char = vga_text_write_char,
  .write_tab = vga_text_write_tab,
  .write_control = vga_text_write_control,
  .clear = vga_text_clear,
  .update_cursor = vga_text_update_cursor,
  .update_screen = vga_text_update_screen,
  .scroll_down = vga_text_scroll_down,
  .erase_char = vga_text_erase_char,
  .erase_line = vga_text_erase_line
};

static int
vga_text_write_one (struct tty *tty, size_t x, size_t y, unsigned char c)
{
  unsigned int index = vga_text_index (x, y);
  uint16_t entry = vga_text_entry (c, tty->color);
  ((uint16_t *) tty->screen)[index] = entry;
  if (tty == current_tty)
    vga_text_buffer[index] = entry;
  return 0;
}

static int
vga_text_erase_one (struct tty *tty)
{
  if (!tty->x--)
    {
      if (tty->y)
	{
	  tty->x = tty->width - 1;
	  tty->y--;
	}
      else
	{
	  tty->x++;
	  return -1;
	}
    }
  vga_text_write_one (tty, tty->x, tty->y, ' ');
  return 0;
}

int
vga_text_write_char (struct tty *tty, size_t x, size_t y, unsigned char c)
{
  unsigned int index = vga_text_index (x, y);
  vga_text_write_one (tty, x, y, c);
  ((char *) tty->tabs)[index] = TTY_TC_CHAR;
  return 0;
}

int
vga_text_write_tab (struct tty *tty)
{
  unsigned int index = vga_text_index (tty->x, tty->y);
  uint16_t entry = vga_text_entry (' ', tty->color);
  size_t i;
  for (i = tty->x; i <= (tty->x | 7); i++, index++)
    {
      ((uint16_t *) tty->screen)[index] = entry;
      if (tty == current_tty)
	vga_text_buffer[index] = entry;
      ((char *) tty->tabs)[index] = TTY_TC_TAB;
    }
  tty->x |= 7;
  return 0;
}

int
vga_text_write_control (struct tty *tty, unsigned char c)
{
  unsigned int index;
  char *tabs = tty->tabs;
  tty_putchar (tty, '^');
  tty_putchar (tty, c + 0x40);
  index = vga_text_index (tty->x, tty->y);
  tabs[index - 2] = tabs[index - 1] = TTY_TC_CONTROL;
  return 0;
}

int
vga_text_clear (struct tty *tty)
{
  uint16_t *buffer = tty->screen;
  size_t i;
  for (i = 0; i < VGA_TEXT_SCREEN_SIZE; i++)
    buffer[i] = vga_text_entry (' ', tty->color);
  return vga_text_update_screen (tty);
}

int
vga_text_update_cursor (struct tty *tty)
{
  uint16_t pos = vga_text_index (tty->x, tty->y + 1);
  outb (0x0f, VGA_PORT_INDEX);
  outb (pos & 0xff, VGA_PORT_DATA);
  outb (0x0e, VGA_PORT_INDEX);
  outb (pos >> 8, VGA_PORT_DATA);
  return 0;
}

int
vga_text_update_screen (struct tty *tty)
{
  if (tty == current_tty)
    memcpy (vga_text_buffer, tty->screen, VGA_TEXT_SCREEN_SIZE * 2);
  return 0;
}

int
vga_text_scroll_down (struct tty *tty)
{
  size_t i;
  memmove (current_tty->screen, current_tty->screen + current_tty->width * 2,
	   current_tty->width * (current_tty->height - 1) * 2);
  memmove (current_tty->tabs, current_tty->tabs + current_tty->width,
	   current_tty->width * (current_tty->height - 1));
  for (i = 0; i < current_tty->width; i++)
    tty->output->write_char (tty, i, current_tty->height - 1, ' ');
  memset (current_tty->tabs + current_tty->width * (current_tty->height - 1),
	  TTY_TC_CHAR, current_tty->width);
  return vga_text_update_screen (tty);
}

int
vga_text_erase_char (struct tty *tty)
{
  unsigned int index;
  unsigned int i = 1;
  char tab;
  char *tabs = tty->tabs;
  if (vga_text_erase_one (tty))
    return 0;
  index = vga_text_index (tty->x, tty->y);
  tab = tabs[index];
  while (index && i < vga_erase_max_chars[(int) tab] && tabs[index - 1] == tab)
    {
      tabs[index - 1] = TTY_TC_CHAR;
      if (vga_text_erase_one (tty))
        break;
      index--;
      i++;
    }
  vga_text_update_cursor (tty);
  return 0;
}

int
vga_text_erase_line (struct tty *tty, size_t len)
{
  size_t i;
  while (1)
    {
      size_t to_erase = len > tty->x ? tty->x : len;
      for (i = 0; i < to_erase; i++)
	vga_text_write_char (tty, --tty->x, tty->y, ' ');
      len -= to_erase;
      if (!len || !tty->y)
	break;
      tty->y--;
      tty->x = tty->width;
    }
  vga_text_update_cursor (tty);
  return 0;
}

void
vga_text_init (void)
{
  /* Enable text mode cursor */
  outb (0x0a, VGA_PORT_INDEX);
  outb (inb (VGA_PORT_DATA) & ~0x20, VGA_PORT_DATA);

  /* Setup TTY parameters */
  current_tty->color = VGA_TEXT_DEFAULT_COLOR;
  current_tty->width = VGA_TEXT_SCREEN_WIDTH;
  current_tty->height = VGA_TEXT_SCREEN_HEIGHT;
  current_tty->screen = kernel_tty_screen;
  current_tty->tabs = kernel_tty_tabs;
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
  current_tty->emu_handle = vt100_emu_handle; /* Default terminal is VT100 */
}
