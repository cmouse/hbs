#include "chanmod.h"

void chanmod_delchan_walker (void *elem, void *opaque)
{
  CHANNEL *channel;
  USER *user;
  user = *(USER **) elem;
  channel = (CHANNEL *) opaque;
  user_unset_channel_modes (user, channel);
}

void chanmod_command_delchan (NICK * nick, CHANNEL * channel, const char *cmd,
			      const char **args, int argc)
{
  char *c;
  if ((channel->permanent != 0) ||
      ((nick->user->admin == 0) &&
       !(user_get_channel_modes (nick->user, channel) & USER_OWNER)))
    {
      puttext ("NOTICE %s :You cannot remove channel %s\r\n", nick->nick,
	       channel->channel);
      return;
    }
  c = strdup (channel->channel);
  channel_delete (c);
  puttext ("NOTICE %s :Deleted channel %s\r\n", nick->nick, c);
  free (c);
}
