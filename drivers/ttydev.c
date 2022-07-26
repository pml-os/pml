/* ttydev.c -- This file is part of PML.
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

#include <pml/devfs.h>
#include <pml/device.h>
#include <pml/pit.h>
#include <pml/tty.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

struct tty kernel_tty;
struct tty *current_tty = &kernel_tty;
struct hashmap *tty_hashmap;

ssize_t
tty_device_read (struct char_device *dev, unsigned char *c, int block)
{
  struct tty *tty = dev->device.data;
  if (tty->termios.c_lflag & ICANON)
    {
      tty_wait_input_ready (tty);
    ready:
      if (tty->input.start == tty->input.end)
	{
	  tty->flags &= ~TTY_FLAG_FLUSH;
	  return 0;
	}
      *c = tty->input.buffer[tty->input.start];
      if (++tty->input.start == tty->input.end)
	{
	  tty->input.start = tty->input.end = 0;
	  tty->flags &= ~TTY_FLAG_FLUSH;
	  return 2;
	}
      return 1;
    }
  else
    {
      cc_t min = tty->termios.c_cc[VMIN];
      cc_t time = tty->termios.c_cc[VTIME];
      if (!min && !time)
	{
	  if (tty->input.end > tty->input.start)
	    goto ready;
	  else
	    return 0;
	}
      else if (min && !time)
	{
	  while (tty->input.end  - tty->input.start < min)
	    ;
	  goto ready;
	}
      else if (!min && time)
	{
	  unsigned long tick = pit_ticks;
	  while (tty->input.start == tty->input.end
		 && tick + time * 100 < pit_ticks)
	    ;
	  if (tty->input.start == tty->input.end)
	    return 0;
	  else
	    goto ready;
	}
      else
	{
	  size_t bytes = tty->input.end - tty->input.start;
	  unsigned long tick = pit_ticks;
	  while (bytes < min && tick + time * 100 < pit_ticks)
	    {
	      if (tty->input.end - tty->input.start > bytes)
		bytes = tty->input.end - tty->input.start;
	    }
	  goto ready;
	}
    }
}

ssize_t
tty_device_write (struct char_device *dev, unsigned char c, int block)
{
  struct tty *tty = dev->device.data;
  if ((tty->termios.c_lflag & TOSTOP) && THIS_PROCESS->pgid != tty->pgid)
    {
      siginfo_t info;
      info.si_signo = SIGTTOU;
      info.si_code = 0;
      info.si_errno = 0;
      send_signal (THIS_PROCESS, SIGTTOU, &info);
      RETV_ERROR (EINTR, -1);
    }
  tty_output_byte (tty, c, 0);
  return 1;
}

/*!
 * Initializes the terminal device /dev/console.
 */

void
tty_device_init (void)
{
  struct char_device *device =
    (struct char_device *) device_add ("console", 0, 0, DEVICE_TYPE_CHAR);
  if (LIKELY (device))
    {
      device->device.data = current_tty;
      device->read = tty_device_read;
      device->write = tty_device_write;
    }
  else
    printf ("tty: failed to allocate /dev/console\n");

  /* Initialize TTY hashmap */
  tty_hashmap = hashmap_create ();
  if (hashmap_insert (tty_hashmap, 0, &kernel_tty))
    printf ("tty: failed to initialize default tty\n");
}

/*!
 * Determines the controlling TTY of a session.
 *
 * @param sid the session ID
 * @return the controlling TTY of the session, or the default TTY if the
 * session does not have a controlling TTY
 */

struct tty *
tty_get_from_sid (pid_t sid)
{
  struct tty *tty;
  if (!tty_hashmap)
    return &kernel_tty;
  tty = hashmap_lookup (tty_hashmap, sid);
  if (tty)
    return tty;
  else
    return &kernel_tty;
}

/*!
 * Obtains the TTY structure of a file descriptor representing a TTY.
 *
 * @param fd the file descriptor
 * @return the TTY structure, or NULL if the file descriptor is invalid or does
 * not represent a TTY
 */

struct tty *
tty_from_fd (int fd)
{
  struct fd *file = file_fd (fd);
  if (!file)
    return NULL;
  /* XXX Bad hack to get device from vnode */
  if (file->vnode->mount == devfs)
    {
      struct device *device =
	hashmap_lookup (device_num_map, file->vnode->rdev);
      if (UNLIKELY (!device))
	return NULL;
      if (device->type == DEVICE_TYPE_CHAR)
	return device->data;
    }
  return NULL;
}
