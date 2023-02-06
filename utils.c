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
#include "hbs.h"

/**
 * Returns RFC1459 based comparison result
 * @param a - string to compare
 * @param b - string to compare
 * @return an integer less than, equal to, or greater than zero
 * if s1 (or the first n bytes thereof) is found,
 * respectively, to be less than, to match, or be greater than s2.
 */
int strrfccmp (const char *a, const char *b)
{
  for (; *a && *b; a++, b++)
    if (ToUpper (*a) != ToUpper (*b))
      return ToUpper (*a) - ToUpper (*b);
  return ToUpper (*a) - ToUpper (*b);
}

/**
 * match(mask,string)
 * param mask - mask to use
 * param string - string to mask against
 * return 1 if bad, 0 if ok
 */
int match (const char *mask, const char *string)
{
  const char *m = mask, *s = string;
  char ch;
  const char *bm, *bs;		/* Will be reg anyway on a decent CPU/compiler */
  /* Process the "head" of the mask, if any */
  while ((ch = *m++) && (ch != '*'))
    switch (ch)
      {
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	if (tolower (*s) != tolower (ch))
	  return 1;
      case '?':
	if (!*s++)
	  return 1;
      };
  if (!ch)
    return *s;

  /* We got a star: quickly find if/where we match the next char */
got_star:
  bm = m;			/* Next try rollback here */
  while ((ch = *m++))
    switch (ch)
      {
      case '?':
	if (!*s++)
	  return 1;
      case '*':
	bm = m;
	continue;		/* while */
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	goto break_while;	/* C is structured ? */
      };
break_while:
  if (!ch)
    return 0;			/* mask ends with '*', we got it */
  ch = tolower (ch);
  while (tolower (*s++) != ch)
    if (!*s)
      return 1;
  bs = s;			/* Next try start from here */

  /* Check the rest of the "chunk" */
  while ((ch = *m++))
    {
      switch (ch)
	{
	case '*':
	  goto got_star;
	case '\\':
	  if (*m == '?' || *m == '*')
	    ch = *m++;
	default:
	  if (tolower (*s) != tolower (ch))
	    {
	      /* If we've run out of string, give up */
	      if (!*bs)
		return 1;
	      m = bm;
	      s = bs;
	      goto got_star;
	    };
	case '?':
	  if (!*s++)
	    return 1;
	};
    };
  if (*s)
    {
      m = bm;
      s = bs;
      goto got_star;
    };
  return 0;
}

/**
 * rfc_match(mask,string)
 * param mask - mask to use
 * param string - string to mask against
 * return 1 if bad, 0 if ok
 */
int rfc_match (const char *mask, const char *string)
{
  const char *m = mask, *s = string;
  char ch;
  const char *bm, *bs;		/* Will be reg anyway on a decent CPU/compiler */
  /* Process the "head" of the mask, if any */
  while ((ch = *m++) && (ch != '*'))
    switch (ch)
      {
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	if (ToLower (*s) != ToLower (ch))
	  return 1;
      case '?':
	if (!*s++)
	  return 1;
      };
  if (!ch)
    return *s;

  /* We got a star: quickly find if/where we match the next char */
got_star:
  bm = m;			/* Next try rollback here */
  while ((ch = *m++))
    switch (ch)
      {
      case '?':
	if (!*s++)
	  return 1;
      case '*':
	bm = m;
	continue;		/* while */
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	goto break_while;	/* C is structured ? */
      };
break_while:
  if (!ch)
    return 0;			/* mask ends with '*', we got it */
  ch = ToLower (ch);
  while (ToLower (*s++) != ch)
    if (!*s)
      return 1;
  bs = s;			/* Next try start from here */

  /* Check the rest of the "chunk" */
  while ((ch = *m++))
    {
      switch (ch)
	{
	case '*':
	  goto got_star;
	case '\\':
	  if (*m == '?' || *m == '*')
	    ch = *m++;
	default:
	  if (ToLower (*s) != ToLower (ch))
	    {
	      /* If we've run out of string, give up */
	      if (!*bs)
		return 1;
	      m = bm;
	      s = bs;
	      goto got_star;
	    };
	case '?':
	  if (!*s++)
	    return 1;
	};
    };
  if (*s)
    {
      m = bm;
      s = bs;
      goto got_star;
    };
  return 0;
}

/**
 * Returns unsigned long integer matching to mode char
 * @param modechar - mode character to convert
 * @return unsigned long mode
 */
unsigned long modechar2mode (char modechar)
{
  unsigned long modes;
  modes = 0;
  switch (modechar)
    {
    case 'i':
      modes = MODE_i;
      break;
    case 'k':
      modes = MODE_k;
      break;
    case 'l':
      modes = MODE_l;
      break;
    case 'm':
      modes = MODE_m;
      break;
    case 'n':
      modes = MODE_n;
      break;
    case 'p':
      modes = MODE_p;
      break;
    case 's':
      modes = MODE_s;
      break;
    case 't':
      modes = MODE_t;
      break;
    case 'r':
      modes = MODE_r;
      break;
    case 'd':
      modes = MODE_d;
      break;
    case 'D':
      modes = MODE_D;
      break;
    case 'c':
      modes = MODE_c;
      break;
    case 'C':
      modes = MODE_C;
      break;
    case 'N':
      modes = MODE_N;
      break;
    case 'u':
      modes = MODE_u;
      break;
    case 'v':
      modes = MODE_v;
      break;
    case 'o':
      modes = MODE_o;
      break;
    case 'b':
      modes = MODE_b;
      break;
    case 'T':
      modes = MODE_T;
      break;
    }
  return modes;
}

/**
 * Converts mode bitmask to a mode string
 * @param modes - bitmask of modes
 * @return string representation of modes
 */
char *mode2modestr (unsigned long modes)
{
  char *rv;
  rv = malloc (sizeof (char) * (MAX_MODESTRING_LEN + 1));
  rv[0] = '\0';
  if (modes & MODE_i)
    strcat (rv, "i");
  if (modes & MODE_k)
    strcat (rv, "k");
  if (modes & MODE_l)
    strcat (rv, "l");
  if (modes & MODE_m)
    strcat (rv, "m");
  if (modes & MODE_n)
    strcat (rv, "n");
  if (modes & MODE_p)
    strcat (rv, "p");
  if (modes & MODE_s)
    strcat (rv, "s");
  if (modes & MODE_t)
    strcat (rv, "t");
  if (modes & MODE_r)
    strcat (rv, "r");
  if (modes & MODE_D)
    strcat (rv, "D");
  if (modes & MODE_c)
    strcat (rv, "c");
  if (modes & MODE_C)
    strcat (rv, "C");
  if (modes & MODE_N)
    strcat (rv, "N");
  if (modes & MODE_u)
    strcat (rv, "u");
  if (modes & MODE_d)
    strcat (rv, "d");
  if (modes & MODE_v)
    strcat (rv, "v");
  if (modes & MODE_o)
    strcat (rv, "o");
  if (modes & MODE_b)
    strcat (rv, "b");
  if (modes & MODE_T)
    strcat (rv, "T");
  return rv;
}

/**
 * Converts mode string into mode bitmask
 * @param modestr - string of modes
 * @return mode bitmask
 */
unsigned long modestr2mode (const char *modestr)
{
  unsigned long modes;
  modes = 0;
  for (; *modestr; modestr++)
    modes |= modechar2mode (*modestr);
  return modes;
}

char **split_idx2array (const char *idx)
{
  char **rv, *cp;
  if (idx == NULL)
    return NULL;
  rv = calloc (3, sizeof (char *));
  rv[0] = strdup (idx);
  if (!(cp = strchr (rv[0], '!')))
    {
      free (rv[0]);
      free (rv);
      return NULL;
    }
  *cp = '\0';
  cp++;
  rv[1] = cp;
  if (!(cp = strchr (cp, '@')))
    {
      free (rv[0]);
      free (rv);
      return NULL;
    }
  *cp = '\0';
  cp++;
  rv[2] = cp;
  return rv;
}

/**
 * Splits string from whitespace
 * @param message - original string
 * @param count - limit splitting
 * @param args - pointer where to store results
 * @param argc - pointer where to store element count
 * @return 0 for ok, 1 for fail
 */
int string_split (const char *message, int count, char ***args, int *argc)
{
  char **rv, *cp, *ocp;
  int nargs, i;
  if (message == NULL)
    {
      *args = NULL;
      *argc = 0;
      return 0;
    }
  *args = NULL;
  *argc = 0;
  nargs = 1;
  // put the original string into index 0
  rv = malloc (sizeof (char *) * nargs);
  rv[0] = strdup (message);
  ocp = rv[0];
  i = 0;
  // find the next split point
  while (ocp && (count == 0 || i < count - 1) && (cp = strchr (ocp, ' ')))
    {
      while (*cp == ' ')
	{
	  *cp = '\0';
	  cp++;
	}
      // something to do?
      if (*cp)
	{
	  nargs++;
	  rv = realloc (rv, sizeof (char *) * nargs);
	  rv[nargs - 1] = cp;
	  i++;
	  ocp = cp;
	}
      else
	{
	  break;
	}
    }
  // add NULL as last.
  rv = realloc (rv, sizeof (char *) * (nargs + 1));
  rv[nargs] = NULL;
  *args = rv;
  *argc = nargs;
  return 0;
}

/**
 * Generates a random letter
 * @return char
 */
char server_gen_letter (void)
{
  int rval;
  rval = (int) lrand48 () % 64;
  printf ("%d %d %d\n", rval, rval - 26, rval - 9);
  if (rval < 27)
    return 'A' + rval;
  rval -= 26;
  if (rval < 27)
    return 'a' + rval;
  rval -= 9;
  return '0' + rval;
}

/**
 * Replaces ? marks in template with random chars
 * @param template - Template to fill
 * @return fixed template
 */
char *fill_template (const char *template)
{
  char *res, *ch;
  srand48 (TIME);
  res = strdup (template);
  for (ch = res; *ch; ch++)
    if (*ch == '?')
      *ch = server_gen_letter ();
  return res;
}

/**
 * Calculate md5 sum of a string
 * @param src - original string
 * @param srclen - length of string
 * @param dest - where to put sum
 */
void md5sum (const char *src, size_t srclen, char *dest)
{
  unsigned int dstlen __attribute__((unused));
  EVP_Digest(src, srclen, (unsigned char *)dest, &dstlen, EVP_md5(), NULL);
}

/**
 * Calculate sha1 sum of a string
 * @param src - original string
 * @return sha1 sum of string
 */
char *sha1sum (const char *src)
{
  unsigned int dstlen __attribute__((unused));
  char *rv = malloc (SHA_DIGEST_LENGTH);
  EVP_Digest(src, strlen(src), (unsigned char *)rv, &dstlen, EVP_sha1(), NULL);
  return rv;
}

/**
 * Converts binary string to hexadecimal representation
 * @param result - to put result in
 * @param bin - source data
 */
void bin2hex (char *result, const char *bin, size_t len)
{
  char *cp;
  size_t i;
  cp = result;
  memset (result, 0, len * 2 + 1);
  for (i = 0; i < len; i++)
    {
      snprintf (cp, 3, "%02x", bin[i]);
      cp += 2;
    }
}

/**
 * Converts binary string to hexadecimal representation
 * @param result - to put result in
 * @param bin - source data
 */
void hex2bin (char *result, const char *hex)
{
  char *cp;
  size_t i;
  unsigned int val;
  cp = result;
  memset (result, 0, strlen (hex) / 2 + 1);
  for (i = 0; i < strlen (hex); i += 2)
    {
      if (!sscanf (&hex[i], "%02x", &val))
	{
	  return;
	}
      else
	{
	  *cp = (char) val;
	}
      cp++;
    }
}

/**
 * Adds element into a simple array (not ARRAY)
 * @param array - array to use
 * @param elem - element to add
 * @param esize - element size
 * @param ecount - elements in array
 * @return replacement array
 */
void *fixed_array_add (void *array, void *elem, size_t esize, size_t ecount)
{
  if (array == NULL)
    {
      ecount = 1;
    }
  else
    {
      ecount++;
    }
  array = realloc (array, esize * ecount);
  memcpy ((char *) array + esize * (ecount - 1), elem, esize);
  return array;
}

/**
 * Deletes element from a simple array (not ARRAY)
 * @param array - array to use
 * @param esize - element size
 * @param eepos - removal position
 * @param ecount - elements in array
 * @return replacement array
 */
void *fixed_array_delete (void *array, size_t esize, size_t epos,
			  size_t ecount)
{
  if (array == NULL)
    return NULL;
  if (epos > ecount - 1)
    return array;
  if (epos < ecount - 1)
    {
      memmove ((char *) array + esize * epos,
	       (char *) array + esize * (epos + 1),
	       esize * (ecount - epos - 1));
    }
  if (ecount - 1 == 0)
    {
      free (array);
      return NULL;
    }
  return realloc (array, esize * (ecount - 1));
}

#ifndef HAVE_SRANDDEV
/**
 * Supplementary function that seeds the random number generator
 */

void sranddev (void)
{
  /* generate random number seed */
  int i = 0;
  for (i = 0; i < 1000; i++)
    srand (random ());
}

#endif
