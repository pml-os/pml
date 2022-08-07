/* pipe.c -- This file is part of PML.
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

#include <pml/alloc.h>
#include <pml/memory.h>
#include <pml/syscall.h>
#include <pml/vfs.h>
#include <errno.h>
#include <string.h>

/*! Number of pages to allocate for the pipe buffer */
#define PIPE_PAGES              8

/*! Number of bytes in a pipe buffer */
#define PIPE_SIZE               (PIPE_PAGES * PAGE_SIZE)

/*! Inode number of read end vnode */
#define READ_INO                0

/*! Inode number of write end vnode */
#define WRITE_INO               1

static ssize_t pipe_read (struct vnode *vp, void *buffer, size_t len,
			  off_t offset);
static ssize_t pipe_write (struct vnode *vp, const void *buffer, size_t len,
			   off_t offset);
static void pipe_dealloc (struct vnode *vp);

static const struct vnode_ops pipe_vnode_ops = {
  .read = pipe_read,
  .write = pipe_write,
  .dealloc = pipe_dealloc
};

static uintptr_t pipe_addr = PIPE_BUFFER_BASE_VMA;
static lock_t pipe_lock;

/*!
 * Stores information about a pipe. This structure is used as the private data
 * of all pipe vnodes.
 */

struct pipe
{
  char *buffer;                 /*!< Pointer to pipe buffer */
  size_t start;                 /*!< Index of next byte to read */
  size_t end;                   /*!< Index of next byte to place */
  int widowed;                  /*!< Whether the pipe is widowed */
  lock_t lock;                  /*!< Lock for pipe I/O */
};

int
sys_pipe (int fds[2])
{
  struct vnode *readv = vnode_alloc ();
  struct vnode *writev = vnode_alloc ();
  int readfd = alloc_fd ();
  int writefd = alloc_fd ();
  int readpfd = alloc_procfd ();
  int writepfd = alloc_procfd ();
  struct pipe *pipe = calloc (1, sizeof (struct pipe));
  size_t i;
  size_t c;
  if (UNLIKELY (!readv || !writev || !pipe || readfd == -1 || writefd == -1
		|| readpfd == -1 || writepfd == -1))
    goto err0;

  /* Find an unmapped region in the pipe buffer region */
  spinlock_acquire (&pipe_lock);
  while (pipe_addr < PIPE_BUFFER_TOP_VMA)
    {
      if (physical_addr ((void *) pipe_addr))
	pipe_addr += PIPE_SIZE;
      break;
    }
  if (pipe_addr >= PIPE_BUFFER_TOP_VMA)
    {
      spinlock_release (&pipe_lock);
      GOTO_ERROR (ENOMEM, err0);
    }

  /* Allocate pages for the pipe buffer */
  pipe->buffer = (char *) pipe_addr;
  for (i = 0; i < PIPE_PAGES; i++, pipe_addr += PAGE_SIZE)
    {
      uintptr_t page = alloc_page ();
      if (UNLIKELY (!page))
	goto err1;
      if (vm_map_page (THIS_THREAD->args.pml4t, page, (void *) pipe_addr,
		       PAGE_FLAG_RW))
	{
	  free_page (page);
	  goto err1;
	}
    }
  spinlock_release (&pipe_lock);

  /* Fill vnode parameters */
  readv->data = writev->data = pipe;
  readv->ops = writev->ops = &pipe_vnode_ops;
  readv->ino = READ_INO;
  writev->ino = WRITE_INO;

  /* Fill file descriptor parameters */
  fill_fd (readpfd, readfd, readv, O_RDONLY);
  fill_fd (writepfd, writefd, writev, O_WRONLY);
  fds[0] = readpfd;
  fds[1] = writepfd;
  return 0;

 err1:
  for (c = 0, pipe_addr -= PAGE_SIZE; c < i; c++, pipe_addr -= PAGE_SIZE)
    vm_unmap_page (THIS_THREAD->args.pml4t, (void *) pipe_addr);
  spinlock_release (&pipe_lock);
 err0:
  UNREF_OBJECT (readv);
  UNREF_OBJECT (writev);
  free (pipe);
  if (readfd != -1)
    free_fd (readfd);
  if (writefd != -1)
    free_fd (writefd);
  if (readpfd != -1)
    free_procfd (readpfd);
  if (writepfd != -1)
    free_procfd (writepfd);
  return -1;
}

static ssize_t
pipe_read (struct vnode *vp, void *buffer, size_t len, off_t offset)
{
  struct pipe *pipe = vp->data;
  size_t real_len;
  while (pipe->start == pipe->end)
    {
      if (pipe->widowed)
	return 0;
      sched_yield ();
    }
  spinlock_acquire (&pipe->lock);
  real_len = pipe->end - pipe->start;
  memcpy (buffer, pipe->buffer + pipe->start, len > real_len ? real_len : len);
  if (real_len <= len)
    pipe->start = pipe->end = 0;
  spinlock_release (&pipe->lock);
  return len > real_len ? real_len : len;
}

static ssize_t
pipe_write (struct vnode *vp, const void *buffer, size_t len, off_t offset)
{
  struct pipe *pipe = vp->data;
  if (pipe->widowed)
    {
      siginfo_t info;
      info.si_signo = SIGPIPE;
      info.si_code = SI_KERNEL;
      info.si_errno = EPIPE;
      info.si_pid = THIS_PROCESS->pid;
      info.si_uid = THIS_PROCESS->uid;
      send_signal (THIS_PROCESS, SIGPIPE, &info);
      RETV_ERROR (EPIPE, -1);
    }
  if (len > PIPE_SIZE)
    RETV_ERROR (ENOSPC, -1);
  spinlock_acquire (&pipe->lock);
  if (PIPE_SIZE - pipe->end < len)
    {
      spinlock_release (&pipe->lock);
      while (PIPE_SIZE - pipe->end + pipe->start < len)
	sched_yield ();
      spinlock_acquire (&pipe->lock);
      memmove (pipe->buffer, pipe->buffer + pipe->start,
	       pipe->end - pipe->start);
      pipe->end -= pipe->start;
      pipe->start = 0;
    }
  memcpy (pipe->buffer + pipe->end, buffer, len);
  pipe->end += len;
  spinlock_release (&pipe->lock);
  return len;
}

static void
pipe_dealloc (struct vnode *vp)
{
  struct pipe *pipe = vp->data;
  if (pipe->widowed)
    {
      size_t i;
      void *addr;
      spinlock_acquire (&pipe_lock);
      for (i = 0, addr = pipe->buffer; i < PIPE_PAGES; i++, addr += PAGE_SIZE)
	vm_unmap_page (THIS_THREAD->args.pml4t, addr);
      if ((uintptr_t) pipe->buffer < pipe_addr)
	pipe_addr = (uintptr_t) pipe->buffer;
      spinlock_release (&pipe_lock);
      free (pipe);
    }
  else
    pipe->widowed = 1;
}
