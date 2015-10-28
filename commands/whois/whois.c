#include "whois.h"

const struct module_info_s whois_info_s;

const struct module_info_s VISIBLE *whois_info (void)
{
  return &whois_info_s;
}

static const char *whois_levels[] = {
  "channel owner",
  "channel master",
  "channel op",
  "channel voice",
  "peon",
  "banned",
  " (is admin)",
  ""
};

const char *whois_perm (USER * user, CHANNEL * channel)
{
  int perm;
  perm = user_get_channel_modes (user, channel);
  if (perm & USER_OWNER)
    return whois_levels[0];
  else if (perm & USER_MASTER)
    return whois_levels[1];
  else if (perm & USER_OP)
    return whois_levels[2];
  else if (perm & USER_VOICE)
    return whois_levels[3];
  else if (perm & USER_BANNED)
    return whois_levels[5];
  return whois_levels[4];
}

const char *whois_admin (USER * user)
{
  if (user->admin)
    return whois_levels[6];
  else
    return whois_levels[7];
}

void whois_command_whois (NICK * nick, CHANNEL * channel, const char *cmd,
			  const char **args, int argc)
{
  NICK *victim;
  USER *user;
  user = NULL;
  victim = NULL;
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s nick/#user\r\n", nick->nick, cmd);
      return;
    }
  if (*args[0] == '#')
    {
      if (!(user = user_find (args[0] + 1)))
	{
	  puttext ("NOTICE %s :I don't know any %s\r\n", nick->nick,
		   args[0] + 1);
	  return;
	}
    }
  else
    {
      if (!(victim = nick_find (args[0])))
	{
	  puttext ("NOTICE %s :Can't see %s around\r\n", nick->nick, args[0]);
	  return;
	}
      user = victim->user;
    }
  if (user)
    {
      if (user == nick->user)
	{
	  puttext ("NOTICE %s :You are %s and %s is %s%s\r\n",
		   nick->nick,
		   user->user,
		   user->user,
		   whois_perm (user, channel), whois_admin (user));
	}
      else if (victim == NULL)
	{
	  puttext ("NOTICE %s :%s is %s%s\r\n",
		   nick->nick,
		   user->user,
		   whois_perm (user, channel), whois_admin (user));
	}
      else
	{
	  puttext ("NOTICE %s :%s is %s and %s is %s%s\r\n",
		   nick->nick,
		   victim->nick,
		   user->user,
		   user->user,
		   whois_perm (user, channel), whois_admin (user));
	}
    }
  else
    {
      if (nick == victim)
	{
	  puttext ("NOTICE %s :Who do you think you are?\r\n", nick->nick);
	}
      else
	{
	  puttext ("NOTICE %s :Who do you think he is?\r\n", nick->nick);
	}
    }
}

int whois_load (void)
{
  command_register ("whois", 1, 0, 0, 0, whois_command_whois);
  command_register ("whoami", 1, 0, 0, 0, whois_command_whoami);
  return 0;
}

int whois_unload (void)
{
  command_unregister ("whois");
  command_unregister ("whoami");
  return 0;
}

const struct module_info_s whois_info_s = {
  "whois",
  1,
  0,
  whois_load,
  whois_unload
};
