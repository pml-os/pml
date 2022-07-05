/* sha256.c -- This file is part of PML.
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

#include <pml/hash.h>

static const uint32_t sha256_k[] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
  0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
  0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
  0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
  0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
  0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void
sha256_consume_chunk (uint32_t *h, const unsigned char *ptr)
{
  uint32_t ah[8];
  uint32_t w[16];
  unsigned int i;
  unsigned int j;
  for (i = 0; i < 8; i++)
    ah[i] = h[i];

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 16; j++)
	{
	  uint32_t s0;
	  uint32_t s1;
	  uint32_t t1;
	  uint32_t t2;
	  uint32_t ch;
	  uint32_t maj;
	  if (i == 0)
	    {
	      w[j] = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
	      ptr += 4;
	    }
	  else
	    {
	      s0 = ror (w[(j + 1) & 0xf], 7) ^
		ror (w[(j + 1) & 0xf], 18) ^ (w[(j + 1) & 0xf] >> 3);
	      s1 = ror (w[(j + 14) & 0xf], 17) ^
		ror (w[(j + 14) & 0xf], 19) ^ (w[(j + 14) & 0xf] >> 10);
	      w[j] += s0 + w[(j + 9) & 0xf] + s1;
	    }
	  s1 = ror (ah[4], 6) ^ ror (ah[4], 11) ^ ror (ah[4], 25);
	  ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);

	  t1 = ah[7] + s1 + ch + sha256_k[(i << 4) | j] + w[j];
	  s0 = ror (ah[0], 2) ^ ror (ah[0], 13) ^ ror (ah[0], 22);
	  maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
	  t2 = s0 + maj;

	  ah[7] = ah[6];
	  ah[6] = ah[5];
	  ah[5] = ah[4];
	  ah[4] = ah[3] + t1;
	  ah[3] = ah[2];
	  ah[2] = ah[1];
	  ah[1] = ah[0];
	  ah[0] = t1 + t2;
	}
    }

  for (i = 0; i < 8; i++)
    h[i] += ah[i];
}

void
sha256_init (struct sha256_ctx *ctx, unsigned char digest[SHA256_DIGEST_SIZE])
{
  ctx->digest = digest;
  ctx->ptr = ctx->chunk;
  ctx->rem = SHA256_CHUNK_SIZE;
  ctx->len = 0;

  ctx->h[0] = 0x6a09e667;
  ctx->h[1] = 0xbb67ae85;
  ctx->h[2] = 0x3c6ef372;
  ctx->h[3] = 0xa54ff53a;
  ctx->h[4] = 0x510e527f;
  ctx->h[5] = 0x9b05688c;
  ctx->h[6] = 0x1f83d9ab;
  ctx->h[7] = 0x5be0cd19;
}

void
sha256_write (struct sha256_ctx *ctx, const void *data, size_t len)
{
  const unsigned char *ptr = data;
  ctx->len += len;
  while (len > 0)
    {
      if (ctx->rem == SHA256_CHUNK_SIZE && len >= SHA256_CHUNK_SIZE)
	{
	  sha256_consume_chunk (ctx->h, ptr);
	  len -= SHA256_CHUNK_SIZE;
	  ptr += SHA256_CHUNK_SIZE;
	}
      else
	{
	  size_t consumed = len < ctx->rem ? len : ctx->rem;
	  memcpy (ctx->ptr, ptr, consumed);
	  ctx->rem -= consumed;
	  len -= consumed;
	  ptr += consumed;
	  if (ctx->rem == 0)
	    {
	      sha256_consume_chunk (ctx->h, ctx->chunk);
	      ctx->ptr = ctx->chunk;
	      ctx->rem = SHA256_CHUNK_SIZE;
	    }
	  else
	    ctx->ptr += consumed;
	}
    }
}

unsigned char *
sha256_close (struct sha256_ctx *ctx)
{
  unsigned char *pos = ctx->ptr;
  size_t rem = ctx->rem;
  size_t len;
  int i;
  int j;

  *pos++ = 0x80;
  rem--;

  if (rem < 8)
    {
      memset (pos, 0, rem);
      sha256_consume_chunk (ctx->h, ctx->chunk);
      pos = ctx->chunk;
      rem = SHA256_CHUNK_SIZE;
    }
  rem -= 8;
  memset (pos, 0, rem);
  pos += rem;

  len = ctx->len;
  pos[7] = (len << 3) & 0xff;
  len >>= 5;
  for (i = 6; i >= 0; i--)
    {
      pos[i] = len & 0xff;
      len >>= 8;
    }
  sha256_consume_chunk (ctx->h, ctx->chunk);

  for (i = 0, j = 0; i < 8; i++)
    {
      ctx->digest[j++] = (ctx->h[i] >> 24) & 0xff;
      ctx->digest[j++] = (ctx->h[i] >> 16) & 0xff;
      ctx->digest[j++] = (ctx->h[i] >> 8) & 0xff;
      ctx->digest[j++] = ctx->h[i] & 0xff;
    }
  return ctx->digest;
}

void
sha256_data (unsigned char digest[SHA256_DIGEST_SIZE], const void *data,
	     size_t len)
{
  struct sha256_ctx ctx;
  sha256_init (&ctx, digest);
  sha256_write (&ctx, data, len);
  sha256_close (&ctx);
}
