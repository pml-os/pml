/* entry.c -- This file is part of PML.
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

#include <pml/device.h>
#include <pml/panic.h>
#include <pml/process.h>
#include <pml/syscall.h>
#include <pml/tty.h>
#include <pml/vfs.h>
#include <pml/wait.h>
#include <stdio.h>
#include <stdlib.h>

static char *init_argv[] = {"init", NULL};
static char *sh_argv[] = {"sh", NULL};

/*! Prints a welcome message on boot. */

void
splash (void)
{
  printf ("\n\nWelcome to PML 0.1\nCopyright (C) 2021 XNSC\n"
	  "System time: %ld\n", real_time);
}

/*! Forks the kernel process and runs the init program. */

void
fork_init (void)
{
  pid_t pid = __fork (0);
  if (UNLIKELY (pid == -1))
    panic ("Failed to fork init process");
  if (!pid)
    {
      /* Setup file descriptor table */
      struct fd_table *fds = &THIS_PROCESS->fds;
      int fd;
      fds->size = 64;
      fds->table = calloc (sizeof (struct fd *), fds->size);
      if (UNLIKELY (!fds->table))
	panic ("Failed to allocate file descriptor table");
      fds->max_size = 256;

      /* Setup standard streams to kernel console */
      fd = sys_open ("/dev/console", O_RDWR);
      if (LIKELY (fd != -1))
	{
	  sys_dup (fd);
	  sys_dup (fd);
	}

      /* Run an init process */
      sys_execve ("/sbin/init", init_argv, NULL);
      sys_execve ("/bin/init", init_argv, NULL);
      sys_execve ("/init", init_argv, NULL);
      sys_execve ("/bin/sh", sh_argv, NULL);
      panic ("No init process found");
    }
  else
    {
      int status;
      sys_wait4 (pid, &status, 0, NULL);
      if (WIFEXITED (status))
	panic ("Init process terminated with status %d", WEXITSTATUS (status));
      else if (WIFSIGNALED (status))
	panic ("Init process received signal %d", WTERMSIG (status));
      else
	panic ("Init process killed");
    }
}

void
kentry (void)
{
  init_command_line ();
  ata_init ();
  device_map_init ();
  device_ata_init ();
  tty_device_init ();
  mount_root ();
  init_pid_allocator ();
  sched_yield ();

  splash ();
  fork_init ();
}
