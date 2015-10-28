#include "misc.h"

void misc_create_time (char *buffer, size_t blen, time_t time)
{
  struct tm *tm;
  tm = localtime (&time);
  strftime (buffer, blen, "%Y/%m/%d %H:%M:%S", tm);
}


void misc_command_seen (NICK * nick, CHANNEL * c, const char *cmd,
			const char **args, int argc)
{
  USER *u;
  char ts[100];
  if (!argc)
    {
      puttext ("NOTICE %s :Usage: %s username\r\n", nick->nick, cmd);
      return;
    }
  if (!(u = user_find (args[0])))
    {
      if (c)
	{
	  puttext ("PRIVMSG %s :I don't know who '%s' is\r\n",
		   c->channel, args[0]);
	}
      else
	{
	  puttext ("NOTICE %s :I don't know who '%s' is\r\n",
		   nick->nick, args[0]);
	}
      return;
    }
  if (u == nick->user)
    {
      if (c)
	{
	  puttext ("PRIVMSG %s :How about looking in the mirror?\r\n",
		   c->channel);
	}
      else
	{
	  puttext ("NOTICE %s :How about looking in the mirror\r\n",
		   nick->nick);
	}
      return;
    }
  if (u->lastseen == (time_t) 0)
    {
      if (c)
	{
	  puttext ("PRIVMSG %s :%s hasn't been spotted\r\n",
		   c->channel, u->user);
	}
      else
	{
	  puttext ("NOTICE %s :%s hasn't been spotted\r\n",
		   nick->nick, u->user);
	}
      return;
    }
  misc_create_time (ts, sizeof ts, u->lastseen);
  if (c)
    {
      puttext ("PRIVMSG %s :%s was last spotted %s\r\n",
	       c->channel, u->user, ts);
    }
  else
    {
      puttext ("NOTICE %s :%s was last spotted %s\r\n",
	       nick->nick, u->user, ts);
    }
}
