#include "misc.h"

void misc_command_weed (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  irc_weed ();
  puttext ("NOTICE %s :Weed done. Check logs\r\n", nick->nick);
}
