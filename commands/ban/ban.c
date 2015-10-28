#include "ban.h"

void ban_command_ban (NICK * nick, CHANNEL * channel, const char *cmd,
		      const char **args, int argc)
{
  NICK *victim;
  char *mask;
  size_t i;
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s nick/mask\r\n", nick->nick, cmd);
      return;
    }
  if ((victim = nick_find (args[0])) && (victim != irc_get_me ()))
    {
      mask = nick_create_mask (victim, mask_smart);
    }
  else if (victim == irc_get_me ())
    {
      puttext ("NOTICE %s :I refuse to ban myself\r\n", nick->nick);
      return;
    }
  else
    {
      mask = strdup (args[0]);
    }
  channel_add_iban (channel, mask, nick->user->user, args[1], TIME, TIME + 3600, 0);
  puttext ("NOTICE %s :Banned '%s' on %s for 1 hour\r\n", nick->nick, mask,
	   channel->channel);
  for (i = 0; i < channel->nnicks; i++)
    {
      if (!nick_match_mask (channel->nicks[i].nick, mask))
	{
	  putmode (channel, "+b", mask);
	  break;
	}
    }
}
