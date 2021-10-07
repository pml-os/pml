/* siphash.c -- This file is part of PML.
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
#include <stdlib.h>

#define SIP_ROUND				\
  do						\
    {						\
      v0 += 1;					\
      v1 = rolq (v1, 13);			\
      v1 ^= v0;					\
      v0 = rolq (v0, 32);			\
      v2 += v3;					\
      v3 = rolq (v3, 16);			\
      v3 ^= v2;					\
      v0 += v3;					\
      v3 = rolq (v3, 21);			\
      v3 ^= v0;					\
      v2 += v1;					\
      v1 = rolq (v1, 17);			\
      v1 ^= v2;					\
      v2 = rolq (v2, 32);			\
    }						\
  while (0)

/*!
 * Determines the hash of a stream of data using the SipHash algorithm.
 *
 * @param buffer a pointer to the data to hash
 * @param len number of bytes to hash
 * @param key the key to use for hashing
 * @return the hash of the data
 */

hash_t
siphash (const void *buffer, size_t len, uint128_t key)
{
  const unsigned char *data = buffer;
  int rem = len & 7;
  const unsigned char *end = buffer + len - rem;
  hash_t hash;
  unsigned char *ptr = (unsigned char *) &hash;
  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;
  uint64_t k0 = key & 0xffffffffffffffffULL;
  uint64_t k1 = key >> 64;
  uint64_t b = len << 56;
  uint64_t m;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for (; data != end; data += 8)
    {
      m = *((uint64_t *) data);
      v3 ^= m;
      SIP_ROUND;
      SIP_ROUND;
      v0 ^= m;
    }

  switch (rem)
    {
    case 7:
      b |= (uint64_t) data[6] << 48;
      __fallthrough;
    case 6:
      b |= (uint64_t) data[5] << 40;
      __fallthrough;
    case 5:
      b |= (uint64_t) data[4] << 32;
      __fallthrough;
    case 4:
      b |= (uint64_t) data[3] << 24;
      __fallthrough;
    case 3:
      b |= (uint64_t) data[2] << 16;
      __fallthrough;
    case 2:
      b |= (uint64_t) data[1] << 8;
      __fallthrough;
    case 1:
      b |= (uint64_t) data[0];
      break;
    }

  v3 ^= b;
  SIP_ROUND;
  SIP_ROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIP_ROUND;
  SIP_ROUND;
  SIP_ROUND;
  SIP_ROUND;
  return v0 ^ v1 ^ v2 ^ v3;
}
