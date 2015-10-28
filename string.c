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

#ifndef strdup

/**
 * Duplicates a string
 * @param src - source string
 * @return dst - destination string
 */
char *strdup (const char *src)
{
  char *dst;
  if (src == NULL)
    return NULL;
  dst = malloc (strlen (src) + 1);
  strcpy (dst, src);
  return dst;
}

#endif
