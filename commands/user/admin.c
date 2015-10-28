#include "user.h"

void user_command_admin (NICK * nick, CHANNEL * channel, const char *cmd,
			 const char **args, int argc)
{
  USER *user;
  NICK *victim;
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s nick/#user\r\n", nick->nick, cmd);
      return;
    }
  victim = NULL;
  if (*args[0] == '#')
    {
      if (!(user = user_find (args[0] + 1)))
	{
	  puttext ("NOTICE %s :No such user '%s'\r\n", nick->nick,
		   args[0] + 1);
	  return;
	}
    }
  else if (!(victim = nick_find (args[0])))
    {
      puttext ("NOTICE %s :No such nick '%s'\r\n", nick->nick, args[0]);
      return;
    }
  else
    {
      if (!victim->user)
	{
	  puttext ("NOTICE %s :%s is not known by me\r\n", nick->nick,
		   victim->nick);
	  return;
	}
      user = victim->user;
    }
  user->admin = 1;
  puttext ("NOTICE %s :Granted admin priviledges to %s\r\n", nick->nick,
	   user->user);
  if (victim)
    {
      puttext ("NOTICE %s :You have been granted admin priviledges\r\n",
	       victim->nick);
    }
}

void user_command_unadmin (NICK * nick, CHANNEL * channel, const char *cmd,
			   const char **args, int argc)
{
  USER *user;
  NICK *victim;
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s nick/#user\r\n", nick->nick, cmd);
      return;
    }
  user = NULL;
  victim = NULL;
  if (*args[0] == '#')
    {
      if (!(user = user_find (args[0] + 1)))
	{
	  puttext ("NOTICE %s :No such user '%s'\r\n", nick->nick,
		   args[0] + 1);
	  return;
	}
    }
  else if (!(victim = nick_find (args[0])))
    {
      puttext ("NOTICE %s :No such nick '%s'\r\n", nick->nick, args[0]);
    }
  else
    {
      if (!victim->user)
	{
	  puttext ("NOTICE %s :%s is not known by me\r\n", nick->nick,
		   victim->nick);
	  return;
	}
      user = victim->user;
    }
  user->admin = 0;
  puttext ("NOTICE %s :Revoked admini priviledges from %s\r\n", nick->nick,
	   user->user);
  if (victim)
    {
      puttext ("NOTICE %s :Your admin priviledges have been removed\r\n",
	       victim->nick);
    }
}
