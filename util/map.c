/* map.c -- This file is part of PML.
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

#include <pml/map.h>
#include <stdlib.h>

/*!
 * Creates a new hashmap with no elements and a bucket cout of
 * @ref HASHMAP_INIT_BUCKETS.
 *
 * @return a new hashmap, or NULL if the allocation failed
 */

struct hashmap *
hashmap_create (void)
{
  struct hashmap *hashmap = malloc (sizeof (struct hashmap));
  if (UNLIKELY (!hashmap))
    return NULL;
  hashmap->bucket_count = HASHMAP_INIT_BUCKETS;
  hashmap->object_count = 0;
  hashmap->buckets =
    calloc (hashmap->bucket_count, sizeof (struct hashmap_entry *));
  if (UNLIKELY (!hashmap->buckets))
    {
      free (hashmap);
      return NULL;
    }
  return hashmap;
}

/*!
 * Frees a hashmap and optionally all of its values.
 *
 * @param hashmap the hashmap to free
 * @param free_func callback function to call on each hashmap entry's value
 */

void
hashmap_free (struct hashmap *hashmap, hashmap_free_func_t free_func)
{
  size_t i;
  for (i = 0; i < hashmap->bucket_count; i++)
    {
      struct hashmap_entry *bucket;
      struct hashmap_entry *temp;
      for (bucket = hashmap->buckets[i]; bucket != NULL; bucket = temp)
	{
	  temp = bucket->next;
	  if (free_func)
	    free_func (bucket->value);
	  free (bucket);
	}
    }
  free (hashmap);
}

/*!
 * Sets the value of a key in a hashmap. If a hashmap entry with the key
 * already exists, its value is replaced. Otherwise, a new entry is created
 * with the key/value mapping and is appended to the hashmap.
 *
 * @param hashmap the hashmap
 * @param key the key to insert
 * @param value the value corresponding to the key
 * @return zero on success
 */

int
hashmap_insert (struct hashmap *hashmap, unsigned long key, void *value)
{
  hash_t index;
  struct hashmap_entry *bucket;
  struct hashmap_entry *new_entry;

  /* If the number of objects is more than 3/4 of the bucket count, double
     the number of buckets */
  if (hashmap->object_count >= hashmap->bucket_count * 3 / 4)
    {
      struct hashmap_entry **buckets =
	calloc (hashmap->bucket_count * 2, sizeof (struct hashmap *));
      size_t i;
      if (UNLIKELY (!buckets))
	return -1;
      for (i = 0; i < hashmap->bucket_count; i++)
	{
	  struct hashmap_entry *temp;
	  for (bucket = hashmap->buckets[i]; bucket != NULL;
	       bucket = bucket->next)
	    {
	      index =
		siphash ((void *) &bucket->key, sizeof (unsigned long), 0) %
		(hashmap->bucket_count * 2);
	      temp = malloc (sizeof (struct hashmap_entry));
	      if (UNLIKELY (!temp))
		{
		  free (buckets);
		  return -1;
		}
	      temp->next = NULL;
	      temp->key = bucket->key;
	      temp->value = bucket->value;
	      if (buckets[index])
		{
		  for (new_entry = buckets[index]; new_entry->next != NULL;
		       new_entry = new_entry->next)
		    ;
		  new_entry->next = temp;
		}
	      else
		buckets[index] = temp;
	    }
	  for (bucket = hashmap->buckets[i]; bucket != NULL; bucket = temp)
	    {
	      temp = bucket->next;
	      free (bucket);
	    }
	}
      free (hashmap->buckets);
      hashmap->bucket_count *= 2;
      hashmap->buckets = buckets;
    }

  /* Replace an existing entry with the target key */
  index = siphash ((void *) &key, sizeof (unsigned long), 0) %
    hashmap->bucket_count;
  for (bucket = hashmap->buckets[index]; bucket != NULL; bucket = bucket->next)
    {
      if (bucket->key == key)
	{
	  bucket->value = value;
	  return 0;
	}
    }

  /* Create and insert a new entry into the hashmap */
  new_entry = malloc (sizeof (struct hashmap_entry));
  if (UNLIKELY (!new_entry))
    return -1;
  index = siphash ((void *) &key, sizeof (unsigned long), 0) %
    hashmap->bucket_count;
  new_entry->next = NULL;
  new_entry->key = key;
  new_entry->value = value;
  if (hashmap->buckets[index])
    {
      for (bucket = hashmap->buckets[index]; bucket->next != NULL;
	   bucket = bucket->next)
	;
      bucket->next = new_entry;
    }
  else
    hashmap->buckets[index] = new_entry;
  return 0;
}
