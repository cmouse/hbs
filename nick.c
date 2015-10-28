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

/* 
 * Nicks are added to array by ptr to avoid problems
 */
ARRAY nicks = 0;

/**
 * Compares nickname and nick struct
 * @param a - nickname
 * @param b - nick structure
 * @return comparison
 */
int nick_cmp (const void *a, const void *b)
{
  const NICK *n;
  const char *s;
  n = *(const NICK **) b;
  s = (const char *) a;
  return strrfccmp (n->nick, s);
}

/**
 * Cleans up a nick structure
 * @param a - nick structure
 * @param o - ignored
 */
void nick_free (void *a, void *o)
{
  NICK *n;
  n = *(NICK **) a;
  if (n->host)
    free (n->host);
  if (n->ident)
    free (n->ident);
  if (n->acct)
    free (n->acct);
  // free everything
  if (n->channels)
    {
      size_t i;
      for (i = 0; i < n->nchannels; i++)
	channel_unset_nick_modes (n->channels[i].channel, n);
      free (n->channels);
    }
  free (n->nick);
  free (n);
}

/**
 * Rename nick. 
 * @param nick - old nick structure
 * @param newnick - new nickname
 */
void nick_rename (NICK * nick, const char *newnick)
{
  NICK **ptr;
  /* drop nick */
  array_drop (nicks, nick->nick);
  free (nick->nick);
  /* rename entry */
  nick->nick = strdup (newnick);
  /* put it back in */
  ptr = array_add (nicks, nick->nick);
  /* Collision? Remove old */
  if (*ptr)
    {
      print ("nick: in nick_rename(): nick collision");
      nick_free (ptr, NULL);
    }
  /* Update ptr */
  *ptr = nick;
}

/**
 * Creates a new nick based on nickname.
 * @param nick - nickname to use
 * @return nick struct
 */
NICK *nick_create (const char *nick)
{
  NICK **ptr;
  NICK *new;
  if (nicks == NULL)
    {
      nicks = array_create (sizeof (NICK *), nick_cmp, nick_free);
    }
  ptr = array_add (nicks, nick);
  /* Already fixed */
  if ((*ptr) != NULL)
    return *ptr;
  new = (*ptr) = malloc (sizeof (NICK));
  memset (new, 0, sizeof (NICK));
  new->nick = strdup (nick);
  return new;
}

/**
 * Try to locate nick 
 * @param nick - nickname to look for
 * @return NULL or nick struct
 */
NICK *nick_find (const char *nick)
{
  NICK **ptr;
  if ((ptr = array_find (nicks, nick)) == NULL)
    return NULL;
  return *ptr;
}

/**
 * Delete nick 
 * @param nick - nickname to delete
 */
void nick_delete (const char *nick)
{
  array_delete (nicks, nick);
}

/**
 * Delete all nicks.
 */
void nick_flush (void)
{
  /* Fast and efficient */
  array_clear (nicks);
}

/**
 * Link nick and channel together
 * @param nick - Nick to use
 * @param channel - Channel to use
 * @param modes - Modes nick has
 */
void nick_set_channel_modes (NICK * nick, CHANNEL * channel, int modes)
{
  size_t n;
  for (n = 0; n < nick->nchannels; n++)
    {
      if (nick->channels[n].channel == channel)
	{
	  nick->channels[n].modes = modes;
	  return;
	}
    }
  nick->nchannels++;
  nick->channels = realloc (nick->channels,
			    sizeof (struct nick_channel_s) * nick->nchannels);
  nick->channels[nick->nchannels - 1].channel = channel;
  nick->channels[nick->nchannels - 1].modes = modes;
  memset (&nick->channels[nick->nchannels - 1].lamer, 0,
	  sizeof (struct lamer_control_s));
}

/**
 * Get channel info for nick
 * @param nick - Nick to use
 * @param channel - Channel to use
 */
int nick_get_channel_modes (NICK * nick, CHANNEL * channel)
{
  size_t n;
  for (n = 0; n < nick->nchannels; n++)
    {
      if (nick->channels[n].channel == channel)
	{
	  return nick->channels[n].modes;
	}
    }
  return 0;
}

/**
 * Unlink nick and channel together
 * @param nick - Nick to use
 * @param channel - Channel to use
 * @param modes - Modes nick has
 */
void nick_unset_channel_modes (NICK * nick, CHANNEL * channel)
{
  size_t n;
  for (n = 0; n < nick->nchannels; n++)
    {
      if (nick->channels[n].channel == channel)
	{
	  if (n < nick->nchannels - 1)
	    memmove (&nick->channels[n], &nick->channels[n + 1],
		     sizeof (struct nick_channel_s) * (nick->nchannels - n -
						       1));
	  nick->nchannels--;
	  nick->channels =
	    realloc (nick->channels,
		     nick->nchannels * sizeof (struct nick_channel_s));
	}
    }
}

/**
 * Create a mask from nick using masktype.
 * @param n - Nick
 * @param masktype - type of mask
 * @return NULL or mask
 */
char *nick_create_mask (NICK * n, nick_masktype_t masktype)
{
  struct in_addr inp;
  char *ch;
  char *mask;
  int count, i;
  size_t len;
  mask = NULL;
  // no mask can be made
  if (!n || !n->nick || !n->ident || !n->host)
    return mask;
  switch (masktype)
    {
      // normal strict mask
    case mask_nuh:
      len = strlen (n->nick) + strlen (n->ident) + strlen (n->host) + 3;
      mask = malloc (len + 1);
      snprintf (mask, len, "%s!%s@%s", n->nick, n->ident, n->host);
      break;
      // per host mask
    case mask_h:
      len = strlen (n->host) + 6;
      mask = malloc (len + 1);
      snprintf (mask, len, "*!*@%s", n->host);
      break;
      // intelligent mask
    case mask_smart:
      // account based
      if (n->acct)
	{
	  len = strlen (n->acct) + strlen (NICK_AUTHED_HOSTMASK) + 6;
	  mask = malloc (len);
	  snprintf (mask, len, "*!*@%s%s", n->acct, NICK_AUTHED_HOSTMASK);
	  return mask;
	}
      // Q account based
      if (strstr (n->host, NICK_AUTHED_HOSTMASK))
	{
	  len = strlen (n->host) + 6;
	  mask = malloc (len);
	  snprintf (mask, len, "*!*@%s", n->host);
	  break;
	}
      // IP BASED CHECK
      if (inet_aton (n->host, &inp))
	{
	  len = strlen (n->host) + 6;
	  mask = malloc (len);
	  snprintf (mask, len, "*!*@%s", n->host);
	  break;
	}
      // user @ domain based
      ch = n->host;
      count = 0;
      while ((ch = strchr (ch, '.')))
	{
	  ch++;
	  count++;
	}
      if (count < 2)
	{
	  len = strlen (n->ident) + strlen (n->host) + 6;
	  mask = malloc (len);
	  snprintf (mask, len, "*!%s@%s", n->ident, n->host);
	}
      else
	{
	  ch = n->host;
	  for (i = 0; i < count - 1; i++)
	    {
	      ch = strchr (ch, '.');
	      ch++;
	    }
	  len = strlen (n->ident) + strlen (ch) + 7;
	  mask = malloc (len);
	  snprintf (mask, len, "*!%s@*.%s", n->ident, ch);
	}
    }
  return mask;
}

/**
 * Check if nick matches given mask
 * @param n - Nick
 * @param mask - Mask to match
 * @return 1 fail, 0 match
 */
int nick_match_mask (NICK * n, const char *mask)
{
  char user[MAX_USERNAME_LEN + 1];
  char *nuh;
  int rv;
  if (!n)
    return 1;
  // create a mask
  if (!(nuh = nick_create_mask (n, mask_nuh)))
    return 1;
  *user = '\0';
  if (strstr (mask, NICK_AUTHED_HOSTMASK))
    {
      sscanf (mask, "%*[^@]@%[^.]", user);
    }
  // check if user matches mask OR if mask has account
  // to the given Q account
  if ((*user != '\0') && (n->acct) && !strrfccmp (n->acct, user))
    {
      free (nuh);
      return 0;
    }
  rv = rfc_match (mask, nuh);
  free (nuh);
  return rv;
}

/**
 * Makes all nicks user accounts lastseen to be updated.
 * @param elem - Nick 
 * @param opaque - ignored
 */
void nick_toucher (void *elem, void *opaque)
{
  NICK *nick;
  nick = *(NICK **) elem;
  if (nick->user)
    nick->user->lastseen = TIME;
}

/**
 * Remove username from nick
 * @param elem - Nick
 * @param opaque - Username
 */
void nick_user_remover (void *elem, void *opaque)
{
  NICK *nick;
  USER *user;
  nick = *(NICK **) elem;
  user = (USER *) opaque;
  if (nick->user == user)
    nick->user = NULL;
}

/**
 * Touches all authed users
 */
void nick_touch_users (void)
{
  array_walk (nicks, nick_toucher, NULL);
}

/**
 * Removes user account from all nicks.
 */
void nick_remove_user (USER * u)
{
  array_walk (nicks, nick_user_remover, u);
}
