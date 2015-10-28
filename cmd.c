/* (c) Aki Tuomi 2005

This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
/*
 * Command handlers 
 *
 * Command handler callback 
 * void handler(CHANNEL*,NICK*,const char*,const char**,int);
 * as in, channel, nick, command, arguments, argumentcount
 */

#include "hbs.h"

ARRAY commands = 0;

// Internal command structure
struct command_s
{
  // command name
  char *command;
  // does it want a channel
  int needchan;
  // does it want parameters (as in how much, 0 means split all)
  int params;
  // required channel access
  int umask;
  // does it require admin priviledges
  int admin;
  // is it disabled
  int disabled;
  // who handles it
  command_handler_t handler;
};

// Our trigger
const char *trigger;

void
command_enable_command (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  struct command_s *handler;
  // show usage
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s command\r\n", nick->nick, cmd);
      return;
    }
  // find handler
  if (!(handler = array_find (commands, args[0])))
    {
      puttext ("NOTICE %s :Command '%s' not known\r\n", nick->nick, args[0]);
    }
  else
    {
      // enable it
      handler->disabled = 0;
      puttext ("NOTICE %s :Command '%s' has been enabled\r\n", nick->nick,
	       handler->command);
    }
}

void
command_disable_command (NICK * nick, CHANNEL * channel, const char *cmd,
			 const char **args, int argc)
{
  struct command_s *handler;
  // show usage
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s command\n", nick->nick, cmd);
      return;
    }
  // find out handler
  if (!(handler = array_find (commands, args[0])))
    {
      puttext ("NOTICE %s :Command '%s' not known\r\n", nick->nick, args[0]);
      // ENSURE THAT IT IS NOT DISABLE OR ENABLE AS THESE ARE CRITICAL COMMANDS
    }
  else if ((handler->handler == command_disable_command) ||
	   (handler->handler == command_enable_command))
    {
      puttext ("NOTICE %s :Command '%s' cannot be disabled\r\n", nick->nick,
	       handler->command);
    }
  else
    {
      // disable command
      handler->disabled = 1;
      puttext ("NOTICE %s :Command '%s' has been disabled\r\n", nick->nick,
	       handler->command);
    }
}

/**
 * Initialize command system.
 */
void command_setup (void)
{
  trigger = config_getval ("global", "trigger");
  command_register ("enable", 0, 1, 0, 1, command_enable_command);
  command_register ("disable", 0, 1, 0, 1, command_disable_command);
}

/**
 * Command comparator used by ARRAY.
 * @param key - char name of a command
 * @param elem - command structure
 */
int command_comparator (const void *key, const void *elem)
{
  const char *str;
  const struct command_s *cmd;
  str = (const char *) key;
  cmd = (const struct command_s *) elem;
  return strcasecmp (str, cmd->command);
}

/**
 * Frees the command name
 * @param elem - command structure
 * @param o - Ignored
 */
void command_eraser (void *elem, void *o)
{
  free (((struct command_s *) elem)->command);
}

/**
 * Registers new command to be listened for
 * @param command - Command name
 * @param needchan - Does it require channel
 * @param params - How many params does it want (0=infinite)
 * @param umask - Channel access perms
 * @param admin - Does it require admin privs
 * @param handler - command handler function pointer
 * @return 1 if fails, 0 if ok
 */
int
command_register (const char *command, int needchan, int params,
		  int umask, int admin, command_handler_t handler)
{
  struct command_s *ptr;
  // init cmd array
  if (commands == NULL)
    commands = array_create (sizeof (struct command_s),
			     command_comparator, command_eraser);
  // ensure not registered and can be allocated
  if (!(ptr = array_add (commands, command)))
    return 1;
  if (ptr->handler)
    return 1;
  // set up structure
  ptr->command = strdup (command);
  ptr->needchan = needchan;
  ptr->params = params;
  ptr->umask = umask;
  ptr->admin = (admin != 0 ? 1 : 0);
  ptr->handler = handler;
  ptr->disabled = 0;
  // o.k.
  return 0;
}

/**
 * Unregister command, this is very simple function.
 * @param command - Command to unregister
 * @return 0 ok, 1 failed
 */
int command_unregister (const char *command)
{
  if (commands == NULL)
    return 1;
  return array_delete (commands, command);
}

/**
 * Check if user may access command.
 * @param nick - nick to check
 * @param channel - possible channel
 * @param handler - command handler
 */
int
command_check_perms (NICK * nick, CHANNEL * channel,
		     struct command_s *handler)
{
  int flags;
  /* If command does not need perms, return */
  if ((handler->admin == 0) && (handler->umask == 0))
    return 0;
  /* If it needs perms user must be authed */
  if (nick->user == NULL)
    return 1;
  /* Admin can do anything he wants */
  if (nick->user->admin == 1)
    return 0;
  /* Admin privs required */
  if (handler->admin == 1)
    return 1;
  /* Check umask against channel */
  if (!channel)
    return 1;
  flags = user_get_channel_modes (nick->user, channel);
  if ((flags & handler->umask))
    return 0;
  /* disallow */
  return 1;
}

/**
 * Try to parse a command and call handler. 
 * @param nick - Calling nick
 * @param channel - Possible channel 
 * @param line - possible command line
 * @return 1 if no command, 0 if command
 */
int command_try_parse (NICK * nick, CHANNEL * channel, const char *line)
{
  struct command_s *handler;
  char **phase1;
  char **phase2;
  char **tptr;
  int nphase1;
  int nphase2;
  /* Use the channel-parse-line */
  phase1 = phase2 = NULL;
  if (channel)
    {
      // we know we have a channel, so we just split it in three parts
      // namely, trigger, command, args
      string_split (line, 3, &phase1, &nphase1);
      // failed.
      if (phase1 == NULL)
	return 1;
      // was it addressed to me?
      if ((strrfccmp (phase1[0], irc_get_me ()->nick)) &&
	  (strrfccmp (phase1[0], trigger)))
	{
	  free (phase1[0]);
	  free (phase1);
	  return 1;
	}
      // do we have such command?
      if (!(handler = array_find (commands, phase1[1])))
	{
	  free (phase1[0]);
	  free (phase1);
	  return 1;
	}
      // is it available to use?
      if (handler->disabled)
	{
	  puttext ("NOTICE %s :Command disabled\r\n", nick->nick);
	  free (phase1[0]);
	  free (phase1);
	  return 0;
	}
      nphase2 = 0;
      // ensure permissions
      if (command_check_perms (nick, channel, handler))
	{
	  if (nick->user)
	    puttext ("NOTICE %s :Permission denied\r\n", nick->nick);
	  print ("%s[%s] tried to use %s - denied", nick->nick,
		 (nick->user ? nick->user->user : "*"), handler->command);
	}
      else
	{
	  // phase 2. Split arguments as the handler would like to have them
	  // (sunny side up? My of course!)
	  string_split (phase1[2], handler->params, &phase2, &nphase2);
	  print ("%s[%s] used %s on %s", nick->nick,
		 (nick->user ? nick->user->user : "*"), handler->command,
		 channel->channel);
	  // call handler callback
	  (*handler->handler) (nick, channel, handler->command,
			       (const char **) phase2, nphase2);
	}
      // release memory
      if (phase2)
	{
	  free (phase2[0]);
	  free (phase2);
	}
      free (phase1[0]);
      free (phase1);
      return 0;
    }
  else
    {
      // OK. here we do not have the luxury of knowing channel.
      // this: here: splits the string into command and args.
      string_split (line, 2, &phase1, &nphase1);
      if (phase1 == NULL)
	return 1;
      // locate handler
      if (!(handler = array_find (commands, phase1[0])))
	{
	  free (phase1[0]);
	  free (phase1);
	  return 1;
	}
      // is it disabled
      if (handler->disabled)
	{
	  puttext ("NOTICE %s :Command disabled\r\n", nick->nick);
	  free (phase1[0]);
	  free (phase1);
	  return 0;
	}
      nphase2 = 0;
      // in contrast to behaviour before, we split the string before
      // permission checks. The channel is added there, IF needed.
      if (handler->needchan)
	{
	  string_split (phase1[1],
			(handler->params == 0 ? 0 : handler->params + 1),
			&phase2, &nphase2);
	  // check that there is a channel.
	  if (nphase2 < 1)
	    {
	      puttext
		("NOTICE %s :Command '%s' needs a valid channel as first argument\r\n",
		 nick->nick, handler->command);
	      free (phase1[0]);
	      free (phase1);
	      return 0;
	    }
	  // check again.
	  if ((channel = channel_find (phase2[0])) == NULL)
	    {
	      puttext
		("NOTICE %s :Command '%s' needs a valid channel as first argument - not %s\r\n",
		 nick->nick, handler->command, phase2[0]);
	      // free memory
	      if (phase2)
		{
		  free (phase2[0]);
		  free (phase2);
		}
	      free (phase1[0]);
	      free (phase1);
	      return 0;
	    }
	}
      else
	{
	  // split the params normally.
	  string_split (phase1[1], handler->params, &phase2, &nphase2);
	}
      // check for access.
      if (command_check_perms (nick, channel, handler))
	{
	  if (nick->user)
	    puttext ("NOTICE %s :Permission denied\r\n", nick->nick);
	  print ("%s[%s] tried to use %s - denied", nick->nick,
		 (nick->user ? nick->user->user : "*"), handler->command);
	  if (phase2)
	    {
	      free (phase2[0]);
	      free (phase2);
	    }
	  free (phase1[0]);
	  free (phase1);
	  return 0;
	}
      // log usage
      if (channel)
	{
	  print ("%s[%s] used %s on %s",
		 nick->nick,
		 (nick->user ? nick->user->user : "*"), handler->command,
		 channel->channel);
	}
      else
	{
	  print ("%s[%s] used %s",
		 nick->nick,
		 (nick->user ? nick->user->user : "*"), handler->command);
	}
      // execute properly.
      if (handler->needchan)
	{
	  (*handler->handler) (nick, channel, handler->command,
			       (const char **) (nphase2 -
						handler->needchan ? phase2 +
						1 : NULL),
			       nphase2 - handler->needchan);
	}
      else
	{
	  (*handler->handler) (nick, channel, handler->command,
			       (const char **) phase2, nphase2);
	}
      // free memory
      if (phase2)
	{
	  free (phase2[0]);
	  free (phase2);
	}
      if (phase1)
	{
	  free (phase1[0]);
	  free (phase1);
	}
      return 0;
    }
  return 1;
}
