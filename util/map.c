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
#include <string.h>

/*!
 * Creates a new hashmap with no elements and a bucket count of
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
  hashmap->object_count++;
  return 0;
}

/*!
 * Looks up the value of a key in a hashmap.
 *
 * @param hashmap the hashmap
 * @param key the key to look up
 * @return the value mapped to the key, or NULL if the key is not present
 */

void *
hashmap_lookup (struct hashmap *hashmap, unsigned long key)
{
  hash_t index = siphash ((void *) &key, sizeof (unsigned long), 0) %
    hashmap->bucket_count;
  struct hashmap_entry *bucket;
  for (bucket = hashmap->buckets[index]; bucket != NULL; bucket = bucket->next)
    {
      if (bucket->key == key)
	return bucket->value;
    }
  return NULL;
}

/*!
 * Removes an entry matching a key from a hashmap.
 *
 * @param hashmap the hashmap
 * @param key the key to remove
 * @return zero if the key was in the hashmap and successfully removed
 */

int
hashmap_remove (struct hashmap *hashmap, unsigned long key)
{
  size_t i;
  for (i = 0; i < hashmap->bucket_count; i++)
    {
      struct hashmap_entry *prev = NULL;
      struct hashmap_entry *bucket;
      for (bucket = hashmap->buckets[i]; bucket != NULL; bucket = bucket->next)
	{
	  if (bucket->key == key)
	    {
	      if (prev)
		prev->next = bucket->next;
	      else
		hashmap->buckets[i] = bucket->next;
	      free (bucket);
	      return 0;
	    }
	  prev = bucket;
	}
    }
  return -1;
}

/*!
 * Creates a new string hashmap with no elements and a bucket count of
 * @ref HASHMAP_INIT_BUCKETS.
 *
 * @return a new hashmap, or NULL if the allocation failed
 */

struct strmap *
strmap_create (void)
{
  struct strmap *strmap = malloc (sizeof (struct strmap));
  if (UNLIKELY (!strmap))
    return NULL;
  strmap->bucket_count = HASHMAP_INIT_BUCKETS;
  strmap->object_count = 0;
  strmap->buckets =
    calloc (strmap->bucket_count, sizeof (struct strmap_entry *));
  if (UNLIKELY (!strmap->buckets))
    {
      free (strmap);
      return NULL;
    }
  return strmap;
}

/*!
 * Frees a string hashmap and optionally all of its values.
 *
 * @param strmap the hashmap to free
 * @param free_func callback function to call on each hashmap entry's value
 */

void
strmap_free (struct strmap *strmap, hashmap_free_func_t free_func)
{
  size_t i;
  for (i = 0; i < strmap->bucket_count; i++)
    {
      struct strmap_entry *bucket;
      struct strmap_entry *temp;
      for (bucket = strmap->buckets[i]; bucket != NULL; bucket = temp)
	{
	  temp = bucket->next;
	  if (free_func)
	    free_func (bucket->value);
	  free (bucket->key);
	  free (bucket);
	}
    }
  free (strmap);
}

/*!
 * Sets the value of a key in a string hashmap. If a hashmap entry with the key
 * already exists, its value is replaced. Otherwise, a new entry is created
 * with the key/value mapping and is appended to the hashmap.
 *
 * @param strmap the hashmap
 * @param key the key to insert
 * @param value the value corresponding to the key
 * @return zero on success
 */

int
strmap_insert (struct strmap *strmap, const char *key, void *value)
{
  hash_t index;
  struct strmap_entry *bucket;
  struct strmap_entry *new_entry;

  /* If the number of objects is more than 3/4 of the bucket count, double
     the number of buckets */
  if (strmap->object_count >= strmap->bucket_count * 3 / 4)
    {
      struct strmap_entry **buckets =
	calloc (strmap->bucket_count * 2, sizeof (struct strmap *));
      size_t i;
      if (UNLIKELY (!buckets))
	return -1;
      for (i = 0; i < strmap->bucket_count; i++)
	{
	  struct strmap_entry *temp;
	  for (bucket = strmap->buckets[i]; bucket != NULL;
	       bucket = bucket->next)
	    {
	      index = siphash (bucket->key, strlen (bucket->key), 0) %
		(strmap->bucket_count * 2);
	      temp = malloc (sizeof (struct strmap_entry));
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
	  for (bucket = strmap->buckets[i]; bucket != NULL; bucket = temp)
	    {
	      temp = bucket->next;
	      free (bucket);
	    }
	}
      free (strmap->buckets);
      strmap->bucket_count *= 2;
      strmap->buckets = buckets;
    }

  /* Replace an existing entry with the target key */
  index = siphash (key, strlen (key), 0) % strmap->bucket_count;
  for (bucket = strmap->buckets[index]; bucket != NULL; bucket = bucket->next)
    {
      if (!strcmp (bucket->key, key))
	{
	  bucket->value = value;
	  return 0;
	}
    }

  /* Create and insert a new entry into the strmap */
  new_entry = malloc (sizeof (struct strmap_entry));
  if (UNLIKELY (!new_entry))
    return -1;
  index = siphash (key, strlen (key), 0) % strmap->bucket_count;
  new_entry->next = NULL;
  new_entry->key = strdup (key);
  new_entry->value = value;
  if (strmap->buckets[index])
    {
      for (bucket = strmap->buckets[index]; bucket->next != NULL;
	   bucket = bucket->next)
	;
      bucket->next = new_entry;
    }
  else
    strmap->buckets[index] = new_entry;
  strmap->object_count++;
  return 0;
}

/*!
 * Looks up the value of a key in a string hashmap.
 *
 * @param strmap the hashmap
 * @param key the key to look up
 * @return the value mapped to the key, or NULL if the key is not present
 */

void *
strmap_lookup (struct strmap *strmap, const char *key)
{
  hash_t index = siphash (key, strlen (key), 0) % strmap->bucket_count;
  struct strmap_entry *bucket;
  for (bucket = strmap->buckets[index]; bucket != NULL; bucket = bucket->next)
    {
      if (!strcmp (bucket->key, key))
	return bucket->value;
    }
  return NULL;
}

/*!
 * Removes an entry matching a key from a string hashmap.
 *
 * @param strmap the hashmap
 * @param key the key to remove
 * @return zero if the key was in the strmap and successfully removed
 */

int
strmap_remove (struct strmap *strmap, const char *key)
{
  size_t i;
  for (i = 0; i < strmap->bucket_count; i++)
    {
      struct strmap_entry *prev = NULL;
      struct strmap_entry *bucket;
      for (bucket = strmap->buckets[i]; bucket != NULL; bucket = bucket->next)
	{
	  if (!strcmp (bucket->key, key))
	    {
	      if (prev)
		prev->next = bucket->next;
	      else
		strmap->buckets[i] = bucket->next;
	      return 0;
	    }
	  free (bucket->key);
	  free (bucket);
	  prev = bucket;
	}
    }
  return -1;
}
