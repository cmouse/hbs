#include "chanmod.h"

const struct module_info_s chanmod_info_s;

const struct module_info_s VISIBLE *chanmod_info (void)
{
  return &chanmod_info_s;
}

int chanmodmod_load (void)
{
  command_register ("addchan", 0, 0, 0, 1, chanmod_command_addchan);
  command_register ("chaninfo", 1, 0, USER_MASTER, 0,
		    chanmod_command_chaninfo);
  command_register ("delchan", 1, 0, USER_OWNER, 0, chanmod_command_delchan);
  command_register ("setchan", 1, 0, USER_MASTER, 0, chanmod_command_setchan);
  return 0;
}

int chanmodmod_unload (void)
{
  command_unregister ("addchan");
  command_unregister ("chaninfo");
  command_unregister ("delchan");
  command_unregister ("setchan");
  return 0;
}

const struct module_info_s chanmod_info_s = {
  "chanmod",
  1,
  0,
  chanmodmod_load,
  chanmodmod_unload
};
