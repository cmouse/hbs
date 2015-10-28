#include "ban.h"

const char *ban_find_latest_mask (CHANNEL * channel)
{
  time_t last;
  const char *mask;
  size_t i;
  last = 0;
  mask = NULL;
  if (channel->nibans)
    {
      if (last == 0)
	{
	  last = channel->ibans[0].when;
	  mask = channel->ibans[0].mask;
	}
      for (i = 0; i < channel->nibans; i++)
	{
	  if (last > channel->ibans[i].when)
	    {
	      last = channel->ibans[i].when;
	      mask = channel->ibans[i].mask;
	    }
	}
    }
  if (channel->nbans)
    {
      if (last == 0)
	{
	  last = channel->bans[0].when;
	  mask = channel->bans[0].mask;
	}
      for (i = 0; i < channel->nbans; i++)
	{
	  if (last > channel->bans[i].when)
	    {
	      last = channel->bans[i].when;
	      mask = channel->bans[i].mask;
	    }
	}
    }
  return mask;
}

void ban_command_unban (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  const char *mask;
  int i;
  size_t t;
  if (argc < 1)
    {
      /* Remove latest ban */
      if ((mask = ban_find_latest_mask (channel)))
	{
	  putmode (channel, "-b", mask);
	  puttext ("NOTICE %s :Removed ban '%s' from %s\r\n", nick->nick,
		   mask, channel->channel);
	  channel_del_iban (channel, mask);
	}
      else
	{
	  puttext ("NOTICE %s :No bans on %s\r\n", nick->nick,
		   channel->channel);
	}
    }
  else
    {
      for (i = 0; i < argc; i++)
	{
	  // we need to be more specific on bans
	  for (t = 0; t < channel->nbans; t++)
	    {
	      if (!rfc_match (args[i], channel->bans[t].mask))
		{
		  putmode (channel, "-b", channel->bans[t].mask);
		}
	    }
	  channel_del_iban (channel, args[i]);
	  puttext ("NOTICE %s :Removed ban '%s' from %s\r\n", nick->nick,
		   args[i], channel->channel);
	}
    }
}
