/* stdlib.h -- This file is part of PML.
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

#ifndef __STDLIB_H
#define __STDLIB_H

/*! @file */

#include <pml/cdefs.h>
#include <pml/types.h>

/*!
 * Determines whether an integer or pointer value is aligned.
 *
 * @param x the value to test
 * @param s the required alignment
 * @return nonzero if aligned
 */

#define ALIGNED(x, s) (!((uintptr_t) (x) & ((s) - 1)))

/*!
 * Determines whether a NUL character @c '\0' is present in a @c long value.
 *
 * @param x the value to test
 * @return nonzero if a NUL character is present
 */

#define LONG_NULL(x) (((x) - 0x0101010101010101) & ~(x) & 0x8080808080808080)

/*!
 * Determines whether a specific character is present in a @c long value.
 *
 * @param x the value to test
 * @param c the character to test for
 * @return nonzero if the character is present
 */

#define LONG_CHAR(x, c) LONG_NULL ((x) ^ (c))

/*!
 * Aligns an integer or pointer value down.
 *
 * @param x the value to align
 * @param a the required alignment
 * @return an aligned value of the same type
 */

#define ALIGN_DOWN(x, a) ((__typeof__ (x)) ((uintptr_t) (x) & ~((a) - 1)))

/*!
 * Aligns an integer or pointer value up.
 *
 * @param x the value to align
 * @param a the required alignment
 * @return an aligned value of the same type
 */

#define ALIGN_UP(x, a)						\
  ((__typeof__ (x)) ((((uintptr_t) (x) - 1) | ((a) - 1)) + 1))

/*!
 * Determines whether an integer value is a power of two.
 *
 * @param x the value to test
 * @return nonzero if the value is a power of two
 */

#define IS_P2(x) ((x) != 0 && !((x) & ((x) - 1)))

/*! Marks a condition as likely to occur to the branch predictor. */
#define LIKELY(x)               (__builtin_expect (!!(x), 1))
/*! Marks a condition as unlikely to occur to the branch predictor. */
#define UNLIKELY(x)             (__builtin_expect (!!(x), 0))

/*!
 * Creates a symbol alias. The targeted symbol must appear in the same
 * translation unit.
 *
 * @param old the existing symbol to alias
 * @param new the name of the new symbol
 */

#define ALIAS(old, new)         extern __typeof__ (old) new __alias (old)

/*!
 * Convenience macro that executes a statement, then jumps to a label.
 *
 * @param x statement to execute
 * @param l label to jump to
 */

#define DO_GOTO(x, l) do { (x); goto l; } while (0)

/*!
 * Convenience macro that executes a statement, then returns from the function.
 *
 * @param x statement to execute
 */

#define DO_RET(x) do { (x); return; } while (0)

/*!
 * Convenience macro that executes a statement, then returns from the function
 * with a given return value.
 *
 * @param x statement to execute
 * @param v return value
 */

#define DO_RETV(x, v) do { (x); return (v); } while (0)

/*!
 * Sets a bit in a bitmap.
 *
 * @param bitmap the bitmap
 * @param index the index of the bit to set
 */

static inline void
set_bit (void *bitmap, size_t index)
{
  ((unsigned char *) bitmap)[index >> 3] |= 1 << (index & 7);
}

/*!
 * Clears a bit in a bitmap.
 *
 * @param bitmap the bitmap
 * @param index the index of the bit to clear
 */

static inline void
clear_bit (void *bitmap, size_t index)
{
  ((unsigned char *) bitmap)[index >> 3] &= ~(1 << (index & 7));
}

/*!
 * Tests whether a bit in a bitmap is set.
 *
 * @param bitmap the bitmap
 * @param index the index of the bit to test
 * @return nonzero if the bit is set
 */

static inline int
test_bit (const void *bitmap, size_t index)
{
  return ((unsigned char *) bitmap)[index >> 3] & (1 << (index & 7));
}

/*!
 * Rotates a 32-bit unsigned integer left.
 *
 * @param x the number to rotate
 * @param n the number of bits to rotate by
 * @return the rotated number
 */

static inline unsigned int
roll (unsigned int x, int n)
{
  return (x << n) | (x >> (32 - n));
}

/*!
 * Rotates a 32-bit unsigned integer right.
 *
 * @param x the number to rotate
 * @param n the number of bits to rotate by
 * @return the rotated number
 */

static inline unsigned int
rorl (unsigned int x, int n)
{
  return (x >> n) | (x << (32 - n));
}

/*!
 * Rotates a 64-bit unsigned integer left.
 *
 * @param x the number to rotate
 * @param n the number of bits to rotate by
 * @return the rotated number
 */

static inline unsigned long
rolq (unsigned long x, int n)
{
  return (x << n) | (x >> (64 - n));
}

/*!
 * Rotates a 64-bit unsigned integer right.
 *
 * @param x the number to rotate
 * @param n the number of bits to rotate by
 * @return the rotated number
 */

static inline unsigned long
rorq (unsigned long x, int n)
{
  return (x >> n) | (x << (64 - n));
}

/*!
 * Divides a 32-bit unsigned integer by another integer, rounding up the result.
 *
 * @param a the dividend
 * @param b the divisor
 * @return the result rounded up
 */

static inline unsigned int
div32_ceil (unsigned int a, unsigned int b)
{
  if (!a)
    return 0;
  return (a - 1) / b + 1;
}

/*!
 * Divides a 64-bit unsigned integer by another integer, rounding up the result.
 *
 * @param a the dividend
 * @param b the divisor
 * @return the result rounded up
 */

static inline unsigned long
div64_ceil (unsigned long a, unsigned long b)
{
  if (!a)
    return 0;
  return (a - 1) / b + 1;
}

__BEGIN_DECLS

extern time_t real_time;

unsigned long strtoul (const char *__restrict__ str, char **__restrict__ end,
		       int base) __pure;

void *malloc (size_t size);
void *calloc (size_t block, size_t size);
void *aligned_alloc (size_t align, size_t size);
void *valloc (size_t size);
void *realloc (void *ptr, size_t size);
void free (void *ptr);

__END_DECLS

#endif
