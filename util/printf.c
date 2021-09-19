/* printf.c -- This file is part of PML.
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define INTEGER_BUFFER_SIZE     32

#define PRINTF_FLAG_PAD_ZERO    (1 << 0)
#define PRINTF_FLAG_LEFT        (1 << 1)
#define PRINTF_FLAG_PLUS        (1 << 2)
#define PRINTF_FLAG_SPACE       (1 << 3)
#define PRINTF_FLAG_SPECIAL     (1 << 4)
#define PRINTF_FLAG_UPCASE      (1 << 5)
#define PRINTF_FLAG_CHAR        (1 << 6)
#define PRINTF_FLAG_SHORT       (1 << 7)
#define PRINTF_FLAG_LONG        (1 << 8)
#define PRINTF_FLAG_PREC        (1 << 9)

typedef void (*out_func_t) (char, char *, size_t, size_t maxlen);

static inline void
write_buffer (char c, char *buffer, size_t index, size_t maxlen)
{
  if (index < maxlen)
    buffer[index] = c;
}

static inline void
write_tty (char c, char *buffer, size_t index, size_t maxlen)
{
  if (c)
    putchar (c);
}

static unsigned int
parse_fmt_number (const char **str)
{
  unsigned int n = 0;
  while (isdigit (**str))
    n = n * 10 + *(*str)++ - '0';
  return n;
}

static size_t
print_pad_string (out_func_t func, char *buffer, size_t index, size_t maxlen,
		  const char *input, size_t len, unsigned int width,
		  unsigned int flags)
{
  size_t start = index;
  if (!(flags & PRINTF_FLAG_LEFT) && !(flags & PRINTF_FLAG_PAD_ZERO))
    {
      size_t i;
      for (i = len; i < width; i++)
	func (' ', buffer, index++, maxlen);
    }
  while (len > 0)
    func (input[--len], buffer, index++, maxlen);
  if (flags & PRINTF_FLAG_LEFT)
    {
      while (index - start < width)
	func (' ', buffer, index++, maxlen);
    }
  return index;
}

static size_t
print_integer (out_func_t func, char *buffer, size_t index, size_t maxlen,
	       char *input, size_t len, int negative, unsigned int base,
	       unsigned int prec, unsigned int width, unsigned int flags)
{
  if (!(flags & PRINTF_FLAG_LEFT))
    {
      if (width && (flags & PRINTF_FLAG_PAD_ZERO)
	  && (negative || (flags & (PRINTF_FLAG_PLUS | PRINTF_FLAG_SPACE))))
	width--;
      while (len < prec && len < INTEGER_BUFFER_SIZE)
	input[len++] = '0';
      while ((flags & PRINTF_FLAG_PAD_ZERO) && len < width
	     && len < INTEGER_BUFFER_SIZE)
	input[len++] = '0';
    }

  if (flags & PRINTF_FLAG_SPECIAL)
    {
      if (!(flags & PRINTF_FLAG_PREC) && len && (len == prec || len == width))
	{
	  len--;
	  if (len && base == 16)
	    len--;
	}
      if (len < INTEGER_BUFFER_SIZE)
	{
	  if (base == 16)
	    {
	      if (flags & PRINTF_FLAG_UPCASE)
		input[len++] = 'X';
	      else
		input[len++] = 'x';
	    }
	  else if (base == 2)
	    input[len++] = 'b';
	  input[len++] = '0';
	}
    }

  if (len < INTEGER_BUFFER_SIZE)
    {
      if (negative)
	input[len++] = '-';
      else if (flags & PRINTF_FLAG_PLUS)
	input[len++] = '+';
      else if (flags & PRINTF_FLAG_SPACE)
	input[len++] = ' ';
    }
  return print_pad_string (func, buffer, index, maxlen, input, len, width,
			   flags);
}

static size_t
print_long (out_func_t func, char *buffer, size_t index, size_t maxlen,
	    unsigned long value, int negative, unsigned long base,
	    unsigned int prec, unsigned int width, unsigned int flags)
{
  char input[INTEGER_BUFFER_SIZE];
  size_t len = 0;
  if (!(flags & PRINTF_FLAG_PREC) || value)
    {
      do
	{
	  char digit = value % base;
	  input[len++] = digit < 10 ? digit + '0' :
	    (flags & PRINTF_FLAG_UPCASE ? 'A' : 'a') + digit - 10;
	  value /= base;
	}
      while (value && len < INTEGER_BUFFER_SIZE);
    }
  return print_integer (func, buffer, index, maxlen, input, len, negative, base,
			prec, width, flags);
}

static size_t
print_human_size (out_func_t func, char *buffer, size_t index, size_t maxlen,
		  size_t value, unsigned int width, unsigned int flags)
{
  char input[INTEGER_BUFFER_SIZE];
  size_t len = 0;
  unsigned int i = 0;
  size_t temp;
  for (temp = value; temp / 1024 > 0; temp /= 1024)
    i++;
  if (value >> (10 * i) < 16)
    i--;
  value >>= 10 * i;
  input[len++] = ((char[]) {'B', 'K', 'M', 'G', 'T', 'P'})[i];
  do
    {
      input[len++] = value % 10 + '0';
      value /= 10;
    }
  while (value && len < INTEGER_BUFFER_SIZE);
  flags &= PRINTF_FLAG_LEFT;
  return print_pad_string (func, buffer, index, maxlen, input, len, width,
			   flags);
}

static int
print_internal (out_func_t func, char *buffer, size_t maxlen, const char *fmt,
		va_list args)
{
  size_t index = 0;
  while (*fmt)
    {
      unsigned int flags = 0;
      unsigned int width = 0;
      unsigned int prec = 0;
      unsigned int base;
      const char *str;

      if (*fmt != '%')
	{
	  func (*fmt, buffer, index++, maxlen);
	  fmt++;
	  continue;
	}
      else
	fmt++;

      while (1)
	{
	  int end = 0;
	  switch (*fmt)
	    {
	    case '0':
	      flags |= PRINTF_FLAG_PAD_ZERO;
	      break;
	    case '-':
	      flags |= PRINTF_FLAG_LEFT;
	      break;
	    case '+':
	      flags |= PRINTF_FLAG_PLUS;
	      break;
	    case ' ':
	      flags |= PRINTF_FLAG_SPACE;
	      break;
	    case '#':
	      flags |= PRINTF_FLAG_SPECIAL;
	      break;
	    default:
	      end = 1;
	    }
	  if (end)
	    break;
	  fmt++;
	}

      if (isdigit (*fmt))
	width = parse_fmt_number (&fmt);
      else if (*fmt == '*')
	{
	  int w = va_arg (args, int);
	  if (w < 0)
	    {
	      flags |= PRINTF_FLAG_LEFT;
	      width = -w;
	    }
	  else
	    width = w;
	  fmt++;
	}

      if (*fmt == '.')
	{
	  flags |= PRINTF_FLAG_PREC;
	  fmt++;
	  if (isdigit (*fmt))
	    prec = parse_fmt_number (&fmt);
	  else if (*fmt == '*')
	    {
	      int p = va_arg (args, int);
	      if (p > 0)
		prec = p;
	      fmt++;
	    }
	}

      switch (*fmt)
	{
	case 'l':
	  flags |= PRINTF_FLAG_LONG;
	  fmt++;
	  if (*fmt == 'l')
	    fmt++;
	  break;
	case 'h':
	  flags |= PRINTF_FLAG_SHORT;
	  fmt++;
	  if (*fmt == 'h')
	    {
	      flags |= PRINTF_FLAG_CHAR;
	      fmt++;
	    }
	  break;
	case 't':
	case 'j':
	case 'z':
	  flags |= PRINTF_FLAG_LONG;
	  fmt++;
	  break;
	}

      switch (*fmt)
	{
	case 'd':
	case 'i':
	case 'u':
	case 'x':
	case 'X':
	case 'o':
	case 'b':
	  if (*fmt == 'x')
	    base = 16;
	  else if (*fmt == 'X')
	    {
	      base = 16;
	      flags |= PRINTF_FLAG_UPCASE;
	    }
	  else if (*fmt == 'o')
	    base = 8;
	  else if (*fmt == 'b')
	    base = 2;
	  else
	    {
	      base = 10;
	      flags &= ~PRINTF_FLAG_SPECIAL;
	    }

	  if (*fmt != 'i' && *fmt != 'd')
	    flags &= ~(PRINTF_FLAG_PLUS | PRINTF_FLAG_SPACE);
	  if (flags & PRINTF_FLAG_PREC)
	    flags &= ~PRINTF_FLAG_PAD_ZERO;

	  if (*fmt == 'i' || *fmt == 'd')
	    {
	      if (flags & PRINTF_FLAG_LONG)
		{
		  long value = va_arg (args, long);
		  index = print_long (func, buffer, index, maxlen, value > 0 ?
				      value : -value, value < 0, base, prec,
				      width, flags);
		}
	      else
		{
		  int value = va_arg (args, int);
		  if (flags & PRINTF_FLAG_CHAR)
		    value = (char) value;
		  else if (flags & PRINTF_FLAG_SHORT)
		    value = (short) value;
		  index = print_long (func, buffer, index, maxlen,
				      value > 0 ? value : -value, value < 0,
				      base, prec, width, flags);
		}
	    }
	  else
	    {
	      if (flags & PRINTF_FLAG_LONG)
		index = print_long (func, buffer, index, maxlen,
				    va_arg (args, unsigned long), 0, base,
				    prec, width, flags);
	      else
		{
		  int value = va_arg (args, int);
		  if (flags & PRINTF_FLAG_CHAR)
		    value = (char) value;
		  else if (flags & PRINTF_FLAG_SHORT)
		    value = (short) value;
		  index = print_long (func, buffer, index, maxlen, value, 0,
				      base, prec, width, flags);
		}
	    }
	  fmt++;
	  break;
	case 'c':
	  base = 1;
	  if (!(flags & PRINTF_FLAG_LEFT))
	    {
	      while (base++ < width)
		func (' ', buffer, index++, maxlen);
	    }
	  func (va_arg (args, int), buffer, index++, maxlen);
	  if (flags & PRINTF_FLAG_LEFT)
	    {
	      while (base++ < width)
		func (' ', buffer, index++, maxlen);
	    }
	  fmt++;
	  break;
	case 's':
	  str = va_arg (args, const char *);
	  base = strnlen (str, prec ? prec : (size_t) -1);
	  if (flags & PRINTF_FLAG_PREC && base > prec)
	    base = prec;
	  if (!(flags & PRINTF_FLAG_LEFT))
	    {
	      while (base++ < width)
		func (' ', buffer, index++, maxlen);
	    }
	  while (*str && (!(flags & PRINTF_FLAG_PREC) || prec--))
	    func (*str++, buffer, index++, maxlen);
	  if (flags & PRINTF_FLAG_LEFT)
	    {
	      while (base++ < width)
		func (' ', buffer, index++, maxlen);
	    }
	  fmt++;
	  break;
	case 'p':
	  flags |=
	    PRINTF_FLAG_PAD_ZERO | PRINTF_FLAG_SPECIAL | PRINTF_FLAG_PREC;
	  index = print_long (func, buffer, index, maxlen,
			      (uintptr_t) va_arg (args, void *), 0, 16, 16,
			      0, flags);
	  fmt++;
	  break;
	case 'H':
	  index = print_human_size (func, buffer, index, maxlen,
				    va_arg (args, size_t), width, flags);
	  fmt++;
	  break;
	case '%':
	  func ('%', buffer, index++, maxlen);
	  fmt++;
	  break;
	default:
	  func (*fmt, buffer, index++, maxlen);
	  fmt++;
	}
    }
  func ('\0', buffer, index < maxlen ? index : maxlen - 1, maxlen);
  return index;
}

int
printf (const char *fmt, ...)
{
  va_list args;
  char buffer[1];
  int ret;
  va_start (args, fmt);
  ret = print_internal (write_tty, buffer, (size_t) -1, fmt, args);
  va_end (args);
  return ret;
}

int
sprintf (char *buffer, const char *fmt, ...)
{
  va_list args;
  int ret;
  va_start (args, fmt);
  ret = print_internal (write_buffer, buffer, (size_t) -1, fmt, args);
  va_end (args);
  return ret;
}

int
snprintf (char *buffer, size_t len, const char *fmt, ...)
{
  va_list args;
  int ret;
  va_start (args, fmt);
  ret = print_internal (write_buffer, buffer, len, fmt, args);
  va_end (args);
  return ret;
}

int
vprintf (const char *fmt, va_list args)
{
  char buffer[1];
  return print_internal (write_tty, buffer, (size_t) -1, fmt, args);
}

int
vsprintf (char *buffer, const char *fmt, va_list args)
{
  return print_internal (write_buffer, buffer, (size_t) -1, fmt, args);
}

int
vsnprintf (char *buffer, size_t len, const char *fmt, va_list args)
{
  return print_internal (write_buffer, buffer, len, fmt, args);
}
