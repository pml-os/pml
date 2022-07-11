/* ioctl.h -- This file is part of PML.
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

#ifndef __PML_IOCTL_H
#define __PML_IOCTL_H

#define TCGETS                  0x8000
#define TCSETS                  0x8001
#define TCSETSW                 0x8002
#define TCSETSF                 0x8003
#define TIOCGWINSZ              0x8004
#define TIOCSWINSZ              0x8005
#define TCXONC                  0x8006
#define TIOCINQ                 0x8007
#define FIONREAD                TIOCINQ
#define TIOCOUTQ                0x8008
#define TCFLSH                  0x8009
#define TIOCSTI                 0x800a
#define TIOCCONS                0x800b
#define TIOCSCTTY               0x800c
#define TIOCNOTTY               0x800d
#define TIOCGPGRP               0x800e
#define TIOCSPGRP               0x800f
#define TIOCGSID                0x8010
#define TIOCEXCL                0x8011
#define TIOCGEXCL               0x8012
#define TIOCNXCL                0x8013
#define TIOCGETD                0x8014
#define TIOCSETD                0x8015

struct winsize
{
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
};

#endif
