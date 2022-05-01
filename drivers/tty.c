/* tty.c -- This file is part of PML.
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

/*! @file */

#include <pml/pit.h>
#include <pml/syscall.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!
 * Checks whether an input character matches a control character. A value of
 * 0xff is used to indicate that no character matches the control sequence.
 *
 * @param c the character to test
 * @param tp the termios structure
 * @param x control character name
 * @return nonzero if the character matches the control sequence
 */

#define CHAR_MATCH(c, tp, x) ((c) == (tp)->c_cc[(x)] && (tp)->c_cc[(x)] != 0xff)

struct tty kernel_tty;
struct tty *current_tty = &kernel_tty;

/*!
 * Writes a character to a terminal.
 *
 * @param tty the terminal to write to
 * @param c the character to write, which will be converted to a byte
 */

int
tty_putchar (struct tty *tty, int c)
{
  int wrap = 0;
  if (c == '\0')
    return c;
  switch (c)
    {
    case '\n':
      tty->x = 0;
      __fallthrough;
    case '\v':
    case '\f':
      wrap = 1;
      break;
    case '\r':
      tty->x = 0;
      tty->output->update_cursor (tty);
      return c;
    case '\t':
      tty->x |= 7;
      break;
    default:
      if (tty->output->write_char (tty, tty->x, tty->y, c))
	return EOF;
    }

  if (!wrap && ++tty->x == tty->width)
    {
      tty->x = 0;
      wrap = 1;
    }

  if (wrap && ++tty->y == tty->height)
    {
      if (tty->output->scroll_down (tty))
	return EOF;
      tty->y--;
    }
  tty->output->update_cursor (tty);
  return c;
}

/*!
 * Waits until input for a terminal is ready.
 *
 * @param tty the terminal
 */

void
tty_wait_input_ready (struct tty *tty)
{
  while (!(tty->flags & TTY_FLAG_FLUSH))
    ;
}

/*!
 * Flushes the terminal input buffer.
 *
 * @param tty the terminal
 */

void
tty_flush_input_line (struct tty *tty, unsigned char delim)
{
  if (delim)
    tty_recv (tty, delim);
  tty->flags |= TTY_FLAG_FLUSH;
}

/*!
 * Erases the previous character in the input buffer.
 *
 * @param tty the terminal
 * @return the number of characters erased
 */

size_t
tty_erase_input (struct tty *tty)
{
  struct tty_input *input = &tty->input;
  if (input->end
      && !CHAR_MATCH (input->buffer[input->end - 1], &tty->termios, VEOF))
    {
      input->end--;
      return 1;
    }
  else
    return 0;
}

/*!
 * Erases the previous word in the input buffer, including any whitespace after
 * the word.
 *
 * @param tty the terminal
 * @return the number of characters erased
 */

size_t
tty_erase_input_word (struct tty *tty)
{
  struct tty_input *input = &tty->input;
  int i = input->end - 1;
  size_t len;
  while ((size_t) i >= input->start && isspace (input->buffer[i])
	 && !CHAR_MATCH (input->buffer[i], &tty->termios, VEOF))
    i--;
  while ((size_t) i >= input->start && !isspace (input->buffer[i])
	 && !CHAR_MATCH (input->buffer[i], &tty->termios, VEOF))
    i--;
  if (i < 0)
    i = 0;
  else
    i++;
  len = input->end - i;
  input->end = i;
  return len;
}

/*!
 * Erases the current line of input from the input buffer.
 *
 * @param tty the terminal
 * @return the number of characters erased
 */

size_t
tty_kill_input (struct tty *tty)
{
  struct tty_input *input = &tty->input;
  size_t len;
  unsigned char *ptr = tty->termios.c_cc[VEOF] == 0xff ? NULL :
    memrchr (input->buffer, tty->termios.c_cc[VEOF], input->end);
  if (!ptr || ptr < input->buffer + input->start)
    {
      len = input->end - input->start;
      input->start = 0;
      input->end = 0;
    }
  else
    {
      len = input->buffer + input->end - ptr;
      input->end = ptr - input->buffer;
    }
  return len;
}

/*!
 * Reprints all input currently in the input buffer.
 *
 * @param tty the terminal
 */

void
tty_reprint_input (struct tty *tty)
{
  struct tty_input *input = &tty->input;
  size_t i;
  for (i = input->start; input->end; i++)
    tty_output_byte (tty, input->buffer[i], 0);
}

/*!
 * Appends a byte to a terminal input buffer.
 *
 * @param tty the terminal
 * @param c the byte
 */

void
tty_recv (struct tty *tty, unsigned char c)
{
  if (tty->input.end >= TTY_INPUT_BUFFER_SIZE - 1 && tty->input.start)
    {
      /* Move the current input to the start of the buffer to make space
	 for new data */
      memmove (tty->input.buffer, tty->input.buffer + tty->input.start,
	       tty->input.end - tty->input.start);
      tty->input.end -= tty->input.start;
      tty->input.start = 0;
    }
  if (tty->input.end >= TTY_INPUT_BUFFER_SIZE - 1
      && (!(tty->termios.c_lflag & ICANON) || c != '\n'))
    {
      /* Not enough space, so discard input and sound the system bell */
      if (tty->termios.c_iflag & IMAXBEL)
	pcspk_beep ();
      return;
    }
  tty->input.buffer[tty->input.end++] = c;
}

/*!
 * Receives an input character to the TTY, doing any necessary input
 * pre-processing.
 *
 * @param tty the terminal
 * @param c the byte
 */

void
tty_input_byte (struct tty *tty, unsigned char c)
{
  struct termios *tp = &tty->termios;
  size_t len = 0;

  /* If the input should be taken literally, skip all the processing */
  if (tty->flags & TTY_FLAG_LITERAL_INPUT)
    {
      tty->flags &= ~TTY_FLAG_LITERAL_INPUT;
      goto raw;
    }

  if (tp->c_lflag & ICANON)
    {
      /* Check if the character matches any control sequences */
      if (c == '\n' || CHAR_MATCH (c, tp, VEOL) || CHAR_MATCH (c, tp, VEOL2))
	DO_GOTO (tty_flush_input_line (tty, c), end);
      else if (CHAR_MATCH (c, tp, VEOF) && !tty->input.end)
	DO_GOTO (tty_flush_input_line (tty, '\0'), end);
      else if (CHAR_MATCH (c, tp, VERASE))
	DO_GOTO (len = tty_erase_input (tty), end);
      else if (CHAR_MATCH (c, tp, VKILL))
	DO_GOTO (len = tty_kill_input (tty), end);
      else if (tp->c_lflag & IEXTEN)
	{
	  if (CHAR_MATCH (c, tp, VWERASE))
	    DO_GOTO (len = tty_erase_input_word (tty), end);
	  else if (CHAR_MATCH (c, tp, VREPRINT))
	    DO_GOTO (tty_reprint_input (tty), end);
	  else if (CHAR_MATCH (c, tp, VLNEXT))
	    DO_GOTO (tty->flags |= TTY_FLAG_LITERAL_INPUT, end);
	}
    }

  if (tp->c_lflag & ISIG)
    {
      int sig = 0;
      if (CHAR_MATCH (c, tp, VINTR))
	sig = SIGINT;
      else if (CHAR_MATCH (c, tp, VSUSP))
	sig = SIGTSTP;
      else if (CHAR_MATCH (c, tp, VQUIT))
	sig = SIGQUIT;
      if (sig)
	{
	  sys_kill (-tty->pgid, sig);
	  return;
	}
    }

 raw:
  tty_recv (tty, c);

 end:
  if (tp->c_lflag & ECHO)
    tty_output_byte (tty, c, len);
  else if (c == '\n' && (tp->c_lflag & ECHONL))
    tty_output_byte (tty, '\n', len);
}

/*!
 * Writes a character to the TTY, doing any necessary output pre-processing.
 *
 * @param tty the terminal
 * @param c the byte
 * @param len number of bytes to erase on the output backend, if the character
 * is an erase sequence
 */

void
tty_output_byte (struct tty *tty, unsigned char c, size_t len)
{
  struct termios *tp = &tty->termios;
  if (tp->c_lflag & ICANON)
    {
      if (CHAR_MATCH (c, tp, VERASE) && (tp->c_lflag & ECHOE))
	{
	  if (len)
	    tty->output->erase_char (tty);
	  return;
	}
      else if (CHAR_MATCH (c, tp, VWERASE) && (tp->c_lflag & ECHOE))
	DO_RET (tty->output->erase_line (tty, len));
      else if (CHAR_MATCH (c, tp, VWERASE) && (tp->c_lflag & (ECHOK | ECHOKE)))
	DO_RET (tty->output->erase_line (tty, len));
    }

  switch (c)
    {
    case '\n':
    case '\t':
      tty_putchar (tty, c);
      break;
    default:
      if (iscntrl (c) && (tp->c_lflag & ECHOCTL))
	{
	  if (!CHAR_MATCH (c, tp, VEOF) || tty->x)
	    {
	      tty_putchar (tty, '^');
	      tty_putchar (tty, c + 0x40);
	    }
	}
      else
	tty_putchar (tty, c);
    }
}
