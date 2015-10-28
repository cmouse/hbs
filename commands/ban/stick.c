#include "ban.h"

int ban_on_chan (CHANNEL * channel, const char *mask)
{
  size_t t;
  for (t = 0; t < channel->nbans; t++)
    {
      if (!strrfccmp (mask, channel->bans[t].mask))
	{
	  return 1;
	}
    }
  return 0;
}

void ban_command_stick (NICK * nick, CHANNEL * channel, const char *cmd,
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
		  channel->ibans[t].sticky = 1;
		  dunit = 1;
		  // if ban is not on channel, put it there.
		  if (!ban_on_chan (channel, channel->ibans[t].mask))
		    {
		      putmode (channel, "+b", channel->ibans[t].mask);
		    }
		  puttext ("NOTICE %s :Stuck ban '%s' on %s\r\n", nick->nick,
			   channel->ibans[t].mask, channel->channel);
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
