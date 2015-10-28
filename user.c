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
 * Users are added to array by ptr to avoid problems
 */

ARRAY users = 0;

/**
 * Compares username and user structure
 * @param a - username
 * @param b - user structure
 * @return comparison
 */
int user_cmp (const void *a, const void *b)
{
  const USER *n;
  const char *s;
  n = *(const USER **) b;
  s = (const char *) a;
  return strrfccmp (s, n->user);
}

/**
 * Free up user record
 * @param a - user structure
 * @param o - opaque
 */
void user_free (void *a, void *o)
{
  USER *n;
  n = *(USER **) a;
  nick_remove_user (n);
  free (n->user);
  if (n->channels)
    free (n->channels);
  free (n);
}

/**
 * Create a new user record
 * @param user - username
 * @return user record
 */
USER *user_create (const char *user)
{
  USER **ptr;
  USER *new;
  char *tmp;
  if (users == NULL)
    {
      users = array_create (sizeof (USER *), user_cmp, user_free);
    }
  if (strlen (user) > MAX_USERNAME_LEN)
    {
      tmp = malloc (MAX_USERNAME_LEN + 1);
      tmp[MAX_USERNAME_LEN] = '\0';
      strncpy (tmp, user, MAX_USERNAME_LEN);
    }
  else
    {
      tmp = strdup (user);
    }
  ptr = array_add (users, tmp);
  /* Already fixed */
  if ((*ptr) != NULL)
    {
      free (tmp);
      return *ptr;
    }
  new = (*ptr) = malloc (sizeof (USER));
  memset (new, 0, sizeof (USER));
  new->user = tmp;
  return new;
}

/**
 * Find a user record
 * @param user - Username
 * @return user record or NULL
 */
USER *user_find (const char *user)
{
  USER **ptr;
  if (user == NULL)
    return NULL;
  if ((ptr = array_find (users, user)) == NULL)
    return NULL;
  return *ptr;
}

/** 
 * Walk over all users
 * @param walker - Walker to use
 * @param opaque - Parameter to give for walker
 */
void user_walk (array_walker_t walker, void *opaque)
{
  array_walk (users, walker, opaque);
}

/**
 * Check user's password.
 * @param user - User 
 * @param pw - password unhashed
 * @return 0 for no pw/no match, 1 ok
 */
int user_check_pass (USER * user, const char *pw)
{
  char *sha1;
  sha1 = sha1sum (pw);
  if (user->pass && !memcmp (user->pass, sha1, SHA_DIGEST_LENGTH))
    {
      free (sha1);
      /* User had pw and it matches */
      return 1;
    }
  free (sha1);
  /* No pw or no match */
  return 0;
}

/**
 * Delete user
 * @param user - Username to delete
 */
void user_delete (const char *user)
{
  array_delete (users, user);
}

/**
 * Link user and channel together
 * @param user - User to use
 * @param channel - Channel to use
 * @param modes - Modes nick has
 */
void user_set_channel_modes (USER * user, CHANNEL * channel, int modes)
{
  size_t n;
  if (user == NULL)
    return;
  for (n = 0; n < user->nchannels; n++)
    {
      if (user->channels[n].channel == channel)
	{
	  user->channels[n].modes = modes;
	  return;
	}
    }
  user->nchannels++;
  user->channels = realloc (user->channels,
			    sizeof (struct user_channel_s) * user->nchannels);
  user->channels[user->nchannels - 1].channel = channel;
  user->channels[user->nchannels - 1].modes = modes;
}

/**
 * Get user's channel modes
 * @param user - User to use
 * @param channel - Channel to use
 * @return modes
 */
int user_get_channel_modes (USER * user, CHANNEL * channel)
{
  size_t n;
  if (user == NULL)
    return 0;
  for (n = 0; n < user->nchannels; n++)
    {
      if (user->channels[n].channel == channel)
	{
	  return user->channels[n].modes;
	}
    }
  return 0;
}

/**
 * Unlink user and channel together
 * @param user - User to use
 * @param channel - Channel to use
 */
void user_unset_channel_modes (USER * user, CHANNEL * channel)
{
  size_t n;
  if (user == NULL)
    return;
  for (n = 0; n < user->nchannels; n++)
    {
      if (user->channels[n].channel == channel)
	{
	  if (n < user->nchannels - 1)
	    memmove (&user->channels[n], &user->channels[n + 1],
		     sizeof (struct user_channel_s) * (user->nchannels - n -
						       1));
	  user->nchannels--;
	  if (user->nchannels)
	    {
	      user->channels =
		realloc (user->channels,
			 user->nchannels * sizeof (struct user_channel_s));
	    }
	  else
	    {
	      user->channels = NULL;
	      user->nchannels = 0;
	    }
	  break;
	}
    }
}

/**
 * Load user's channels. 
 * @param user - user structure
 * @aram ptr - xml document
 */
void user_load_channels (USER * user, xmlNodePtr ptr)
{
  char *cptr;
  ptr = ptr->children;
  while (ptr)
    {
      if (ptr->type == XML_ELEMENT_NODE)
	{
	  // channels are one tag with level in param and 
	  // name in tag
	  if (!strcasecmp ((const char *) ptr->name, "channel"))
	    {
	      CHANNEL *c;
	      int flags;
	      flags = s_atoi (xmlGetNodeAttrValue (ptr, "flags"));
	      cptr = xmlNodeGetLat1Value (ptr);
	      if ((c = channel_find (cptr)))
		{
		  user_set_channel_modes (user, c, flags);
		}
	      else
		{
		  print ("user: User %s has unknown channel %s - skipping",
			 user->user, cptr);
		}
	      free (cptr);
	    }
	}
      ptr = ptr->next;
    }
}

/**
 * Load a user
 * @param userptr - XML document
 */
void user_load_user (xmlNodePtr userptr)
{
  USER *user;
  char *cptr;
  xmlNodePtr ptr;
  ptr = xmlGetNodeByName (userptr, "name");
  if (ptr)
    {
      // try to create a user
      cptr = xmlNodeGetLat1Value (ptr);
      if ((user = user_create (cptr)) == NULL)
	{
	  print ("user: Unable to load user %s - skipping", cptr);
	  free (cptr);
	  return;
	}
      free (cptr);
    }
  else
    {
      print ("user: Unable to load user - skipping record");
      return;
    }
  ptr = userptr->children;
  while (ptr)
    {
      if (ptr->type == XML_ELEMENT_NODE)
	{
	  if (!strcasecmp ((const char *) ptr->name, "name"))
	    {
	      /* ignore */
	    }
	  else if (!strcasecmp ((const char *) ptr->name, "password"))
	    {
	      // password
	      if (xmlNodeGetValue (ptr) != NULL)
		hex2bin (user->pass, (const char *) xmlNodeGetValue (ptr));
	    }
	  else if (!strcasecmp ((const char *) ptr->name, "admin"))
	    {
	      // is admin
	      user->admin = (s_atoi (xmlNodeGetValue (ptr)) ? 1 : 0);
	    }
	  else if (!strcasecmp ((const char *) ptr->name, "lastseen"))
	    {
	      // last seen
	      user->lastseen =
		(time_t) s_atoll ((const char *) xmlNodeGetValue (ptr));
	    }
	  else if (!strcasecmp ((const char *) ptr->name, "channels"))
	    {
	      // user's channels
	      user_load_channels (user, ptr);
	    }
	  else if (!strcasecmp ((const char *) ptr->name, "bot"))
	    {
	      // is it a bot
	      user->bot = (s_atoi (xmlNodeGetValue (ptr)) ? 1 : 0);
	    }
	  else
	    {
	      print ("Unknown user property %s - skipping", ptr->name);
	    }
	}
      ptr = ptr->next;
    }
}

/**
 * Load all users from file
 * @param file - XML file to load
 * @return 1 fail, 0 ok
 */
int user_load (const char *file)
{
  xmlDocPtr userfile;
  xmlNodePtr user;

  // try to read file
  if ((userfile =
       xmlReadFile (file, NULL, XML_PARSE_NONET | XML_PARSE_NOCDATA)) == NULL)
    {
      xmlErrorPtr error;
      error = xmlGetLastError ();
      if (error)
	{
	  char *msg, *ch;
	  msg = strdup (error->message);
	  if ((ch = strrchr (msg, '\n')))
	    *ch = '\0';
	  if ((ch = strrchr (msg, '\r')))
	    *ch = '\0';
	  print ("Config: Unable to parse %s: line %d: %s", file, error->line,
		 msg);
	  free (msg);
	}
      else
	{
	  print ("Config: Unable to parse %s", file);
	}
      return 1;
    }

  // start parsing
  user = xmlDocGetRootElement (userfile);
  user = user->children;

  while (user)
    {
      if (user->type == XML_ELEMENT_NODE)
	{
	  // load each user
	  user_load_user (user);
	}
      user = user->next;
    }
  xmlFreeDoc (userfile);
  return 0;
}

/**
 * Writes one user record
 * @param c - User structure
 * @param o - FILE * 
 */
void _user_store (void *c, void *o)
{
  char pass[SHA_DIGEST_LENGTH * 2 + 1];
  USER *user;
  FILE *fout;
  size_t i;
  user = *(USER **) c;
  fout = (FILE *) o;
  // write user start
  fprintf (fout, "\t<user>\n");
  // user's name
  fprintf (fout, "\t\t<name><![CDATA[%s]]></name>\n", user->user);
  // password
  if (strlen (user->pass) > 0)
    {
      memset (pass, 0, sizeof pass);
      bin2hex (pass, user->pass, sizeof user->pass);
      fprintf (fout, "\t\t<password>%s</password>\n", pass);
    }
  // is administrator
  if (user->admin)
    fprintf (fout, "\t\t<admin>%d</admin>\n", user->admin);
  // last seen
  if (user->lastseen)
    fprintf (fout, "\t\t<lastseen>%lu</lastseen>\n", user->lastseen);
  // issit a bit
  if (user->bot)
    fprintf (fout, "\t\t<bot>%d</bot>\n", user->bot);
  // all channels
  if (user->nchannels)
    {
      fprintf (fout, "\t\t<channels>\n");
      for (i = 0; i < user->nchannels; i++)
	fprintf (fout,
		 "\t\t\t<channel flags=\"%d\"><![CDATA[%s]]></channel>\n",
		 user->channels[i].modes, user->channels[i].channel->channel);
      fprintf (fout, "\t\t</channels>\n");
    }
  // end
  fprintf (fout, "\t</user>\n");
}

/**
 * Writes users to XML file
 * @param filename - XML file to create
 * @return 1 fail, 0 ok
 */
int user_store (const char *filename)
{
  FILE *fout;
  if ((fout = fopen (filename, "w")) == NULL)
    return 1;
  // write all data
  fprintf (fout,
	   "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<users>\n");
  array_walk (users, _user_store, fout);
  fprintf (fout, "</users>\n");
  fclose (fout);
  return 0;
}

/**
 * Allows access to user array
 * @return users ARRAY
 */
ARRAY user_get_array (void)
{
  return users;
}
