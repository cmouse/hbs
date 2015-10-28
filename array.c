/* (c) Aki Tuomi 2005

This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#define IN_ARRAY_C 1
#include "hbs.h"
// Amount to keep allocated - reduces reallocations
#define ARRAY_ALLOC_AMOUNT 50

/**
 * Revamped array code
 *
 */

/**
 * Array reallocator
 * 
 * parameters:
 *  ARRAY - array to reallocate
 *  nmembs - number of members wanted
 *
 * returns: 
 *  non-zero if failed
 */
int array_reallocate (ARRAY array, size_t nmembs)
{
  char *memory;
  size_t amembs;
  /* Calulate reallocation need */
  /* Check if 0... */
  if (nmembs == 0)
    {
      if (array->items != NULL)
	{
	  free (array->items);
	}
      array->items = NULL;
      array->amembs = 0;
      array->nmembs = 0;
      return 0;
    }
  if ((array->amembs <= nmembs) ||
      (array->amembs - nmembs > ARRAY_ALLOC_AMOUNT))
    {
      /* We must adjust memory */
      if (array->amembs <= nmembs)
	{
	  amembs = nmembs + ARRAY_ALLOC_AMOUNT;
	}
      else
	{
	  amembs = nmembs + ARRAY_ALLOC_AMOUNT / 2;
	}
      if ((memory = realloc (array->items, array->size * amembs)) == NULL)
	{
	  /* Oh no */
	  return 1;
	}
      array->items = memory;
      array->amembs = amembs;
    }
  array->nmembs = nmembs;
  return 0;
}

/**
 * Creates a new array to use.
 * 
 * parameters:
 *  size   - size of array elements
 *  cmp    - comparator for elements
 *  eraser - erasure function for elements, if null no cleanup is done
 * 
 * return value:
 *  an ARRAY object
 */
ARRAY
array_create (size_t size, array_comparator_t cmp, array_walker_t eraser)
{
  ARRAY new;
  /* Sanity checks */
  if ((cmp == NULL) || (size < 1))
    return NULL;
  /* Allocate new array */
  if ((new = malloc (sizeof (struct array_s))) == NULL)
    {
      return NULL;
    }
  /* Initialize array */
  memset (new, 0, sizeof (struct array_s));
  new->items = NULL;
  new->amembs = 0;
  new->nmembs = 0;
  new->size = size;
  new->comparator = cmp;
  new->eraser = eraser;
  return new;
}

void *array_add (ARRAY array, const void *key)
{
  /* Search start and stop point */
  size_t a, b, m;
  int cmp;
  if (array == NULL)
    return NULL;
  if (array->items != NULL)
    {
      a = 0;
      b = array->nmembs - 1;
      /* It's easier to search the actual search point using for-loop */
      while ((b - a) > 1)
	{
	  /* Choose pivot. The odds and evens must be sorted. */
	  m = ((b - a) % 2 == 0 ? a + (b - a) / 2 : a + (b - a + 1) / 2);
	  /* Compare pivot */
	  cmp = (*array->comparator) (key, array->items + m * array->size);
	  /* If key is smaller than pivot, search the lower half */
	  if (cmp < 0)
	    {
	      b = m;
	    }
	  else if (cmp == 0)
	    {
	      /* The key is already here */
	      return (void *) (array->items + m * array->size);
	    }
	  else
	    {
	      /* The key is bigger than pivot, search the upper half */
	      a = m;
	    }
	}
      /* Locate the actual insert point */
      for (; a < b + 1; a++)
	{
	  cmp = (*array->comparator) (key, array->items + a * array->size);
	  /* Found it */
	  if (cmp < 0)
	    {
	      break;
	    }
	  else if (cmp == 0)
	    {
	      /* Found ourselves */
	      return (void *) (array->items + a * array->size);
	    }
	}
    }
  else
    {
      /* No elements in array */
      a = 0;
    }
  /* Reallocate array if necessary, check for failure */
  if (array_reallocate (array, array->nmembs + 1))
    return NULL;
  /* Move elements if necessary */
  if (array->nmembs - 1 > a)
    memmove (array->items + (a + 1) * array->size,
	     array->items + a * array->size,
	     array->size * (array->nmembs - a - 1));
  /* Return pointer to this new memory */
  memset (array->items + a * array->size, 0, array->size);
  return (void *) (array->items + a * array->size);
}

void *array_find (ARRAY array, const void *key)
{
  if (array == NULL)
    return NULL;
  if (key == NULL)
    return NULL;
  /* Check for empty array */
  if (array->nmembs == 0)
    return NULL;
  /* Return said element */
  return bsearch (key, array->items, array->nmembs, array->size,
		  array->comparator);
}

int _array_delete (ARRAY array, const void *key, int drop)
{
  char *ptr;
  unsigned long pos;
  /* No such element exists */
  if ((ptr = array_find (array, key)) == NULL)
    return 1;
  /* clear element */
  if (drop && array->eraser)
    (*array->eraser) (ptr, NULL);
  if (ptr != array->items + array->size * (array->nmembs - 1))
    {
      /* Calculate element position */
      pos =
	((unsigned long) ptr - (unsigned long) array->items) / array->size;
      memmove (ptr, ptr + array->size,
	       (array->nmembs - pos - 1) * array->size);
    }
  if (array_reallocate (array, array->nmembs - 1))
    abort ();
  return 0;
}

/**
 * Wrapper that only drops the element from array. Cleanup not called.
 * @param array - Array to manipulate
 * @param key - key to drop
 */
int array_drop (ARRAY array, const void *key)
{
  return _array_delete (array, key, 0);
}

/**
 * Wrapper that deletes the element from array. Cleanup is called.
 * @param array - Array to manipulate
 * @param key - key to drop
 */
int array_delete (ARRAY array, const void *key)
{
  return _array_delete (array, key, 1);
}

/**
 * Clears the array from all entries. 
 * @param array - Array to clear
 */
void array_clear (ARRAY array)
{
  size_t n;
  if (array == NULL)
    return;
  if (array->eraser)
    {
      for (n = 0; n < array->nmembs; n++)
	{
	  (*array->eraser) (array->items + n * array->size, NULL);
	}
    }
  array_reallocate (array, 0);
}

/**
 * Frees and destroys the array
 * @param array - Array to destroy
 * @param opaque - Parameter to give for erasers
 */
void array_destroy (ARRAY array, void *opaque)
{
  size_t n;
  if (array == NULL)
    return;
  if (array->eraser)
    {
      /* Call eraser for each element */
      for (n = 0; n < array->nmembs; n++)
	{
	  (*array->eraser) (array->items + n * array->size, opaque);
	}
    }
  /* Reallocate to 0 */
  array_reallocate (array, 0);
  /* Delete array itself */
  free (array);
}

/**
 * Walks all elements on the array with walker
 * Walker must be of void func(void *elem, void *opaque)
 * @param array - Array to walk
 * @param walker - Function to walk with 
 * @param opaque - param to pass to walker function
 */
void array_walk (ARRAY array, array_walker_t walker, void *opaque)
{
  size_t n;
  if (array == NULL)
    return;
  for (n = 0; n < array->nmembs; n++)
    {
      (*walker) (array->items + n * array->size, opaque);
    }
}

/**
 * Returns nth item from array
 * @param array - Array to access
 * @param element - Element index
 * @return nth element
 */
void *array_get_index (ARRAY array, size_t element)
{
  if (array == NULL)
    return NULL;
  if (element < array->nmembs)
    return (void *) (array->items + element * array->size);
  return NULL;
}

/**
 * Tells how many elements in the array
 * @param array - Array to count
 * @return number of elements or 0 if invalid array
 */
size_t array_count (ARRAY array)
{
  if (array == NULL)
    return 0;
  return array->nmembs;
}
