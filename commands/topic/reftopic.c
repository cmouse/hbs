#include "hbs.h"

void topic_command_reftopic (NICK * nick, CHANNEL * channel, const char *cmd,
			     const char **args, int argc)
{
  if (channel->topic)
    {
      if (channel->hasop)
	{
	  if (chanserv_message (channel, topic, channel->topic))
	    putserv ("TOPIC %s :%s\r\n", channel->channel, channel->topic);
	  puttext ("NOTICE %s :Done.\r\n", nick->nick);
	}
      else
	{
	  puttext ("NOTICE %s :I'm not opped on %s\r\n", nick->nick,
		   channel->channel);
	}
    }
  else
    {
      puttext ("NOTICE %s :I don't know the topic\r\n", nick->nick);
    }
}
