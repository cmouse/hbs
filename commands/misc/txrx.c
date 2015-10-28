#include "misc.h"
#include <malloc.h>

void misc_command_bwinfo (NICK * nick, CHANNEL * channel, const char *cmd,
			  const char **args, int argc)
{
  SERVER *server;
  server = server_get_current ();
  puttext ("NOTICE %s :Received %llu bytes / Sent %llu bytes\r\n", nick->nick,
	   server->rx, server->tx);
}
