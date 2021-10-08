/* ctype.h -- This file is part of PML.
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

#ifndef __CTYPE_H
#define __CTYPE_H

/*!
 * @file
 * @brief Macros for character classification
 *
 * The macros in this file will only work properly on ASCII characters since
 * they use the numerical values of character literals.
 */

#define isascii(c)              ((c) >= 0 && (c) < 0x80)
#define isblank(c)              ((c) == ' ' || (c) == '\t')
#define iscntrl(c)              ((unsigned char) (c) < ' ' || (c) == '\177')
#define islower(c)              ((c) >= 'a' && (c) <= 'z')
#define isupper(c)              ((c) >= 'A' && (c) <= 'Z')
#define isdigit(c)              ((c) >= '0' && (c) <= '9')
#define isgraph(c)              ((c) > ' ' && (c) <= '~')
#define isprint(c)              ((c) >= ' ' && (c) <= '~')
#define isalpha(c)              (islower (c) || isupper (c))
#define isalnum(c)              (isalpha (c) || isdigit (c))
#define ispunct(c)              (isgraph (c) && !isalnum (c))
#define isspace(c)              (isblank (c) || (c) == '\n' || (c) == '\v' \
				 || (c) == '\f' || (c) == '\r')
#define isxdigit(c)             (isdigit (c) || ((c) >= 'a' && (c) <= 'f') \
				 || ((c) >= 'A' && (c) <= 'F'))

#define toascii(c) ((unsigned char) (c) & 0x7f)
#define tolower(c) (isupper (c) ? (c) + 0x20 : (c))
#define toupper(c) (islower (c) ? (c) - 0x20 : (c))

#endif
