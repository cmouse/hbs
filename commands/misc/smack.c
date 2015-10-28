#include "misc.h"

void misc_command_smack (NICK * nick, CHANNEL * channel, const char *cmd,
			 const char **args, int argc)
{
  if (argc < 3)
    {
      puttext
	("NOTICE %s :Usage: %s target with something (e.g. a carrot, the sun; '%s lamer with an axe')\r\n",
	 nick->nick, cmd, cmd);
      return;
    }
  if (nick_find (args[2]) == irc_get_me ())
    puttext ("PRIVMSG %s :\001ACTION smacks %s with %s\001\r\n",
	     channel->channel, args[0], nick->nick);
  else
    puttext ("PRIVMSG %s :\001ACTION smacks %s with %s\001\r\n",
	     channel->channel, args[0], args[2]);
}
