#include "hbs.h"

const struct module_info_s conwrapper_info_s;

const struct module_info_s VISIBLE *conwrapper_info (void)
{
  return &conwrapper_info_s;
}

void conwrapper_command_status_walker (void *chan, void *opaque)
{
  char *status;
  CHANNEL *channel;
  CONSOLE *console;
  console = (CONSOLE *) opaque;
  channel = *(CHANNEL **) chan;
  if (channel->onchan)
    {
      if (channel->hasop)
	{
	  status = "lurking";
	}
      else
	{
	  status = "want ops";
	}
    }
  else
    {
      status = "trying";
    }
  console_write (console, "\t%s (%s)\r\n", channel->channel, status);
}

void conwrapper_command_who (CONSOLE * console, const char *cmd,
			     const char **args, int argc)
{

}

void conwrapper_command_status (CONSOLE * console, const char *cmd,
				const char **args, int argc)
{
  SERVER *server;
  server = server_get_current ();
  console_write (console, "HBS bot status\r\n");
  console_write (console, "--------------\r\n");
  if (server_isconnected ())
    {
      console_write (console, "Server: %s:%s (rx: %llu tx: %llu)\r\n",
		     server->host, server->port, server->rx, server->tx);
    }
  else
    {
      console_write (console, "Server: disconnected\r\n");
    }
  console_write (console, "Channels\r\n");
  array_walk (channel_get_array (), conwrapper_command_status_walker,
	      console);
}

void conwrapper_command_weed (CONSOLE * console, const char *cmd,
			      const char **args, int argc)
{
  irc_weed ();
}

void conwrapper_command_save (CONSOLE * console, const char *cmd,
			      const char **args, int argc)
{
  irc_store ();
  console_write (console, "User and channel files written\r\n");
}

void conwrapper_command_raw (CONSOLE * console, const char *cmd,
			     const char **args, int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s string\r\n", cmd);
      return;
    }
  puttext ("%s\r\n", args[0]);
}

void conwrapper_command_addchan (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  CHANNEL *c;
  USER *u;
  if (argc < 2)
    {
      console_write (console, "Usage: %s #channel owner\r\n", cmd);
      return;
    }
  if ((*args[0] != '#') && (*args[0] != '&') && (*args[0] != '+'))
    {
      console_write (console, "Invalid channel name\r\n");
      return;
    }
  if ((c = channel_find (args[0])))
    {
      console_write (console, "Channel already added\r\n");
      return;
    }
  if (!(u = user_find (args[1])))
    {
      console_write (console, "User does not exist\r\n");
      return;
    }
  c = channel_create (args[0]);
  c->created = TIME;
  c->lastop = TIME;
  c->laston = TIME;
  c->lastjoin = TIME;

  user_set_channel_modes (u, c, USER_OWNER | USER_MASTER | USER_OP);
  putserv ("JOIN %s\r\n", c->channel);
  console_write (console, "Channel %s added\r\n", c->channel);
}

void conwrapper_command_delchan (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  CHANNEL *c;
  if (argc < 1)
    {
      console_write (console, "Usage: %s #channel", cmd);
      return;
    }
  if (!(c = channel_find (args[0])))
    {
      console_write (console, "No such channel exists\r\n");
      return;
    }
  channel_delete (args[0]);
  console_write (console, "Channel %s deleted\r\n", args[0]);

}

void conwrapper_command_adduser (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  USER *u;
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  if ((u = user_find (args[0])))
    {
      console_write (console, "User already added\r\n");
      return;
    }
  u = user_create (args[0]);
  console_write (console, "User %s added\r\n", u->user);
}

void conwrapper_command_deluser (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  USER *u;
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  if (!(u = user_find (args[0])))
    {
      console_write (console, "No such user exists\r\n");
      return;
    }
  user_delete (args[0]);
  console_write (console, "User %s deleted\r\n", args[0]);
}

void conwrapper_command_admin (CONSOLE * console, const char *cmd,
			       const char **args, int argc)
{
  USER *u;
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  if (!(u = user_find (args[0])))
    {
      console_write (console, "No such user exists\r\n");
      return;
    }
  u->admin = 1;
  console_write (console, "%s has been made admin\r\n", u->user);
}

void conwrapper_command_unadmin (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  USER *u;
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  if (!(u = user_find (args[0])))
    {
      console_write (console, "No such user exists\r\n");
      return;
    }
  u->admin = 0;
  console_write (console, "Admin rights revoked from %s\r\n", u->user);
}

void conwrapper_command_addbot (CONSOLE * console, const char *cmd,
				const char **args, int argc)
{
  USER *u;
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  if ((u = user_find (args[0])))
    {
      console_write (console, "User already added\r\n");
      return;
    }
  u = user_create (args[0]);
  console_write (console, "Bot %s added\r\n", u->user);
  u->bot = 1;
}

void conwrapper_command_bncount (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  console_relay (console, "count %s\r\n", console_get_myname ());
  console_write (console, "%s has " FSO " channels\r\n",
		 console_get_myname (), array_count (channel_get_array ()));
}

void conwrapper_command_bnadduser (CONSOLE * console, const char *cmd,
				   const char **args, int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  console_relay (console, "adduser %s %s\r\n", console_get_myname (),
		 args[0]);
}

void conwrapper_command_bndeluser (CONSOLE * console, const char *cmd,
				   const char **args, int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  console_relay (console, "deluser %s %s\r\n", console_get_myname (),
		 args[0]);
}

void conwrapper_command_bnadmin (CONSOLE * console, const char *cmd,
				 const char **args, int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  console_relay (console, "admin %s %s\r\n", console_get_myname (), args[0]);
}

void conwrapper_command_bnunadmin (CONSOLE * console, const char *cmd,
				   const char **args, int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s username\r\n", cmd);
      return;
    }
  console_relay (console, "unadmin %s %s\r\n", console_get_myname (),
		 args[0]);
}

void conwrapper_bot_adduser (CONSOLE * console, const char *cmd,
			     const char **args, int argc)
{
  if (argc < 2)
    return;
  USER *u;
  if ((u = user_find (args[1])))
    {
      console_write (console, "genok %s %s %s\r\n",
		     args[0], console_get_myname (), "User already exists");
    }
  else
    {
      u = user_create (args[1]);
      console_write (console, "genok %s %s User %s added\r\n",
		     args[0], console_get_myname (), u->user);
    }
  console_relay (console, "%s %s %s\r\n", cmd, args[0], args[1]);
}

void conwrapper_bot_deluser (CONSOLE * console, const char *cmd,
			     const char **args, int argc)
{
  USER *u;
  if (!(u = user_find (args[1])))
    {
      console_write (console, "genok %s %s %s\r\n",
		     args[0], console_get_myname (), "No such user");
    }
  else
    {
      user_delete (args[1]);
      console_write (console, "genok %s %s User %s deleted\r\n",
		     args[0], console_get_myname (), args[1]);
    }
  console_relay (console, "%s %s %s\r\n", cmd, args[0], args[1]);
}

void conwrapper_bot_admin (CONSOLE * console, const char *cmd,
			   const char **args, int argc)
{
  USER *u;
  if (!(u = user_find (args[1])))
    {
      console_write (console, "genok %s %s %s\r\n",
		     args[0], console_get_myname (), "No such user");
    }
  else
    {
      u->admin = 1;
      console_write (console, "genok %s %s User %s made admin\r\n",
		     args[0], console_get_myname (), args[1]);
    }
  console_relay (console, "%s %s %s\r\n", cmd, args[0], args[1]);
}

void conwrapper_bot_unadmin (CONSOLE * console, const char *cmd,
			     const char **args, int argc)
{
  USER *u;
  if (!(u = user_find (args[1])))
    {
      console_write (console, "genok %s %s %s\r\n",
		     args[0], console_get_myname (), "No such user\r\n");
    }
  else
    {
      u->admin = 1;
      console_write (console, "genok %s %s Removed user %s admin rights\r\n",
		     args[0], console_get_myname (), args[1]);
    }
  console_relay (console, "%s %s %s\r\n", cmd, args[0], args[1]);
}

int conwrapper_load (void)
{
  console_register_command ("raw", 1, conwrapper_command_raw,
			    "Sends text to server");
  console_register_command ("save", 0, conwrapper_command_save,
			    "Stores user and channel files");
  console_register_command ("weed", 0, conwrapper_command_weed,
			    "Cleans the user and channel files");
  console_register_command ("+chan", 0, conwrapper_command_addchan,
			    "Adds new channel");
  console_register_command ("-chan", 0, conwrapper_command_delchan,
			    "Deletes a channel");
  console_register_command ("+user", 0, conwrapper_command_adduser,
			    "Adds a new user");
  console_register_command ("-user", 0, conwrapper_command_deluser,
			    "Deletes a user");
  console_register_command ("admin", 0, conwrapper_command_admin,
			    "Gives admin rights to a user");
  console_register_command ("unadmin", 0, conwrapper_command_unadmin,
			    "Revokes admin rights from a user");
  console_register_command ("+bot", 0, conwrapper_command_addbot,
			    "Adds a bot");
  console_register_command ("bncount", 0, conwrapper_command_bncount,
			    "Ask botnet for channel count");
  console_register_command ("+bnuser", 0, conwrapper_command_bnadduser,
			    "Adds a network wide user");
  console_register_command ("-bnuser", 0, conwrapper_command_bndeluser,
			    "Deletes a network wide user");
  console_register_command ("bnadmin", 0, conwrapper_command_bnadmin,
			    "Makes a network wide admin");
  console_register_command ("bnunadmin", 0, conwrapper_command_bnunadmin,
			    "Removes a network wide admin");
  console_register_command ("status", 0, conwrapper_command_status,
			    "Show status information");
  console_register_botcommand ("adduser", 0, conwrapper_bot_adduser);
  console_register_botcommand ("deluser", 0, conwrapper_bot_deluser);
  console_register_botcommand ("admin", 0, conwrapper_bot_admin);
  console_register_botcommand ("unadmin", 0, conwrapper_bot_unadmin);
  return 0;
}

int conwrapper_unload (void)
{
  console_unregister_command ("raw");
  console_unregister_command ("save");
  console_unregister_command ("weed");
  console_unregister_command ("+chan");
  console_unregister_command ("-chan");
  console_unregister_command ("+user");
  console_unregister_command ("-user");
  console_unregister_command ("admin");
  console_unregister_command ("unadmin");
  console_unregister_command ("+bot");
  console_unregister_command ("bncount");
  console_unregister_command ("+bnuser");
  console_unregister_command ("-bnuser");
  console_unregister_command ("bnadmin");
  console_unregister_command ("bnunadmin");
  console_unregister_command ("status");
  console_unregister_botcommand ("adduser");
  console_unregister_botcommand ("deluser");
  console_unregister_botcommand ("admin");
  console_unregister_botcommand ("unadmin");
  return 0;
}

const struct module_info_s conwrapper_info_s = {
  "conwrapper",
  1,
  0,
  conwrapper_load,
  conwrapper_unload
};
