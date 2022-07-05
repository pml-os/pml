/* hash.h -- This file is part of PML.
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

#ifndef __PML_HASH_H
#define __PML_HASH_H

/*!
 * @file
 * @brief Hash function implementations
 */

#include <pml/cdefs.h>
#include <pml/types.h>

/*! Type used for representing a hash. */
typedef unsigned long hash_t;

#define SHA256_DIGEST_SIZE 32
#define SHA256_CHUNK_SIZE  64

struct sha256_ctx
{
  unsigned char *digest;
  unsigned char chunk[SHA256_CHUNK_SIZE];
  unsigned char *ptr;
  size_t rem;
  size_t len;
  uint32_t h[8];
};

__BEGIN_DECLS

uint16_t crc16 (uint16_t seed, const void *data, size_t len);
uint32_t crc32 (uint32_t seed, const void *data, size_t len);

void sha256_init (struct sha256_ctx *ctx,
		  unsigned char digest[SHA256_DIGEST_SIZE]);
void sha256_write (struct sha256_ctx *ctx, const void *data, size_t len);
unsigned char *sha256_close (struct sha256_ctx *ctx);
void sha256_data (unsigned char digest[SHA256_DIGEST_SIZE], const void *data,
		  size_t len);

hash_t siphash (const void *buffer, size_t len, uint128_t key);

__END_DECLS

#endif
