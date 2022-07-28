/* random.c -- This file is part of PML.
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

#include <pml/hash.h>
#include <pml/hpet.h>
#include <pml/lock.h>
#include <pml/random.h>
#include <string.h>

static unsigned char entropy_pool[SHA256_DIGEST_SIZE];
static lock_t entropy_lock;

static void
__add_entropy (const void *data, size_t len)
{
  struct sha256_ctx ctx;
  unsigned char buffer[SHA256_DIGEST_SIZE];
  memcpy (buffer, entropy_pool, SHA256_DIGEST_SIZE);
  sha256_init (&ctx, buffer);
  sha256_write (&ctx, entropy_pool, SHA256_DIGEST_SIZE);
  sha256_write (&ctx, data, len);
  sha256_close (&ctx);
  memcpy (entropy_pool, buffer, SHA256_DIGEST_SIZE);
}

/*!
 * Adds entropy from a block of data to the entropy pool.
 *
 * @param data pointer to the data
 * @param length of the data in bytes
 */

void
add_entropy (const void *data, size_t len)
{
  spinlock_acquire (&entropy_lock);
  __add_entropy (data, len);
  spinlock_release (&entropy_lock);
}

/*!
 * Reads random data from the entropy pool into a buffer.
 *
 * @param data pointer to the buffer
 * @param len number of bytes to read
 */

void
get_entropy (void *data, size_t len)
{
  unsigned char buffer[SHA256_DIGEST_SIZE];
  spinlock_acquire (&entropy_lock);
  while (len > 0)
    {
      size_t l;
      sha256_data (buffer, entropy_pool, SHA256_DIGEST_SIZE);
      __add_entropy (buffer, SHA256_DIGEST_SIZE);
      l = len < SHA256_DIGEST_SIZE ? len : SHA256_DIGEST_SIZE;
      memcpy (data, buffer, l);
      data += l;
      len -= l;
    }
  spinlock_release (&entropy_lock);
}

/*!
 * Initializes the random number generator.
 */

void
random_init (void)
{
  clock_t ticks = hpet_nanotime ();
  sha256_data (entropy_pool, &ticks, sizeof (clock_t));
}

ssize_t
sys_getrandom (void *buffer, size_t len, unsigned int flags)
{
  get_entropy (buffer, len);
  return len;
}
