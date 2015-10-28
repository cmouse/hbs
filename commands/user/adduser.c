#include "user.h"

void user_command_adduser (NICK * nick, CHANNEL * channel, const char *cmd,
			   const char **args, int argc)
{
  NICK *victim;
  USER *user;
  int level;
  if (argc < 2)
    {
      puttext ("NOTICE %s :Usage %s nick/#user level\r\n", nick->nick, cmd);
      return;
    }
  if (!(level = user_level_to_mode (args[1])))
    {
      puttext
	("NOTICE %s :Invalid level '%s': Must be one of owner,master,op,voice,lamer\r\n",
	 nick->nick, args[1]);
      return;
    }
  if ((nick->user->admin == 0) && (level & (USER_OWNER | USER_MASTER))
      && !(user_get_channel_modes (nick->user, channel) & USER_OWNER))
    {
      puttext ("NOTICE %s :Only owner can add more owners and master\r\n",
	       nick->nick);
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
		       nick->nick, args[0]);
	      return;
	    }
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
      if ((victim == nick) && (nick->user->admin == 0))
	{
	  puttext ("NOTICE %s :You may not change your own level\r\n",
		   nick->nick);
	  return;
	}
      user_set_channel_modes (user, channel, level);
      puttext ("NOTICE %s :Set %s as %s on %s\r\n", nick->nick, user->user,
	       args[1], channel->channel);
      if (victim != NULL)
	{
	  if (!(level & USER_OP)
	      && (nick_get_channel_modes (victim, channel) & NICK_OP))
	    putmode (channel, "-o", victim->nick);
	  if (!(level & (USER_VOICE | USER_OP))
	      && (nick_get_channel_modes (victim, channel) & NICK_VOICE))
	    putmode (channel, "-v", victim->nick);
	  victim->user->lastseen = TIME;
	  irc_process_nick (victim, 1);
	}
    }
}
