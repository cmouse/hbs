#include "whois.h"

void whois_command_whoami (NICK * nick, CHANNEL * channel, const char *cmd,
			   const char **args, int argc)
{
  if (nick->user)
    {
      puttext ("NOTICE %s :You are %s and %s is %s%s\r\n",
	       nick->nick,
	       nick->user->user,
	       nick->user->user,
	       whois_perm (nick->user, channel), whois_admin (nick->user));
    }
  else
    {
      puttext ("NOTICE %s :Who do you think you are?\r\n", nick->nick);
    }
}
