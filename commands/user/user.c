#include "user.h"

const struct module_info_s user_info_s;

const char *user_mode_to_level (int level)
{
  if (level & USER_OWNER)
    return "owner";
  if (level & USER_MASTER)
    return "master";
  if (level & USER_OP)
    return "op";
  if (level & USER_VOICE)
    return "voice";
  if (level & USER_BANNED)
    return "banned";
  return "";
}

int user_level_to_mode (const char *level)
{
  if (!strcasecmp (level, "owner"))
    return USER_OWNER | USER_MASTER | USER_OP;
  else if (!strcasecmp (level, "master"))
    return USER_MASTER | USER_OP;
  else if (!strcasecmp (level, "op"))
    return USER_OP;
  else if (!strcasecmp (level, "voice"))
    return USER_VOICE;
  else if (!strcasecmp (level, "lamer"))
    return USER_BANNED;
  else
    return 0;
}

const struct module_info_s VISIBLE *user_info (void)
{
  return &user_info_s;
}

int usermod_load (void)
{
  command_register ("adduser", 1, 0, USER_MASTER, 0, user_command_adduser);
  command_register ("deluser", 1, 0, USER_MASTER, 0, user_command_deluser);
  command_register ("chanlev", 1, 0, USER_VOICE | USER_OP, 0,
		    user_command_chanlev);
  command_register ("lamer", 1, 0, USER_MASTER, 0, user_command_lamer);
  command_register ("admin", 0, 0, 0, 1, user_command_admin);
  command_register ("unadmin", 0, 0, 0, 1, user_command_unadmin);
  command_register ("password", 0, 0, 0, 1, user_command_password);
  console_register_command ("chanlev", 0, user_concommand_chanlev,
			    "Displays channel's access control list");
  console_register_command ("whois", 0, user_command_whois,
			    "Displays user's record");
  return 0;
}

int usermod_unload (void)
{
  command_unregister ("adduser");
  command_unregister ("deluser");
  command_unregister ("chanlev");
  command_unregister ("lamer");
  command_unregister ("admin");
  command_unregister ("unadmin");
  command_unregister ("password");
  console_unregister_command ("whois");
  console_unregister_command ("chanlev");
  return 0;
}

const struct module_info_s user_info_s = {
  "user",
  1,
  0,
  usermod_load,
  usermod_unload
};
