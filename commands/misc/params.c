#include "misc.h"

void misc_command_params (NICK * nick, CHANNEL * channel, const char *cmd,
			  const char **args, int argc)
{
  SERVER *server;
  if (argc < 2)
    {
      puttext ("NOTICE %s :Usage %s param value\r\n", nick->nick, cmd);
      return;
    }
  server = server_get_current ();
  if (!strcasecmp (args[0], "oblines"))
    {
      int nlines;
      nlines = s_atoi (args[1]);
      if (nlines > 0)
	{
	  server->ob.mlines = nlines;
	  puttext ("NOTICE %s :Max lines now %d\r\n", nick->nick, nlines);
	}
      else
	{
	  puttext ("NOTICE %s :Need positive number\r\n", nick->nick);
	}
    }
  else if (!strcasecmp (args[0], "obmsec"))
    {
      int nmsec;
      nmsec = s_atoi (args[1]);
      if (nmsec > 0)
	{
	  server->ob.msec = nmsec;
	  puttext ("NOTICE %s :Max msecs now %d\r\n", nick->nick, nmsec);
	}
      else
	{
	  puttext ("NOTICE %s :Need positive number\r\n", nick->nick);
	}
    }
}
