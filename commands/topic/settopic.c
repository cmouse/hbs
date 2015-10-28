#include "hbs.h"

void topic_command_settopic (NICK * nick, CHANNEL * channel, const char *cmd,
			     const char **args, int argc)
{
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage %s topic\r\n", nick->nick, cmd);
      return;
    }
  if (channel->hasop)
    {
      if (chanserv_message (channel, topic, args[0]))
	putserv ("TOPIC %s :%s\r\n", channel->channel, args[0]);
    }
  else
    {
      puttext ("NOTICE %s :I'm not opped on %s\r\n", nick->nick,
	       channel->channel);
    }
}
