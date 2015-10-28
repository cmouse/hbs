#include "hbs.h"

#ifdef ALLOW_VOICE_KICK
# define KICK_CMD_MASK USER_OP|USER_VOICE
#else
# define KICK_CMD_MASK USER_OP
#endif

const struct module_info_s kick_info_s;

void kick_perform_kick (NICK * nick, CHANNEL * channel, const char *target,
			const char *reason)
{
  NICK *victim;
  if ((victim = nick_find (target)))
    {
      if (victim == irc_get_me ())
	{
	  puttext ("NOTICE %s :I refuse to kick myself\r\n", nick->nick);
	  return;
	}
      if (((user_get_channel_modes (victim->user, channel) & USER_OP) ||
	   (nick_get_channel_modes (victim, channel) & NICK_OP)) &&
	  !(user_get_channel_modes (nick->user, channel) & USER_OP))
	{
	  puttext
	    ("NOTICE %s :As voice, you can only kick those without op\r\n",
	     nick->nick);
	  return;
	}
      putkick (channel, victim, reason);
    }
  else
    {
      size_t i;
      for (i = 0; i < channel->nnicks; i++)
	{
	  if (!(channel->nicks[i].modes & NICK_OP)
	      && !nick_match_mask (channel->nicks[i].nick, target))
	    putkick (channel, channel->nicks[i].nick, reason);
	}
    }
}

void kick_command_kick (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  /* masked kick will not work on ops
   * and kick in general won't work on opers.
   */
  const char *reason;
  char *cptr1, *cptr2, *tmp;
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s nick/mask,nick/mask [reason]\r\n",
	       nick->nick, cmd);
      return;
    }
  else if (argc < 2)
    {
      reason = "Goodbye";
    }
  else
    {
      reason = args[1];
    }
  /* Stage 0 */
  tmp = cptr1 = strdup (args[0]);
  while ((cptr2 = strchr (cptr1, ',')))
    {
      *cptr2 = '\0';
      kick_perform_kick (nick, channel, cptr1, reason);
      cptr1 = cptr2 + 1;
    }
  kick_perform_kick (nick, channel, cptr1, reason);
  free (tmp);
  puttext ("NOTICE %s :Done.\r\n", nick->nick);
}

const struct module_info_s VISIBLE *kick_info (void)
{
  return &kick_info_s;
}

int kick_load (void)
{
  command_register ("kick", 1, 2, KICK_CMD_MASK, 0, kick_command_kick);
  return 0;
}

int kick_unload (void)
{
  command_unregister ("kick");
  return 0;
}

const struct module_info_s kick_info_s = {
  "kick",
  1,
  0,
  kick_load,
  kick_unload
};
