/* map.h -- This file is part of PML.
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

#ifndef __PML_MAP_H
#define __PML_MAP_H

/*!
 * @file
 * @brief Hashmap manipulation functions
 */

#include <pml/hash.h>

#define HASHMAP_INIT_BUCKETS    16  /*!< Initial number of buckets in hashmap */

/*!
 * Callback function for freeing a value in a hashmap entry.
 *
 * @param value the value that was inserted into the hashmap
 */

typedef void (*hashmap_free_func_t) (void *value);

/*!
 * Represents a linked list for a bucket in a hashmap. Contains a key and
 * value pair as well as a pointer to the next entry in the linked list.
 */

struct hashmap_entry
{
  struct hashmap_entry *next;       /*!< Next entry in the current bucket */
  unsigned long key;                /*!< Key matching this entry */
  void *value;                      /*!< Value corresponding to the key */
};

/*!
 * Represents a hashmap that maps integer keys to pointer values.
 */

struct hashmap
{
  struct hashmap_entry **buckets;   /*!< Array of hash buckets */
  size_t bucket_count;              /*!< Number of buckets in hashmap */
  size_t object_count;              /*!< Number of objects in hashmap */
};

/*!
 * Represents a linked list for a bucket in a string hashmap. Contains a key and
 * value pair as well as a pointer to the next entry in the linked list.
 */

struct strmap_entry
{
  struct strmap_entry *next;        /*!< Next entry in the current bucket */
  char *key;                        /*!< Key matching this entry */
  void *value;                      /*!< Value corresponding to the key */
};

/*!
 * Represents a hashmap that maps string keys to pointer values.
 */

struct strmap
{
  struct strmap_entry **buckets;    /*!< Array of hash buckets */
  size_t bucket_count;              /*!< Number of buckets in hashmap */
  size_t object_count;              /*!< Number of objects in hashmap */
};

__BEGIN_DECLS

struct hashmap *hashmap_create (void);
void hashmap_free (struct hashmap *hashmap, hashmap_free_func_t free_func);
int hashmap_insert (struct hashmap *hashmap, unsigned long key, void *value);
void *hashmap_lookup (struct hashmap *hashmap, unsigned long key);
int hashmap_remove (struct hashmap *hashmap, unsigned long key);

struct strmap *strmap_create (void);
void strmap_free (struct strmap *strmap, hashmap_free_func_t free_func);
int strmap_insert (struct strmap *strmap, const char *key, void *value);
void *strmap_lookup (struct strmap *strmap, const char *key);
int strmap_remove (struct strmap *strmap, const char *key);

__END_DECLS

#endif
