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
 * There are several queues here. I have refrained from commenting them
 * because they are rather obvious.
 * 
 * Order of dequeing
 *  - fast
 *  - mode 
 *  - whox
 *  - kick
 *  - server
 *  - text
 */

int server_deque_counter = 0;

struct server_text_queue_s
{
  char *text;
  struct server_text_queue_s *next;
};

struct server_kick_queue_s
{
  char *channel;
  char *nick;
  char *reason;
  struct server_kick_queue_s *next;
};

struct server_whox_queue_s
{
  char *target;
  char *params;
  struct server_whox_queue_s *next;
};

struct server_mode_queue_s
{
  CHANNEL *channel;
  char *mode;
  char *param;
  struct server_mode_queue_s *next;
};

struct server_text_queue_s *server_text_queue_start = 0;
struct server_text_queue_s *server_text_queue_stop = 0;

struct server_text_queue_s *server_normal_queue_start = 0;
struct server_text_queue_s *server_normal_queue_stop = 0;

struct server_kick_queue_s *server_kick_queue_start = 0;
struct server_kick_queue_s *server_kick_queue_stop = 0;

struct server_whox_queue_s *server_whox_queue_start = 0;
struct server_whox_queue_s *server_whox_queue_stop = 0;

struct server_mode_queue_s *server_mode_queue_start = 0;
struct server_mode_queue_s *server_mode_queue_stop = 0;

struct server_text_queue_s *server_fast_queue_start = 0;
struct server_text_queue_s *server_fast_queue_stop = 0;

void
server_queue_text (struct server_text_queue_s **start,
		   struct server_text_queue_s **stop, char *text)
{
  struct server_text_queue_s *new;
  new = malloc (sizeof (struct server_text_queue_s));
  new->text = strdup (text);
  new->next = NULL;
  if (*start == NULL)
    *start = new;
  if (*stop != NULL)
    (*stop)->next = new;
  (*stop) = new;
}

void putfast (const char *fmt, ...)
{
  char message[SERVER_MLEN + 1];
  va_list va;
  va_start (va, fmt);
  vsnprintf (message, SERVER_MLEN + 1, fmt, va);
  // fix missing end
  if (!strchr (message, '\n'))
    {
      print ("WARNING: Missing line terminator for putfast message: %s",
	     message);
      strcat (message, "\r\n");
    }
  va_end (va);
  server_queue_text (&server_fast_queue_start, &server_fast_queue_stop,
		     message);
}

void putwhox (const char *target, const char *params)
{
  struct server_whox_queue_s *new;
  new = server_whox_queue_start;
  // no dups - reduces output
  while (new)
    if (!strrfccmp (target, new->target))
      return;
    else
      new = new->next;
  new = malloc (sizeof (struct server_whox_queue_s));
  new->target = strdup (target);
  new->params = strdup (params);
  new->next = NULL;
  if (server_whox_queue_start == NULL)
    server_whox_queue_start = new;
  if (server_whox_queue_stop)
    server_whox_queue_stop->next = new;
  server_whox_queue_stop = new;
}

void putmode (CHANNEL * channel, char *mode, const char *param)
{
  struct server_mode_queue_s *new;
  /* Check for duplicate mode and having ops */
  new = server_mode_queue_start;
  if (channel->hasop == 0)
    return;
  while (new)
    {
      if ((new->channel == channel) && !strcmp (new->mode, mode) &&
	  (((param == new->param) && (param == NULL)) ||
	   !strrfccmp (new->param, param)))
	return;
      new = new->next;
    }
  new = malloc (sizeof (struct server_mode_queue_s));
  new->mode = strdup (mode);
  if (param)
    new->param = strdup (param);
  else
    new->param = NULL;
  new->channel = channel;
  new->next = NULL;
  if (server_mode_queue_start == NULL)
    server_mode_queue_start = new;
  if (server_mode_queue_stop != NULL)
    server_mode_queue_stop->next = new;
  server_mode_queue_stop = new;
}

void putkick (CHANNEL * channel, NICK * nick, const char *reason)
{
  struct server_kick_queue_s *new;
  /* Ignore operkick */
  if (nick->ircop)
    return;
  /* Ignore me */
  if (nick == irc_get_me ())
    return;
  new = malloc (sizeof (struct server_kick_queue_s));
  new->nick = strdup (nick->nick);
  new->channel = strdup (channel->channel);
  new->reason = strdup (reason);
  new->next = NULL;
  if (server_kick_queue_start == NULL)
    server_kick_queue_start = new;
  if (server_kick_queue_stop != NULL)
    server_kick_queue_stop->next = new;
  server_kick_queue_stop = new;
}

void putserv (const char *fmt, ...)
{
  char message[SERVER_MLEN + 1];
  va_list va;
  va_start (va, fmt);
  vsnprintf (message, SERVER_MLEN + 1, fmt, va);
  if (!strchr (message, '\n'))
    {
      print ("WARNING: Missing line terminator for putserv message: %s",
	     message);
      strcat (message, "\r\n");
    }
  va_end (va);
  server_queue_text (&server_normal_queue_start, &server_normal_queue_stop,
		     message);
}

void puttext (const char *fmt, ...)
{
  char message[SERVER_MLEN + 1];
  va_list va;
  va_start (va, fmt);
  vsnprintf (message, SERVER_MLEN + 1, fmt, va);
  if (!strchr (message, '\n'))
    {
      print ("WARNING: Missing line terminator for puttext message: %s",
	     message);
      strcat (message, "\r\n");
    }
  va_end (va);
  server_queue_text (&server_text_queue_start, &server_text_queue_stop,
		     message);
}

char *server_deque_fast (void)
{
  struct server_text_queue_s *ptr;
  char *rv;
  if ((ptr = server_fast_queue_start) == NULL)
    return NULL;
  rv = ptr->text;
  server_fast_queue_start = ptr->next;
  if (ptr->next == NULL)
    server_fast_queue_stop = NULL;
  free (ptr);
  return rv;
}

struct server_mode_queue_s *_server_deque_mode (CHANNEL * channel)
{
  struct server_mode_queue_s *prv, *ptr, *nxt;
  if ((ptr = server_mode_queue_start) == NULL)
    return NULL;
  prv = NULL;
  while (ptr)
    {
      if ((channel == NULL) || (channel == ptr->channel))
	{
	  nxt = ptr->next;
	  if (ptr == server_mode_queue_start)
	    server_mode_queue_start = nxt;
	  if (ptr == server_mode_queue_stop)
	    server_mode_queue_stop = prv;
	  if (prv)
	    prv->next = nxt;
	  return ptr;
	}
      prv = ptr;
      ptr = ptr->next;
    }
  return ptr;
}

char *server_deque_mode (void)
{
  /* Needs to merge modes */
  struct server_mode_queue_s *mode;
  CHANNEL *channel;
  char message[SERVER_MLEN + 1];
  char modes[MAX_MODES_PER_LINE * 2 + 1];
  char params[MAX_MODEPARAM_LEN + 1];
  char dir;
  int i;
  if (server_mode_queue_start == NULL)
    return NULL;
  memset (modes, 0, sizeof (modes));
  memset (params, 0, sizeof (params));
  dir = '\0';
  channel = NULL;
  for (i = 0; (i < MAX_MODES_PER_LINE) &&
       (strlen (params) < MAX_MODEPARAM_LEN + 1); i++)
    {
      if ((mode = _server_deque_mode (channel)) == NULL)
	break;
      if (channel == NULL)
	channel = mode->channel;
      if (dir != *mode->mode)
	{
	  dir = *mode->mode;
	  strncat (modes, &dir, 1);
	}
      strncat (modes, (mode->mode + 1), 1);
      if (mode->param)
	{
	  strcat (params, mode->param);
	  strcat (params, " ");
	  free (mode->param);
	}
      free (mode->mode);
      free (mode);
    }
  snprintf (message, SERVER_MLEN + 1, "MODE %s %s %s\r\n", channel->channel,
	    modes, params);
  return strdup (message);
}

char *server_deque_whox (void)
{
  char type, *params, *ch;
  char message[SERVER_MLEN + 1];
  int count;
  size_t plen;
  struct server_whox_queue_s *ptr, *next, *prev;
  if ((ptr = server_whox_queue_start) == NULL)
    return NULL;
  type = *ptr->target;
  params = strdup (ptr->params);
  plen = strlen (ptr->params);
  strcpy (message, "WHO ");
  count = 0;
  prev = NULL;
  while (ptr
	 && (strlen (message) + strlen (ptr->target) + plen + 1 <
	     SERVER_MLEN))
    {
      next = ptr->next;
      if (*ptr->target == type)
	{
	  strcat (message, ptr->target);
	  strcat (message, ",");
	  count++;
	  free (ptr->target);
	  free (ptr->params);
	  if (prev)
	    prev->next = next;
	  if (ptr == server_whox_queue_start)
	    server_whox_queue_start = next;
	  if (ptr == server_whox_queue_stop)
	    server_whox_queue_stop = prev;
	  free (ptr);
	  ptr = NULL;
	}
      if (ptr)
	{
	  prev = ptr;
	}
      if ((type == '#') && (count == MAX_CHANNEL_WHOX_PER_LINE))
	break;
      else if (count == MAX_USER_WHOX_PER_LINE)
	break;
      ptr = next;
    }
  if ((ch = strrchr (message, ',')))
    *ch = ' ';
  else
    strcat (message, " ");
  strcat (message, params);
  free (params);
  strcat (message, "\r\n");
  return strdup (message);
}

char *server_deque_kick (void)
{
  char message[SERVER_MLEN + 1];
  struct server_kick_queue_s *ptr;
  if ((ptr = server_kick_queue_start) == NULL)
    return NULL;
  snprintf (message, SERVER_MLEN + 1, "KICK %s %s :%s\r\n",
	    ptr->channel, ptr->nick, ptr->reason);
  free (ptr->nick);
  free (ptr->channel);
  free (ptr->reason);
  server_kick_queue_start = ptr->next;
  if (ptr->next == NULL)
    server_kick_queue_stop = NULL;
  free (ptr);
  return strdup (message);
}

char *server_deque_normal (void)
{
  struct server_text_queue_s *ptr;
  char *rv;
  if ((ptr = server_normal_queue_start) == NULL)
    return NULL;
  rv = ptr->text;
  server_normal_queue_start = ptr->next;
  if (ptr->next == NULL)
    server_normal_queue_stop = NULL;
  free (ptr);
  return rv;
}

char *server_deque_text (void)
{
  struct server_text_queue_s *ptr;
  char *rv;
  if ((ptr = server_text_queue_start) == NULL)
    return NULL;
  rv = ptr->text;
  server_text_queue_start = ptr->next;
  if (ptr->next == NULL)
    server_text_queue_stop = NULL;
  free (ptr);
  return rv;
}

char *server_deque_next_message (void)
{
  char *rv;
  server_deque_counter++;
  if ((rv = server_deque_fast ()) != NULL)
    return rv;
  if (server_deque_counter > 4)
    {
      server_deque_counter = 4;
      if ((rv = server_deque_mode ()) != NULL)
	return rv;
      if ((rv = server_deque_whox ()) != NULL)
	return rv;
      server_deque_counter = 0;
    }
  if ((rv = server_deque_kick ()) != NULL)
    return rv;
  if ((rv = server_deque_normal ()) != NULL)
    return rv;
  if ((rv = server_deque_text ()) != NULL)
    return rv;
  return NULL;
}

void server_queue_clean (void)
{
  struct server_text_queue_s *ptrt, *nxtt;
  struct server_mode_queue_s *ptrm, *nxtm;
  struct server_whox_queue_s *ptrw, *nxtw;
  struct server_kick_queue_s *ptrk, *nxtk;

  ptrt = server_fast_queue_start;
  server_fast_queue_start = server_fast_queue_stop = NULL;
  while (ptrt)
    {
      nxtt = ptrt->next;
      free (ptrt->text);
      free (ptrt);
      ptrt = nxtt;
    }

  ptrm = server_mode_queue_start;
  server_mode_queue_start = server_mode_queue_stop = NULL;
  while (ptrm)
    {
      nxtm = ptrm->next;
      free (ptrm->mode);
      if (ptrm->param)
	free (ptrm->param);
      free (ptrm);
      ptrm = nxtm;
    }

  ptrw = server_whox_queue_start;
  server_whox_queue_start = server_whox_queue_stop = NULL;
  while (ptrw)
    {
      nxtw = ptrw->next;
      free (ptrw->target);
      free (ptrw->params);
      free (ptrw);
      ptrw = nxtw;
    }

  ptrk = server_kick_queue_start;
  server_kick_queue_start = server_kick_queue_stop = NULL;
  while (ptrk)
    {
      nxtk = ptrk->next;
      free (ptrk->reason);
      free (ptrk);
      ptrk = nxtk;
    }

  ptrt = server_normal_queue_start;
  server_normal_queue_start = server_normal_queue_stop = NULL;
  while (ptrt)
    {
      nxtt = ptrt->next;
      free (ptrt->text);
      free (ptrt);
      ptrt = nxtt;
    }

  ptrt = server_text_queue_start;
  server_text_queue_start = server_text_queue_stop = NULL;
  while (ptrt)
    {
      nxtt = ptrt->next;
      free (ptrt->text);
      free (ptrt);
      ptrt = nxtt;
    }
}
