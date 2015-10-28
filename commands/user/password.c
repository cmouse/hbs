#include "hbs.h"

void user_command_password (NICK * nick, CHANNEL * channel, const char *cmd,
			    const char **args, int argc)
{
  char *newpw;
  if (channel)
    {
      puttext
	("NOTICE %s :Do not use this command in a channel - logging this so you will get spanked with a wide SCSI cable\r\n",
	 nick->nick);
      print ("SECURITY ALERT: %s[%s] tried to change password on a channel",
	     nick->nick, nick->user->user);
      return;
    }
  if (argc < 2)
    {
      puttext
	("NOTICE %s :Usage: %s [old password] [new password] [new password]\r\n",
	 nick->nick, cmd);
      puttext
	("NOTICE %s :Usage: %s [new password] [new password] (if you do not have a password)\r\n",
	 nick->nick, cmd);
      return;
    }
  if (strlen (nick->user->pass) > 0)
    {
      if (!user_check_pass (nick->user, args[0]))
	{
	  puttext ("NOTICE %s :Old password did not match - event logged\r\n",
		   nick->nick);
	  print
	    ("SECURITY ALERT: %s[%s] tried to change password - wrong old password",
	     nick->nick, nick->user->user);
	  return;
	}
      if ((argc < 3) || strcmp (args[1], args[2]))
	{
	  puttext ("NOTICE %s :New passwords did not match\r\n", nick->nick);
	  return;
	}
    }
  else
    {
      if (strcmp (args[0], args[1]))
	{
	  puttext ("NOTICE %s :New passwords did not match\r\n", nick->nick);
	  return;
	}
    }
  newpw = sha1sum (args[1]);
  memcpy (nick->user->pass, newpw, SHA_DIGEST_LENGTH);
  puttext ("NOTICE %s :Password changed\r\n", nick->nick);
  print ("%s!%s@%s[%s] changed password", nick->nick, nick->ident, nick->host,
	 nick->user->user);
  free (newpw);
}
