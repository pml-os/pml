/* termios.h -- This file is part of PML.
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

#ifndef __PML_TERMIOS_H
#define __PML_TERMIOS_H

/*!
 * @file
 * @brief Terminal line discipline definitions
 */

#define NCCS                    32      /*!< Number of special characters */

/* Special input sequences */

#define VINTR                   0
#define VQUIT                   1
#define VERASE                  2
#define VKILL                   3
#define VEOF                    4
#define VTIME                   5
#define VMIN                    6
#define VSTART                  7
#define VSTOP                   8
#define VSUSP                   9
#define VEOL                    10
#define VREPRINT                11
#define VDISCARD                12
#define VWERASE                 13
#define VLNEXT                  14
#define VEOL2                   15

/* Input mode flags */

#define IGNBRK                  (1 << 0)    /*!< Ignore BREAK condition */
#define BRKINT                  (1 << 1)    /*!< Flush queues on BREAK */
#define IGNPAR                  (1 << 2)    /*!< Ignore parity errors */
#define PARMRK                  (1 << 3)    /*!< Input erroneous bytes */
#define INPCK                   (1 << 4)    /*!< Input parity checking */
#define ISTRIP                  (1 << 5)    /*!< Strip eighth bit */
#define INLCR                   (1 << 6)    /*!< Translate NL to CR */
#define IGNCR                   (1 << 7)    /*!< Ignore carriage returns */
#define ICRNL                   (1 << 8)    /*!< Translate CR to NL */
#define IUCLC                   (1 << 9)    /*!< Map to lowercase characters */
#define IXON                    (1 << 10)   /*!< Enable output flow control */
#define IXANY                   (1 << 11)   /*!< Typing resets stopped output */
#define IXOFF                   (1 << 12)   /*!< Enable input flow control */
#define IMAXBEL                 (1 << 13)   /*!< Ring on full input queue */
#define IUTF8                   (1 << 14)   /*!< Input is UTF-8 */

/* Output mode flags */

#define OPOST                   (1 << 0)    /*!< Enable output processing */
#define OLCUC                   (1 << 1)    /*!< Map to uppercase characters */
#define ONLCR                   (1 << 2)    /*!< Translate NL to CR-NL */
#define OCRNL                   (1 << 3)    /*!< Translate CR to NL */
#define ONOCR                   (1 << 4)    /*!< Don't output CR at column 0 */
#define ONLRET                  (1 << 5)    /*!< Don't output CR */
#define OFILL                   (1 << 6)    /*!< Send fill chars for delay */
#define OFDEL                   (1 << 7)    /*!< Fill character is ASCII DEL */

#define NLDLY                   0x0100
#define NL0                     0x0000
#define NL1                     0x0100

#define CRDLY                   0x0600
#define CR0                     0x0000
#define CR1                     0x0200
#define CR2                     0x0400
#define CR3                     0x0600

#define TABDLY                  0x1800
#define TAB0                    0x0000
#define TAB1                    0x0800
#define TAB2                    0x1000
#define TAB3                    0x1800
#define XTABS                   0x1800

#define BSDLY                   0x2000
#define BS0                     0x0000
#define BS1                     0x2000

#define VTDLY                   0x4000
#define VT0                     0x0000
#define VT1                     0x4000

#define FFDLY                   0x8000
#define FF0                     0x0000
#define FF1                     0x8000

/* Control mode flags */

#define CSTOPB                  (1 << 6)    /*!< Send two stop bits */
#define CREAD                   (1 << 7)    /*!< Enable receiver */
#define PARENB                  (1 << 8)    /*!< Parity generation/checking */
#define PARODD                  (1 << 9)    /*!< Use odd parity */
#define HUPCL                   (1 << 10)   /*!< Hangup on last process close */
#define CLOCAL                  (1 << 11)   /*!< Ignore modem control lines */

#define CBAUD                   0010017
#define CBAUDEX                 0010000
#define B0                      0000000
#define B50                     0000001
#define B75                     0000002
#define B110                    0000003
#define B134                    0000004
#define B150                    0000005
#define B200                    0000006
#define B300                    0000007
#define B600                    0000010
#define B1200                   0000011
#define B1800                   0000012
#define B2400                   0000013
#define B4800                   0000014
#define B9600                   0000015
#define B19200                  0000016
#define B38400                  0000017
#define B57600                  0010001
#define B115200                 0010002
#define B230400                 0010003
#define B460800                 0010004
#define B500000                 0010005
#define B576000                 0010006
#define B921600                 0010007
#define B1000000                0010010
#define B1152000                0010011
#define B1500000                0010012
#define B2000000                0010013
#define B2500000                0010014
#define B3000000                0010015
#define B3500000                0010016
#define B4000000                0010017

#define CSIZE                   0x0030
#define CS5                     0x0000
#define CS6                     0x0010
#define CS7                     0x0020
#define CS8                     0x0030

/* Local mode flags */

#define ISIG                    (1 << 0)    /*!< Generate signals */
#define ICANON                  (1 << 1)    /*!< Canonical mode */
#define XCASE                   (1 << 2)    /*!< Terminal is uppercase only */
#define ECHO                    (1 << 3)    /*!< Echo input characters */
#define ECHOE                   (1 << 4)    /*!< Echo ERASE and WERASE */
#define ECHOK                   (1 << 5)    /*!< Erase current line on KILL */
#define ECHONL                  (1 << 6)    /*!< Echo NL character */
#define ECHOCTL                 (1 << 7)    /*!< Echo special chars as ^X */
#define ECHOPRT                 (1 << 8)    /*!< Print chars on erase */
#define ECHOKE                  (1 << 9)    /*!< Erase chars on line on KILL */
#define DEFECHO                 (1 << 10)   /*!< Echo on process read only */
#define FLUSHO                  (1 << 11)   /*!< Output is being flushed */
#define NOFLSH                  (1 << 12)   /*!< Don't flush on signal */
#define TOSTOP                  (1 << 13)   /*!< Signal on background write */
#define PENDIN                  (1 << 14)   /*!< Reprint queue on next read */
#define IEXTEN                  (1 << 15)   /*!< Enable input processing */

/* Actions for TCXONC */

#define TCOOFF                  0
#define TCOON                   1
#define TCIOFF                  2
#define TCION                   3

/* Actions for TCFLSH */

#define TCIFLUSH                0
#define TCOFLUSH                1
#define TCIOFLUSH               2

/*! A type suitable for representing special characters */
typedef unsigned char cc_t;
/*! Represents a baud rate */
typedef unsigned int speed_t;
/*! Type for termios flags */
typedef unsigned int tcflag_t;

struct termios
{
  tcflag_t c_iflag;             /*!< Input mode flags */
  tcflag_t c_oflag;             /*!< Output mode flags */
  tcflag_t c_cflag;             /*!< Control mode flags */
  tcflag_t c_lflag;             /*!< Local mode flags */
  cc_t c_cc[NCCS];              /*!< Special character map */
};

#endif
