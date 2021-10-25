/* tty.h -- This file is part of PML.
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

#ifndef __PML_TTY_H
#define __PML_TTY_H

/*!
 * @file
 * @brief Definitions for TTY interfaces
 */

#include <pml/cdefs.h>

#define TTY_INPUT_BUFFER_SIZE   1024

struct tty;

/*! Vector of functions used by a TTY output backend. */

struct tty_output
{
  /*! Writes a character to the TTY at a specific location */
  int (*write_char) (struct tty *, size_t, size_t, unsigned char);
  /*! Clears the TTY screen */
  int (*clear) (struct tty *);
  /*! Updates the position of the cursor, if supported */
  int (*update_cursor) (struct tty *);
  /*! Scrolls down one line */
  int (*scroll_down) (struct tty *);
};

/*! Buffer structure used to store data sent as input to a terminal. */

struct tty_input
{
  unsigned char buffer[TTY_INPUT_BUFFER_SIZE]; /*!< Buffer of bytes */
  size_t start;                 /*!< Index of start of unread data */
  size_t end;                   /*!< Index of last byte written */
  size_t size;                  /*!< Number of unread bytes */
};

/*!
 * Represents a teletypewriter (TTY). Used for displaying output to
 * the VGA text mode console or a terminal-like device.
 */

struct tty
{
  unsigned char color;              /*!< Currently selected color */
  size_t width;                     /*!< Total number of columns */
  size_t height;                    /*!< Total number of rows */
  size_t x;                         /*!< Current column number */
  size_t y;                         /*!< Current row number */
  void *screen;                     /*!< Buffer containing contents of screen */
  struct tty_input input;           /*!< Terminal input buffer */
  const struct tty_output *output;  /*!< Output function vector */
};

__BEGIN_DECLS

extern struct tty kernel_tty;
extern struct tty *current_tty;

void tty_device_init (void);
int tty_putchar (struct tty *tty, int c);
void tty_recv (struct tty *tty, unsigned char c);

__END_DECLS

#endif
