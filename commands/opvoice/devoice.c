#include "hbs.h"

void opvoice_command_devoice (NICK * nick, CHANNEL * channel, const char *cmd,
			      const char **args, int argc)
{
  /* Permissions are bound to be checked */
  int i;
  if (channel->hasop)
    {
      if ((argc > 0)
	  && ((nick->user->admin == 1)
	      || (user_get_channel_modes (nick->user, channel) & USER_OP)))
	{
	  for (i = 0; i < argc; i++)
	    {
	      NICK *n;
	      if ((n = nick_find (args[i])))
		{
		  if (!(nick_get_channel_modes (n, channel) & NICK_VOICE))
		    continue;
		  putmode (channel, "-v", n->nick);
		}
	    }
	}
      else
	{
	  if (!(nick_get_channel_modes (nick, channel) & NICK_VOICE))
	    return;
	  putmode (channel, "-v", nick->nick);
	}
    }
}
