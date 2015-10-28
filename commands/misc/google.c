#include "misc.h"

char *misc_urlencode (const char *src)
{
  char *rv, *ptr;
  rv = malloc (strlen (src) * 3 + 1);
  memset (rv, 0, strlen (src) * 3 + 1);
  ptr = rv;
  for (; *src; src++)
    {
      if (((*src >= 'A') && (*src <= 'Z')) ||
	  ((*src >= 'a') && (*src <= 'z')) ||
	  ((*src >= '0') && (*src <= '9')) ||
	  (*src == '.') || (*src == '_') || (*src == '-'))
	{
	  strncat (ptr, src, 1);
	  ptr++;
	}
      else
	{
	  sprintf (ptr, "%%%02X", *src);
	  ptr += 3;
	}
    }
  return rv;
}

void misc_command_google (NICK * nick, CHANNEL * channel, const char *cmd,
			  const char **args, int argc)
{
  char *google;
  if ((argc < 1) || (args[0] == NULL))
    {
      puttext ("NOTICE %s :Usage: %s keyword(s)\r\n", nick->nick, cmd);
      return;
    }
  google = misc_urlencode (args[0]);
  puttext ("PRIVMSG %s :http://www.google.com/search?q=%s\r\n",
	   (channel ? channel->channel : nick->nick), google);
  free (google);
}
