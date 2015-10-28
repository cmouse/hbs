#include "ban.h"

void ban_command_unstick (NICK * nick, CHANNEL * channel, const char *cmd,
			  const char **args, int argc)
{
  const char *mask;
  int i;
  size_t t;
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s mask [mask mask...]\r\n", nick->nick,
	       cmd);
    }
  else
    {
      int dunit = 0;
      for (i = 0; i < argc; i++)
	{
	  // we need to be more specific on bans
	  for (t = 0; t < channel->nibans; t++)
	    {
	      if (!strrfccmp (args[i], channel->ibans[t].mask))
		{
		  channel->ibans[t].sticky = 0;
		  dunit = 1;
		  puttext ("NOTICE %s :Unstuck ban '%s' on %s\r\n",
			   nick->nick, channel->ibans[t].mask,
			   channel->channel);
		}
	    }
	}
      if (dunit == 0)
	{
	  puttext ("NOTICE %s :No matching ban(s) found on %s\r\n",
		   nick->nick, channel->channel);
	}
    }
}
