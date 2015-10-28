#include "hbs.h"

const struct module_info_s invite_info_s;

void invite_command_invite (NICK * nick, CHANNEL * channel, const char *cmd,
			    const char **args, int argc)
{
  putserv ("INVITE %s %s\r\n", nick->nick, channel->channel);
}

const struct module_info_s VISIBLE *invite_info (void)
{
  return &invite_info_s;
}

int invite_load (void)
{
  command_register ("invite", 1, 0, USER_VOICE | USER_OP, 0,
		    invite_command_invite);
  return 0;
}

int invite_unload (void)
{
  command_unregister ("invite");
  return 0;
}

const struct module_info_s invite_info_s = {
  "invite",
  1,
  0,
  invite_load,
  invite_unload
};
