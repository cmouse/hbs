#include "misc.h"

#ifdef WANT_CALC
void misc_command_calc (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  FILE *i;
  char cmdline[512] = { 0 };
  char solution[300] = { 0 };
  char fn[] = "/tmp/hbsXXXXXXXX";
  char *cp;
  int fd, lc;
  if (argc)
    {
      fd = mkstemp (fn);
      if (fd < 0)
	{
	  puttext ("NOTICE %s :Internal system error - see logs\r\nn",
		   nick->nick);
	  print ("misc_command_calc(): mkstemp(): %s", strerror (errno));
	  unlink (fn);
	  return;
	}
      write (fd, args[0], strlen (args[0]));
      write (fd, "\n", 1);
      close (fd);
      snprintf (cmdline, sizeof cmdline, "/usr/bin/bc -ql math.bc < %s 2>&1", fn);
      i = popen (cmdline, "r");
      lc = 0;
      while (fgets (solution, sizeof solution, i) && (lc++ < 4))
	{
	  if ((cp = strchr (solution, '\n')))
	    *cp = '\0';
	  if ((cp = strchr (solution, '\r')))
	    *cp = '\0';
	  if (channel)
	    {
	      puttext ("PRIVMSG %s :%s\r\n", channel->channel, solution);
	    }
	  else
	    {
	      puttext ("NOTICE %s :%s\r\n", nick->nick, solution);
	    }
	}
      pclose (i);
      unlink (fn);
    }
  else
    {
      if (channel)
	{
	  puttext ("PRIVMSG %s :Usage: %s equation\r\n", channel->channel,
		   cmd);
	}
      else
	{
	  puttext ("NOTICE %s :Usage: %s equation\r\n", nick->nick, cmd);
	}
    }
}
#endif
