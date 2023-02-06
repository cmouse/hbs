#include "ban.h"

int ban_channel_has_iban (CHANNEL * c, const char *mask)
{
  size_t i;
  if (c->ibans)
    {
      for (i = 0; i < c->nibans; i++)
	{
	  if (!rfc_match (c->ibans[i].mask, mask))
	    {
	      return 1;
	    }
	}
    }
  return 0;
}

void ban_command_banlist (NICK * nick, CHANNEL * channel, const char *cmd,
			  const char **args, int argc)
{
  /* read all bans */
  size_t i, t;
  char tbuf[100];
  if ((channel->nibans == 0) && (channel->nbans == 0))
    {
      puttext ("NOTICE %s :No bans on %s\r\n", nick->nick, channel->channel);
      return;
    }
  puttext ("NOTICE %s :Banlist for %s\r\n", nick->nick, channel->channel);
  t = 0;
  if (channel->ibans)
    {
      for (i = 0; i < channel->nibans; i++)
	{
	  if (channel->ibans[i].expires)
	    {
	      strftime (tbuf, sizeof tbuf, "expires %d.%m.%Y %H:%M",
			localtime (&channel->ibans[i].expires));
	    }
	  else
	    {
	      strcpy (tbuf, "permanent");
	    }
	  puttext ("NOTICE %s :[%2zu]: %s by %s (%s)\r\n", nick->nick, ++t,
		   channel->ibans[i].mask, channel->ibans[i].placedby, tbuf);
	}
    }
  else
    {
      channel->nibans = 0;
    }
  if (channel->bans)
    {
      for (i = 0; i < channel->nbans; i++)
	{
	  if (ban_channel_has_iban (channel, channel->bans[i].mask))
	    continue;
	  puttext ("NOTICE %s :[%2zu]: %s by %s\r\n", nick->nick, ++t,
		   channel->bans[i].mask, channel->bans[i].placedby);
	}
    }
  else
    {
      channel->nbans = 0;
    }
}
