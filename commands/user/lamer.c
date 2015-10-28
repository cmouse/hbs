#include "user.h"

void user_command_lamer (NICK * nick, CHANNEL * channel, const char *cmd,
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
	      if (victim->acct)
		{
		  if (!(victim->user = user_create (victim->acct)))
		    {
		      puttext
			("NOTICE %s :Unable to create user '%s' - contant admin\r\n",
			 nick->nick, nick->acct);
		      continue;
		    }
		}
	      else
		{
		  puttext ("NOTICE %s :%s is not authenticated to Q\r\n",
			   nick->nick, victim->nick);
		  continue;
		}
	    }
	  user = victim->user;
	}
      if ((user == nick->user) && (nick->user->admin == 0))
	{
	  puttext ("NOTICE %s :You may not change your own level\r\n",
		   nick->nick);
	  continue;
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
	  user_set_channel_modes (user, channel, USER_BANNED);
	  puttext ("NOTICE %s :Set %s as lamer on %s\r\n", nick->nick,
		   user->user, channel->channel);
	  if (victim != NULL)
	    {
	      if (nick_get_channel_modes (victim, channel) & NICK_OP)
		putmode (channel, "-o", victim->nick);
	      if (nick_get_channel_modes (victim, channel) & NICK_VOICE)
		putmode (channel, "-v", victim->nick);
	      irc_process_nick (victim, 1);
	    }
	}
    }
}
