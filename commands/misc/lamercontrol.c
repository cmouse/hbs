#include "misc.h"

void misc_command_lamercontrol (NICK * nick, CHANNEL * c, const char *cmd,
				const char **args, int argc)
{
  if (argc < 2)
    {
      puttext
	("NOTICE %s :Current lamercontrol settings: %d lines or %d repeats in %d seconds - To change use %s param value (reset,maxlines,maxrepeat)\r\n",
	 nick->nick, c->lamercontrol.maxlines, c->lamercontrol.maxrepeat,
	 c->lamercontrol.maxtime, cmd);
      return;
    }
  if (!strcasecmp (args[0], "maxlines"))
    {
      int nlines;
      nlines = s_atoi (args[1]);
      if (nlines > 0)
	{
	  c->lamercontrol.maxlines = nlines;
	  puttext ("NOTICE %s :Max lines now %d\r\n", nick->nick, nlines);
	}
      else
	{
	  puttext ("NOTICE %s :Need positive number\r\n", nick->nick);
	}
    }
  else if (!strcasecmp (args[0], "reset"))
    {
      int nsec;
      nsec = s_atoi (args[1]);
      if (nsec > 0)
	{
	  c->lamercontrol.maxtime = nsec;
	  puttext ("NOTICE %s :Reset time now %d seconds\r\n", nick->nick,
		   nsec);
	}
      else
	{
	  puttext ("NOTICE %s :Need positive number\r\n", nick->nick);
	}
    }
  else if (!strcasecmp (args[0], "maxrepeat"))
    {
      int nlines;
      nlines = s_atoi (args[1]);
      if (nlines > 0)
	{
	  c->lamercontrol.maxrepeat = nlines;
	  puttext ("NOTICE %s :Max repeated lines now %d\r\n", nick->nick,
		   nlines);
	}
      else
	{
	  puttext ("NOTICE %s :Need positive number\r\n", nick->nick);
	}
    }
  else
    {
      puttext
	("NOTICE %s :Invalid parameter %s. Must be one of: maxlines, maxrepeat, reset\r\n",
	 nick->nick, args[0]);
    }
}
