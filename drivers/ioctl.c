/* ioctl.c -- This file is part of PML.
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

#include <pml/ioctl.h>
#include <pml/process.h>
#include <pml/syscall.h>
#include <pml/tty.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

int
sys_ioctl (int fd, unsigned long req, ...)
{
  va_list args;
  struct tty *tty;
  struct termios *tp;
  struct winsize *ws;
  va_start (args, req);

  tty = tty_from_fd (fd);
  if (!tty)
    RETV_ERROR (ENOTTY, -1);

  switch (req)
    {
    case TCGETS:
      tp = va_arg (args, struct termios *);
      memcpy (tp, &tty->termios, sizeof (struct termios));
      break;
    case TCSETSF:
      tty->input.start = tty->input.end = 0;
      __fallthrough;
    case TCSETS:
    case TCSETSW:
      tp = va_arg (args, struct termios *);
      memcpy (&tty->termios, tp, sizeof (struct termios));
      break;
    case TIOCGWINSZ:
      ws = va_arg (args, struct winsize *);
      ws->ws_col = tty->width;
      ws->ws_row = tty->height;
      break;
    case TIOCSWINSZ:
      {
	int sig = 0;
	ws = va_arg (args, struct winsize *);
	if (tty->width != ws->ws_col)
	  sig = 1;
	tty->width = ws->ws_col;
	if (tty->height != ws->ws_row)
	  sig = 1;
	tty->height = ws->ws_row;
	if (sig)
	  sys_killpg (tty->pgid, SIGWINCH);
	break;
      }
      /* TODO Implement remaining ioctls */
    default:
      RETV_ERROR (EINVAL, -1);
    }
  return 0;
}
