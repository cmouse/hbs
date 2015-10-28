#include "hbs.h"

void opvoice_command_op (NICK * nick, CHANNEL * channel, const char *cmd,
			 const char **args, int argc)
{
  /* Permissions are bound to be checked */
  int i;
  if (channel->hasop)
    {
      if (argc > 0)
	{
	  for (i = 0; i < argc; i++)
	    {
	      NICK *n;
	      if ((n = nick_find (args[i])))
		{
		  if ((nick_get_channel_modes (n, channel) & NICK_OP))
		    continue;
		  putmode (channel, "+o", n->nick);
		}
	    }
	}
      else
	{
	  if ((nick_get_channel_modes (nick, channel) & NICK_OP))
	    return;
	  putmode (channel, "+o", nick->nick);
	}
    }
}
