#include "misc.h"

void misc_command_auth (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  if (nick->user == NULL)
    {
      puttext ("NOTICE %s :Attempting to authenticate you...\r\n",
	       nick->nick);
      putwhox (nick->nick, "n%%cuhnfa");
    }
  else
    {
      puttext ("NOTICE %s :You are already authenticated\r\n", nick->nick);
    }
}
