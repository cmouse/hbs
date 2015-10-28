#include "misc.h"

const struct module_info_s misc_info_s;

const struct module_info_s VISIBLE *misc_info (void)
{
  return &misc_info_s;
}

void misc_command_help (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  if (nick->user)
    {
      puttext ("NOTICE %s :%s\r\n", nick->nick,
	       "http://hbs.help.quakenet.org/manual.php");
    }
}

int misc_load (void)
{
  command_register ("meminfo", 0, 0, 0, 1, misc_command_meminfo);
  command_register ("bwinfo", 0, 0, 0, 1, misc_command_bwinfo);
  command_register ("weed", 0, 0, 0, 1, misc_command_weed);
  command_register ("help", 0, 0, 0, 0, misc_command_help);
  command_register ("params", 0, 0, 0, 1, misc_command_params);
  command_register ("smack", 1, 3, USER_OP | USER_VOICE, 0,
		    misc_command_smack);
  command_register ("auth", 0, 0, 0, 0, misc_command_auth);
  command_register ("google", 0, 1, 0, 0, misc_command_google);
  command_register ("lamercontrol", 1, 0, USER_MASTER, 0,
		    misc_command_lamercontrol);
  command_register ("seen", 0, 0, 0, 0, misc_command_seen);
#ifdef WANT_CALC
  command_register ("calc", 0, 1, USER_OP | USER_VOICE, 0, misc_command_calc);
#endif
  return 0;
}

int misc_unload (void)
{
  command_unregister ("meminfo");
  command_unregister ("bwinfo");
  command_unregister ("weed");
  command_unregister ("help");
  command_unregister ("params");
  command_unregister ("smack");
  command_unregister ("google");
  command_unregister ("auth");
  command_unregister ("lamercontrol");
  command_unregister ("seen");
#ifdef WANT_CALC
  command_unregister ("calc");
#endif
  return 0;
}

const struct module_info_s misc_info_s = {
  "misc",
  1,
  0,
  misc_load,
  misc_unload
};
