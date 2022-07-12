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
  int action;
  va_start (args, req);

  tty = tty_from_fd (fd);
  if (!tty)
    GOTO_ERROR (ENOTTY, err);

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
    case TCSBRK:
    case TCSBRKP:
    case TIOCSBRK:
    case TIOCCBRK:
    case TCXONC:
      GOTO_ERROR (ENOSYS, err);
      /* TODO Implement remaining ioctls */
    case TIOCINQ:
      *va_arg (args, int *) = tty->input.end - tty->input.start;
      break;
    case TIOCOUTQ:
      *va_arg (args, int *) = 0;
      break;
    case TCFLSH:
      action = va_arg (args, int);
      switch (action)
	{
	case TCIFLUSH:
	  tty->input.start = tty->input.end = 0;
	  break;
	default:
	  GOTO_ERROR (ENOSYS, err);
	}
      break;
    case TIOCSTI:
      tty_recv (tty, *va_arg (args, const char *));
      break;
    case TIOCCONS:
    case TIOCSCTTY:
    case TIOCNOTTY:
      GOTO_ERROR (ENOSYS, err);
    case TIOCGPGRP:
      *va_arg (args, pid_t *) = tty->pgid;
      break;
    case TIOCSPGRP:
      tty->pgid = *va_arg (args, const pid_t *);
      break;
    case TIOCGSID:
      *va_arg (args, pid_t *) = tty->sid;
      break;
    case TIOCEXCL:
      tty->flags |= TTY_FLAG_EXCL;
      break;
    case TIOCGEXCL:
      *va_arg (args, int *) = !!(tty->flags & TTY_FLAG_EXCL);
      break;
    case TIOCNXCL:
      tty->flags &= ~TTY_FLAG_EXCL;
      break;
    case TIOCGETD:
    case TIOCSETD:
      GOTO_ERROR (ENOSYS, err);
    default:
      GOTO_ERROR (EINVAL, err);
    }
  va_end (args);
  return 0;

 err:
  va_end (args);
  return -1;
}
