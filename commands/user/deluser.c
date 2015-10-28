#include "user.h"

void user_command_deluser (NICK * nick, CHANNEL * channel, const char *cmd,
			   const char **args, int argc)
{
  NICK *victim;
  USER *user;
  int i;
  if (argc < 1)
    {
      puttext
	("NOTICE %s :Usage %s nick/#user [nick/#user] [nick/#user]...\r\n",
	 nick->nick, cmd);
      return;
    }
  for (i = 0; i < argc; i++)
    {
      victim = NULL;
      if (*args[i] == '#')
	{
	  if (!(user = user_find (args[i] + 1)))
	    {
	      puttext ("NOTICE %s :No such user '%s'\r\n", nick->nick,
		       args[i] + 1);
	      continue;
	    }
	}
      else if (!(victim = nick_find (args[i])))
	{
	  puttext ("NOTICE %s :No such nick '%s'\r\n", nick->nick, args[i]);
	  continue;
	}
      else
	{
	  if (!victim->user)
	    {
	      puttext ("NOTICE %s :%s is not known by me\r\n", nick->nick,
		       victim->nick);
	      continue;
	    }
	  user = victim->user;
	}
      if ((nick->user->admin == 0)
	  && (user_get_channel_modes (user, channel) & USER_OWNER)
	  && !(user_get_channel_modes (nick->user, channel) & USER_OWNER))
	{
	  puttext ("NOTICE %s :Only owner can modify other owners\r\n",
		   nick->nick);
	}
      else
	{
	  user_unset_channel_modes (user, channel);
	  puttext ("NOTICE %s :Removed %s from %s\r\n", nick->nick,
		   user->user, channel->channel);
	  if (victim != NULL)
	    {
	      if (nick_get_channel_modes (victim, channel) & NICK_OP)
		putmode (channel, "-o", victim->nick);
	      if (nick_get_channel_modes (victim, channel) & NICK_VOICE)
		putmode (channel, "-v", victim->nick);
	    }
	}
    }
}
