#include "chanmod.h"

void chanmod_command_addchan (NICK * nick, CHANNEL * channel, const char *cmd,
			      const char **args, int argc)
{
  CHANNEL *c;
  USER *user;
  NICK *victim;
  if (argc < 2)
    {
      puttext ("NOTICE %s :Usage: %s #channel nick/#user [key]\r\n",
	       nick->nick, cmd);
      return;
    }
  if ((*args[0] != '#') && (*args[0] != '&') && (*args[0] != '+'))
    {
      puttext ("NOTICE %s :Invalid channel name %s\r\n", nick->nick, args[0]);
      return;
    }
  if (channel_find (args[0]))
    {
      puttext ("NOTICE %s :Channel %s already added\r\n", nick->nick,
	       args[0]);
      return;
    }
  victim = NULL;
  if (*args[1] == '#')
    {
      if (!(user = user_find (args[1] + 1)))
	{
	  puttext ("NOTICE %s :No such user '%s'\r\n", nick->nick,
		   args[1] + 1);
	  return;
	}
    }
  else if (!(victim = nick_find (args[1])))
    {
      puttext ("NOTICE %s :No such nick '%s'\r\n", nick->nick, args[1]);
      return;
    }
  else
    {
      if (!victim->user)
	{
	  if (victim->acct)
	    {
	      if (!(victim->user = user_create (victim->acct)))
		{
		  puttext
		    ("NOTICE %s :Unable to create user '%s' - contant admin\r\n",
		     nick->nick, nick->acct);
		  return;
		}
	    }
	  else
	    {
	      puttext ("NOTICE %s :Please ask %s to authenticate to Q\r\n",
		       nick->nick, args[1]);
	      return;
	    }
	}
      user = victim->user;
    }
  if (!(c = channel_create (args[0])))
    {
      puttext ("NOTICE %s :Unable to add channel %s\r\n", nick->nick,
	       args[0]);
      return;
    }
  c->created = TIME;
  c->lastop = TIME;
  c->laston = TIME;
  c->lastjoin = TIME;
  if (argc > 2)
    c->key = strdup (args[2]);
  channel_set_service (c, CHANNEL_SERV_ENFORCEBANS);
  user_set_channel_modes (user, c, USER_OWNER | USER_MASTER | USER_OP);
  if (argc > 2)
    putserv ("JOIN %s %s\r\n", c->channel, c->key);
  else
    putserv ("JOIN %s\r\n", c->channel);
  puttext ("NOTICE %s :Channel %s added with %s as owner\r\n", nick->nick,
	   c->channel, user->user);
}
