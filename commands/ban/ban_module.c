#include "ban.h"

const struct module_info_s ban_info_s;

const struct module_info_s VISIBLE *ban_info (void)
{
  return &ban_info_s;
}

int ban_load (void)
{
  command_register ("ban", 1, 2, USER_OP, 0, ban_command_ban);
  command_register ("permban", 1, 2, USER_OP, 0, ban_command_permban);
  command_register ("unban", 1, 0, USER_OP, 0, ban_command_unban);
  command_register ("banlist", 1, 0, USER_OP, 0, ban_command_banlist);
  command_register ("stick", 1, 0, USER_OP, 0, ban_command_stick);
  command_register ("unstick", 1, 0, USER_OP, 0, ban_command_unstick);
  return 0;
}

int ban_unload (void)
{
  command_unregister ("ban");
  command_unregister ("unban");
  command_unregister ("permban");
  command_unregister ("banlist");
  command_unregister ("stick");
  command_unregister ("unstick");
  return 0;
}

const struct module_info_s ban_info_s = {
  "ban",
  1,
  0,
  ban_load,
  ban_unload
};
