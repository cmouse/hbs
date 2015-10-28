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

log_callback_t *log_callbacks = 0;
size_t nlog_callbacks = 0;

/**
 * Create a time in buffer
 * @param buffer - Where to put time data
 * @param blen - how much will fit
 */
void log_create_time (char *buffer, size_t blen)
{
  time_t t;
  struct tm *tm;
  t = TIME;
  tm = localtime (&t);
  strftime (buffer, blen, "%Y/%m/%d %H:%M:%S", tm);
}

/**
 * Register a new log listener
 * @param cb - listener
 */
void log_register (log_callback_t cb)
{
  nlog_callbacks++;
  log_callbacks =
    realloc (log_callbacks, sizeof (log_callback_t) * nlog_callbacks);
  log_callbacks[nlog_callbacks - 1] = cb;
}

/**
 * Erase all log listeners.
 */
void log_free (void)
{
  if (log_callbacks)
    {
      free (log_callbacks);
    }
  nlog_callbacks = 0;
}

/**
 * Print to all log listeners
 * @param fmt - Format string
 * @param va - format arguments
 */
void vprint (const char *fmt, va_list va)
{
  char tbuf[100];
  va_list tmp;
  char *ch, *nfmt;
  size_t i;
  nfmt = strdup (fmt);
  if ((ch = strrchr (nfmt, '\n')))
    *ch = '\0';
  if ((ch = strrchr (nfmt, '\r')))
    *ch = '\0';
  log_create_time (tbuf, 100);
  for (i = 0; i < nlog_callbacks; i++)
    {
      va_copy (tmp, va);
      (*log_callbacks[i]) (tbuf, fmt, tmp);
      va_end (tmp);
    }
  free (nfmt);
}

/**
 * Print to all log listeners
 * @param fmt - Format string
 * @param ... - format arguments
 */
void print (const char *fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  vprint (fmt, va);
  va_end (va);
}
