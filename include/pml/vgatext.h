/* vgatext.h -- This file is part of PML.
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

#ifndef __PML_VGATEXT_H
#define __PML_VGATEXT_H

#include <pml/tty.h>

#define VGA_TEXT_BUFFER         0xfffffe00000b8000
#define VGA_PORT_INDEX          0x3d4
#define VGA_PORT_DATA           0x3d5

#define VGA_TEXT_SCREEN_WIDTH   80
#define VGA_TEXT_SCREEN_HEIGHT  25
#define VGA_TEXT_SCREEN_SIZE    VGA_TEXT_SCREEN_WIDTH * VGA_TEXT_SCREEN_HEIGHT

#define VGA_TEXT_DEFAULT_FG     VGA_TEXT_LIGHT_GREY
#define VGA_TEXT_DEFAULT_BG     VGA_TEXT_BLACK
#define VGA_TEXT_DEFAULT_COLOR					\
  vga_text_color (VGA_TEXT_DEFAULT_FG, VGA_TEXT_DEFAULT_BG)

#define VGA_TEXT_DEFAULT_IFLAG  (BRKINT | ISTRIP | ICRNL | IMAXBEL | IXON \
				 | IXANY)
#define VGA_TEXT_DEFAULT_OFLAG  (OPOST | ONLCR | XTABS)
#define VGA_TEXT_DEFAULT_CFLAG  (B38400 | CREAD | CS7 | PARENB | HUPCL)
#define VGA_TEXT_DEFAULT_LFLAG  (ECHO | ICANON | ISIG | IEXTEN | ECHOE	\
				 | ECHOKE | ECHOCTL)

enum
{
  VGA_TEXT_BLACK = 0,
  VGA_TEXT_BLUE,
  VGA_TEXT_GREEN,
  VGA_TEXT_CYAN,
  VGA_TEXT_RED,
  VGA_TEXT_MAGENTA,
  VGA_TEXT_BROWN,
  VGA_TEXT_LIGHT_GREY,
  VGA_TEXT_DARK_GREY,
  VGA_TEXT_LIGHT_BLUE,
  VGA_TEXT_LIGHT_GREEN,
  VGA_TEXT_LIGHT_CYAN,
  VGA_TEXT_LIGHT_RED,
  VGA_TEXT_LIGHT_MAGENTA,
  VGA_TEXT_YELLOW,
  VGA_TEXT_WHITE
};

__always_inline static inline unsigned char
vga_text_color (unsigned char fg, unsigned char bg)
{
  return fg | bg << 4;
}

__always_inline static inline unsigned char
vga_color_set_fg (unsigned char color, unsigned char fg)
{
  return (color & 0xf0) | fg;
}

__always_inline static inline unsigned char
vga_color_set_bg (unsigned char color, unsigned char bg)
{
  return (color & 0x0f) | bg << 4;
}

__always_inline static inline uint16_t
vga_text_entry (unsigned char c, unsigned char color)
{
  return (uint16_t) c | (uint16_t) color << 8;
}

__always_inline static inline unsigned int
vga_text_index (unsigned int x, unsigned int y)
{
  return y * VGA_TEXT_SCREEN_WIDTH + x;
}

__BEGIN_DECLS

extern const struct tty_output vga_text_output;

int vga_text_write_char (struct tty *tty, size_t x, size_t y, unsigned char c);
int vga_text_clear (struct tty *tty);
int vga_text_update_cursor (struct tty *tty);
int vga_text_update_screen (struct tty *tty);
int vga_text_scroll_down (struct tty *tty);
int vga_text_erase_char (struct tty *tty);
int vga_text_erase_line (struct tty *tty, size_t len);

__END_DECLS

#endif
