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

struct timer_s
{
  time_t when;
  timer_callback_t callback;
  void *opaque;
} *timers = 0;

// number of timers
size_t ntimers = 0;

/**
 * Registers a one-shot timer
 * @param when - when to run
 * @param cb - callback to use
 * @param opaque - ignored
 * @return 0 ok, -1 fail
 */
int timer_register (time_t when, timer_callback_t cb, void *opaque)
{
  struct timer_s *timer;
  if (when < TIME)
    return -1;
  if (!(timer = realloc (timers, sizeof (struct timer_s) * (ntimers + 1))))
    return -1;
  ntimers++;
  timers = timer;
  timer = timers + ntimers - 1;
  timer->when = when;
  timer->callback = cb;
  timer->opaque = opaque;
  return 0;
}

int timer_verify_addr (void *addr)
{
  Dl_info info;
  if (dladdr (addr, &info) == 0)
    return 1;
  return 0;
}

/**
 * Run timers. Timers are removed while running, and new can be added.
 */
void timers_run (void)
{
  struct timer_s *callbacks;
  size_t n, i;
  time_t now;
  if (ntimers == 0)
    return;
  callbacks = NULL;
  now = TIME;
  n = 0;
  // collect all timers to fire
  for (i = 0; i < ntimers; i++)
    {
      if (timers[i].when < now)
	{
	  n++;
	  callbacks = realloc (callbacks, sizeof (struct timer_s) * n);
	  memcpy (&callbacks[n - 1], &timers[i], sizeof (struct timer_s));
	  if (ntimers > 1)
	    {
	      memmove (timers + i, timers + i + 1,
		       sizeof (struct timer_s) * (ntimers - i - 1));
	      ntimers--;
	    }
	  else
	    {
	      free (timers);
	      timers = NULL;
	      ntimers = 0;
	      break;
	    }
	  i--;
	}
    }
  // reallocate timers
  if (ntimers)
    timers = realloc (timers, sizeof (struct timer_s) * ntimers);
  else
    timers = NULL;
  // run timers
  if (callbacks)
    {
      for (i = 0; i < n; i++)
	if (!timer_verify_addr (callbacks[i].callback))
	  (*callbacks[i].callback) (callbacks[i].opaque);
      free (callbacks);
    }
}
