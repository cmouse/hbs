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
 * Console system - tricky, because we need both BOTNET and USER here..
 * The botnet is dealt simply by using protocol handler and 
 * user commands are parsed with the standard command parser, loaned
 * from the cmd.c
 */

#include "hbs.h"

// Loads of file globals...
struct in_addr console_ip;
int console_fd;
int conhub_fd;

// Consoles and commands
ARRAY consoles;
ARRAY concommands;
ARRAY botcommands;

// MY name in console
const char *con_myname;

// Console command structure
struct console_command_s
{
  char *cmd;
  char *help;
  int params;
  console_command_t handler;
};

// Few forward declarations
void console_parse_user (CONSOLE * console, const char *message);
void console_parse_bot (CONSOLE * console, const char *message);
void console_parse_bot_auth (CONSOLE * console, const char *message);

/**
 * Returns the console structure for our hub.
 * @return CONSOLE* 
 */
CONSOLE *conhub (void)
{
  return (CONSOLE *) array_find (consoles, &conhub_fd);
}

/**
 * Comparator that compares fd and console structure
 * @param key - file descriptor
 * @param elem - CONSOLE structure
 */
int console_comparator (const void *key, const void *elem)
{
  const int *fd;
  const CONSOLE *console;
  fd = (const int *) key;
  console = (const CONSOLE *) elem;
  return *fd - console->fd;
}

/**
 * Cleans up console structure.
 * @param elem - Console structure
 * @param opaque - Ignored.
 */
void console_eraser (void *elem, void *opaque)
{
  struct console_text_queue_s *ptr;
  CONSOLE *console;
  console = (CONSOLE *) elem;
  // disconnect user.
  if (console->disconnect == 0)
    close (console->fd);
  // free up all text strings
  if (console->start)
    {
      while (console->start)
	{
	  ptr = console->start->next;
	  free (console->start->text);
	  free (console->start);
	  console->start = ptr;
	}
    }
}

/**
 * Queue a line of text to be sent to a console connection
 * @param start - pointer to start of list pointer
 * @param stop - pointer to end of list pointer
 * @param text - text to queue
 */
void
console_queue_text (struct console_text_queue_s **start,
		    struct console_text_queue_s **stop, char *text)
{
  struct console_text_queue_s *new;
  new = malloc (sizeof (struct console_text_queue_s));
  new->text = strdup (text);
  new->next = NULL;
  if (*start == NULL)
    *start = new;
  if (*stop != NULL)
    (*stop)->next = new;
  (*stop) = new;
}

/**
 * Writes text to all.
 * @param src - ignored
 * @param fmt - format string
 * @param ... - format parameters
 */
void console_write_all (CONSOLE * src, const char *fmt, ...)
{
  CONSOLE *console;
  size_t i;
  va_list va;
  va_start (va, fmt);

  for (i = 0; i < array_count (consoles); i++)
    {
      console = (CONSOLE *) array_get_index (consoles, i);
      if (console->state == console_ok)
	{
	  if (console->handler == console_parse_user)
	    console_vwrite (console, fmt, va);
	}
    }
  va_end (va);
}

/**
 * Writes text to console.
 * @param console - target
 * @param fmt - format string
 * @param va - format parameters
 */
void console_vwrite (CONSOLE * console, const char *fmt, va_list va)
{
  char buf[CONSOLE_BUFLEN + 1];
  vsnprintf (buf, CONSOLE_BUFLEN + 1, fmt, va);
  console_queue_text (&console->start, &console->stop, buf);
}

/**
 * Writes text to console. Call vwrite.
 * @param console - target
 * @param fmt - format string
 * @param ... - format parameters
 */
void console_write (CONSOLE * console, const char *fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  console_vwrite (console, fmt, va);
  va_end (va);
}

/**
 * Logging handler for console.
 * @param tbuf - Time buffer
 * @param fmt - Formatting string
 * @param va - formatting arguments
 */
void console_log (const char *tbuf, const char *fmt, va_list va)
{
  char buf[CONSOLE_BUFLEN + 1];
  CONSOLE *console;
  size_t i;
  vsnprintf (buf, CONSOLE_BUFLEN - strlen (tbuf) - 3, fmt, va);
  // write to all USER consoles.
  for (i = 0; i < array_count (consoles); i++)
    {
      console = (CONSOLE *) array_get_index (consoles, i);
      if ((console->state == console_ok)
	  && (console->handler == console_parse_user))
	{
	  console_write (console, "[%s] %s\r\n", tbuf, buf);
	}
    }
}

/**
 * Compares command name and command structure
 * @param key - command name
 * @param elem - command structure
 */
int console_command_comparator (const void *key, const void *elem)
{
  const char *command;
  const struct console_command_s *cmd;
  cmd = (const struct console_command_s *) elem;
  command = (const char *) key;
  return strcasecmp (command, cmd->cmd);
}

/**
 * Free's up command structure, namely name and help.
 * @param elem - Command structure
 * @param opaque - Ignored
 */
void console_command_eraser (void *elem, void *opaque)
{
  struct console_command_s *cmd;
  cmd = (struct console_command_s *) elem;
  free (cmd->cmd);
  if (cmd->help)
    free (cmd->help);
}

/**
 * Registers new console command.
 * @param command - command name
 * @param params - number of parameters wanted
 * @param cb - console command callback
 * @param help - help string
 */
void
console_register_command (const char *command, int params,
			  console_command_t cb, const char *help)
{
  struct console_command_s *cmd;
  cmd = array_add (concommands, command);
  if (cmd->handler)
    return;
  cmd->cmd = strdup (command);
  cmd->params = params;
  cmd->handler = cb;
  cmd->help = strdup (help);
}

/**
 * Unregisters command.
 * @param command - Command name
 */
void console_unregister_command (const char *command)
{
  array_delete (concommands, command);
}

/**
 * Parses data sent by a user.
 * @param console - User's console
 * @param message - Message sent by user
 */
void console_parse_user (CONSOLE * console, const char *message)
{
  struct console_command_s *cmd;
  char **args;
  int argc;
  // Authentication state
  console->last = TIME;
  if (console->state == console_new)
    {
      // User is disconnected after password even if unknown user.
      // this is to make hammering harder.
      if (!(console->user = user_find (message)))
	print ("Refusing telnet access for unknown user '%s'", message);
      if (console->user && console->user->bot)
	{
	  if (botnet_has_bot (console->user->user))
	    {
	      print ("%s tried to relink.", console->user->user);
	      console_write (console, "%s already linked.\r\n",
			     console->user->user);
	      console->disconnect = 1;
	      return;
	    }
	  console_write (console, "passreq %s\r\n", con_myname);
	  console->handler = console_parse_bot_auth;
	}
      else
	{
	  console_write (console, "Password.\r\n");
	}
      console->state = console_auth;
      return;
    }
  else if (console->state == console_auth)
    {
      // check password.
      if (!console->user)
	{
	  console_write (console, "Sorry.\r\n");
	  console->disconnect = 1;
	  return;
	}
      // no password. refuse.
      if (!user_check_pass (console->user, message))
	{
	  print
	    ("Refusing telnet access for invalid/missing password for user '%s'",
	     console->user->user);
	  console_write (console, "Sorry.\r\n");
	  console->disconnect = 1;
	  return;
	}
      // enter console. and notify everyone.
      console->state = console_ok;
      print ("%s has entered console", console->user->user);
      console_relay (console, "message *** %s@%s has entered console\r\n",
		     console->user->user, con_myname);
      return;
    }
  // if message starts with dot, it's a command.
  if ((*message == '.') && (*(message + 1)))
    {
      char **phase1;
      int nphase1;
      // split the message to command and args
      string_split (message + 1, 2, &phase1, &nphase1);
      // find out command handler
      if (!(cmd = array_find (concommands, phase1[0])))
	{
	  free (phase1[0]);
	  free (phase1);
	  console_write (console, "Invalid command, try .help\r\n");
	  return;
	}
      // resplit args as the handler wants.
      string_split (phase1[1], cmd->params, &args, &argc);
      // log usage
      print ("#%s# used %s", console->user->user, cmd->cmd);
      // call command
      (*cmd->handler) (console, cmd->cmd, (const char **) args, argc);
      // free memory
      if (args)
	{
	  free (args[0]);
	  free (args);
	}
      free (phase1[0]);
      free (phase1);
    }
  else
    {
      // don't write empties
      if (strlen (message))
	{
	  // check for possible CTCP in disquise
	  if (*message == '\001')
	    {
	      if (!strncmp (message + 1, "ACTION", 6))
		{
		  // yep.
		  char *ch, *tmp;
		  tmp = strdup (message + 8);
		  if ((ch = strchr (tmp, '\001')))
		    *ch = '\0';
		  console_write_all (console, "* %s %s\r\n",
				     console->user->user, tmp);
		  console_relay (console, "message * %s@%s %s\r\n",
				 console->user->user, con_myname, tmp);
		  free (tmp);
		}
	    }
	  else
	    {
	      console_write_all (console, "<%s> %s\r\n", console->user->user,
				 message);
	      console_relay (console, "message <%s@%s> %s\r\n",
			     console->user->user, con_myname, message);
	    }
	}
    }
}

/**
 * relays any speaking to bots. except source.
 * @param src - Source
 * @param console - console struct
 * @param fmt - format string
 * @param ... - format args
 */
void
console_relay_speak (CONSOLE * src, CONSOLE * console, const char *fmt, ...)
{
  va_list va;
  char buf[CONSOLE_BUFLEN + 1];
  va_start (va, fmt);
  vsnprintf (buf, CONSOLE_BUFLEN + 1, fmt, va);
  console_write (console, "c %s@%s A %s\r\n", src->user->user, con_myname,
		 buf);
  va_end (va);
}

/**
 * relays something
 * @param src - Source
 * @param fmt - format string
 * @param ... - format args
 */
void console_relay (CONSOLE * src, const char *fmt, ...)
{
  char *nfmt;
  CONSOLE *dst;
  size_t i;
  va_list va;
  va_start (va, fmt);
  nfmt = malloc (strlen (fmt) + 3);
  strcpy (nfmt, fmt);
  if (!strchr (fmt, '\n'))
    strcat (nfmt, "\r\n");
  // only to bots, and not back to where it came from.
  for (i = 0; i < array_count (consoles); i++)
    {
      dst = (CONSOLE *) array_get_index (consoles, i);
      if ((dst->state == console_ok) && (dst->handler == console_parse_bot) &&
	  (dst != src))
	{
	  console_vwrite (dst, nfmt, va);
	}
    }
  free (nfmt);
}

/**
 * Handle bot authentication.
 * @param console - Client console struct
 * @param message - sent message
 */
void console_parse_bot_auth (CONSOLE * console, const char *message)
{
  console->last = TIME;
  if (console->state == console_auth)
    {
      // ensure that the password is our netpassword
      if (!strcmp (message, config_getval ("console", "password")))
	{
	  console_write (console, "version 2000\r\n");
	  console->handler = console_parse_bot;
	}
      else
	{
	  // reject.
	  if (console->user)
	    print ("%s tried to use wrong network password.",
		   console->user->user);
	  console_write (console, "badpass\r\n");
	  console->disconnect = 1;
	}
    }
}

/**
 * Parse any bot message
 * @param console - client console
 * @param message - message sent by client
 */
void console_parse_bot (CONSOLE * console, const char *message)
{
  struct console_command_s *cmd;
  char **args;
  int argc;
  args = NULL;
  // NOTE THAT THIS IS AUTHENTICATION WHEN _WE_ CONNECT TO SOMEPLACE
  // NOT WHEN WE ARE CONNECTED BY A BOT. THIS IS DONE BY THE FUNCTION
  // console_parse_bot_auth
  // Send my username.
  console->last = TIME;
  if (console->state == console_new)
    {
      if (!strcasecmp (message, "Username."))
	{
	  console_write (console, "%s\r\n", con_myname);
	  console->state = console_auth;
	}
      else
	{
	  console->disconnect = 1;
	}
    }
  else if (console->state == console_auth)
    {
      // Send my password if we know eachother.
      if (!strncasecmp (message, "passreq", 7))
	{
	  if ((console->user = user_find (message + 8)) == NULL)
	    {
	      console->disconnect = 1;
	      console_write (console, "badpass\r\n");
	    }
	  else
	    {
	      console_write (console, "%s\r\n",
			     config_getval ("console", "password"));
	    }
	  // Reply with version
	}
      else if (!strncasecmp (message, "version", 7))
	{
	  if (console->dir == CONSOLE_CONNECT)
	    {
	      console_write (console, "version 2000\r\n");
	    }
	  print ("Linked to %s", console->user->user);
	  botnet_add_bot (console->user->user);
	  console->state = console_ok;
	}
      else
	{
	  console->disconnect = 1;
	}
      // Parse messages normally from now on
    }
  else if (console->state == console_ok)
    {
      char **phase1;
      int nphase1;
      // split the message to command and args
      string_split (message, 2, &phase1, &nphase1);
      // find out command handler
      if (!(cmd = array_find (botcommands, phase1[0])))
	{
	  free (phase1[0]);
	  free (phase1);
	  print ("Protocol error: Unknown token %s received from %s",
		 phase1[0], console->user->user);
	  return;
	}
      // resplit args as the handler wants.
      string_split (phase1[1], cmd->params, &args, &argc);
      // call handler
      (*cmd->handler) (console, cmd->cmd, (const char **) args, argc);
      // free memory
      if (args)
	{
	  free (args[0]);
	  free (args);
	}
      free (phase1[0]);
      free (phase1);
    }
}

/**
 * Disconnect a client
 * @param console - Console to disconnect
 */
void console_disconnect (CONSOLE * console)
{
  USER *user;
  int fd;
  int bot;
  close (console->fd);
  fd = console->fd;
  // if it's bot we work different.
  bot = (console->user ? console->user->bot : 0);
  user = NULL;
  // find out username, if possible
  if (console->state == console_ok)
    user = console->user;
  // wassit console? Make console unaccessible
  if (conhub () == console)
    conhub_fd = -1;
  // drop the console out.
  array_delete (consoles, &console->fd);
  // inform.
  if (user && (conhub () != console) && (bot == 0))
    {
      print ("%s has left console", user->user);
      console_relay (console, "message *** %s@%s has left console\r\n",
		     user->user, con_myname);
    }
  else if (user && (bot == 1))
    {
      print ("Lost connection to %s", user->user);
      botnet_remove_bot (user->user);
    }
}

/**
 * Accept new connection. Preparations are made.
 */
void console_accept (void)
{
  CONSOLE *new;
  struct sockaddr_in sock;
  socklen_t slen;
  int fd;
  // try to accept the connection
  slen = sizeof sock;
  if ((fd = accept (console_fd, (struct sockaddr *) &sock, &slen)) == -1)
    {
      print ("console: accept() failed: %s", strerror (errno));
      return;
    }
  // allocate and setup new connection
  new = array_add (consoles, &fd);
  new->fd = fd;
  new->state = console_connect;
  new->handler = console_parse_user;
  new->dir = CONSOLE_ACCEPT;
  new->last = TIME;
  // ask for username
  console_write (new, "Username.\r\n");
}

/**
 * Called by console_poll when data can be written.
 * @param console - Console to write to
 */
void console_handle_write (CONSOLE * console)
{
  struct sockaddr_in sock;
  struct console_text_queue_s *ptr;

  // If the console has just been connected, test for error
  if (console->state == console_connect)
    {
      int val;
      socklen_t vlen;
      vlen = sizeof val;
      val = 0;
      // if such error exists, bail out.
      getsockopt (console->fd, SOL_SOCKET, SO_ERROR, &val, &vlen);
      if (val)
	{
	  console->disconnect = 1;
	  return;
	}
      vlen = sizeof sock;
      getpeername (console->fd, &sock, &vlen);
      // tell everyone that we have company.
      print ("Connection from %s:%d", inet_ntoa (sock.sin_addr),
	     ntohs (sock.sin_port));
      // register the connection as new
      console->state = console_new;
    }

  // write all sort of skit to the user.
  while (console->start)
    {
      ptr = console->start->next;
      if (write
	  (console->fd, console->start->text,
	   strlen (console->start->text)) < 0)
	{
	  console->disconnect = 1;
	  return;
	}
      free (console->start->text);
      free (console->start);
      console->start = ptr;
    }
  // everthing WILL be written.
  console->start = console->stop = NULL;
}

/**
 * Handle reading for console.
 * @param console - Console to read into
 */
void console_handle_read (CONSOLE * console)
{
  char *cp, *ocp, *ch;
  size_t len, left, r;
  // If the console has just been connected, test for error
  if (console->state == console_connect)
    {
      struct sockaddr_in sock;
      int val;
      socklen_t vlen;
      vlen = sizeof val;
      val = 0;
      // if such error exists, bail out.
      getsockopt (console->fd, SOL_SOCKET, SO_ERROR, &val, &vlen);
      if (val)
	{
	  console->disconnect = 1;
	  return;
	}
      vlen = sizeof sock;
      getpeername (console->fd, &sock, &vlen);
      // tell everyone that we have company.
      print ("Connection from %s:%d", inet_ntoa (sock.sin_addr),
	     ntohs (sock.sin_port));
      // register the connection as new
      console->state = console_new;
    }
  /* Find out how much we can read */
  len = strlen (console->buffer);
  left = CONSOLE_BUFLEN - len;
  /* Try read */
  r = read (console->fd, console->buffer + len, left);
  /* Bummer - it failed */
  if (r < 1)
    {
      console->disconnect = 1;
      return;
    }
  /* Process line at a time */
  *(console->buffer + len + r) = '\0';
  ocp = console->buffer;
  /* Look line end */
  while ((cp = strchr (ocp, '\n')))
    {
      /* Parse and go for next */
      *cp = '\0';
      if ((ch = strchr (ocp, '\r')))
	*ch = 0;
      /* Choose correct handler */
      (*console->handler) (console, ocp);
      ocp = cp + 1;
    }
  if (*ocp)
    {
      /* Out of space, forced parse and cleanup */
      if (strlen (ocp) == CONSOLE_BUFLEN)
	{
	  (*console->handler) (console, ocp);
	  *console->buffer = '\0';
	}
      else
	{
	  /* Clean up leftovers after move */
	  len = strlen (ocp);
	  memmove (console->buffer, ocp, len);
	  /* keep things tidy! */
	  *(console->buffer + len) = '\0';
	}
    }
  else
    {
      /* Nothing left, ensure buffer clean */
      *console->buffer = '\0';
    }
}

/**
 * Look up protocol number for TCP protocol. Maybe this should be done 
 * by configure script...
 * @return protocol number
 */
int console_proto (void)
{
  return tcp_protocol_number;
}

/**
 * Creates a new socket to use
 * @return 0 on success, 1 on fail
 */
int console_make_socket (void)
{
  struct addrinfo *info;
  struct addrinfo hints;
  int ec, fd, val;
  time_t t;
  // Setup hints for getaddrinfo
  memset (&hints, 0, sizeof hints);
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = console_proto ();
  // Get a console socket to be used. 
  if ((ec =
       getaddrinfo (config_getval ("console", "host"),
		    config_getval ("console", "port"), &hints, &info)))
    {
      print ("console: getaddrinfo(%s,%s,NULL,&info) failed: %s",
	     config_getval ("console", "host"), config_getval ("console",
							       "port"),
	     gai_strerror (ec));
      return 1;
    }
  // Create socket
  if ((fd =
       socket (info->ai_family, info->ai_socktype, info->ai_protocol)) == -1)
    {
      print ("console: socket(%d,%d,%d) failed: %s", info->ai_family,
	     info->ai_socktype, info->ai_protocol, strerror (errno));
      return 1;
    }
  // Set reuseaddr on
  val = 1;
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
  t = TIME + 30;
  // try to bind for ~30 seconds
  print ("Trying to bind, please wait");
  while ((ec = bind (fd, info->ai_addr, info->ai_addrlen)) && (TIME - t < 30))
    usleep (100);
  // still fails?
  if (ec && bind (fd, info->ai_addr, info->ai_addrlen))
    {
      close (fd);
      print ("console: bind(%d,socket,%d) failed: %s", fd, info->ai_addrlen,
	     strerror (errno));
      freeaddrinfo (info);
      return 1;
    }
  // release info
  freeaddrinfo (info);
  // listen, with 10 connection queue.
  if (listen (fd, 10))
    {
      close (fd);
      print ("console: listen(%d,10) failed: %s", fd, strerror (errno));
      return 1;
    }
  console_fd = fd;
  // oki
  return 0;
}

/**
 * Check and disconnect idle consoles
 */

void console_check_consoles (void)
{
  CONSOLE *console;
  size_t i;
  for (i = 0; i < array_count (consoles); i++)
    {
      console = (CONSOLE *) array_get_index (consoles, i);
      if (console->state < console_ok)
	{
	  // a lot smaller timeout
	  if (TIME - console->last > CONSOLE_MAX_AUTH_WAIT)
	    {
	      console->disconnect = 1;
	    }
	}
      else
	{
	  if (console->handler == console_parse_user)
	    {
	      if (TIME - console->last > CONSOLE_MAX_USER_IDLE)
		{
		  console->disconnect = 1;
		}
	    }
	  else
	    {
	      if (TIME - console->last > CONSOLE_MAX_BOT_IDLE)
		{
		  console->disconnect = 1;
		}
	      else if (TIME - console->last > CONSOLE_MAX_BOT_IDLE / 2)
		{
		  if (console->pinged == 0)
		    {
		      console_write (console, "ping\r\n");
		      console->pinged = 1;
		    }
		}
	    }
	}
    }
}

/**
 * Poll all our file descriptors for changes and call appropriate funcs.
 */
void console_poll (void)
{
  CONSOLE *console;
  struct pollfd *polls;
  unsigned int i;
  console_check_consoles ();
  // we always have at least 1 fd, the listening one.
  polls = malloc (sizeof (struct pollfd) * (array_count (consoles) + 1));
  polls[0].fd = console_fd;
  polls[0].events = POLLIN | POLLOUT;
  polls[0].revents = 0;
  // setup poll structs
  for (i = 0; i < array_count (consoles); i++)
    {
      polls[i + 1].fd = ((CONSOLE *) array_get_index (consoles, i))->fd;
      polls[i + 1].events = POLLIN | POLLOUT;
      polls[i + 1].revents = 0;
    }
  // poll everything
  if (poll (polls, array_count (consoles) + 1, 100))
    {
      // check for events
      for (i = 1; i < array_count (consoles) + 1; i++)
	{
	  console = array_find (consoles, &(polls[i].fd));
	  if (console)
	    {
	      if (polls[i].revents & POLLIN)
		console_handle_read (console);
	      if (polls[i].revents & POLLOUT)
		console_handle_write (console);
	      if (polls[i].revents & (POLLNVAL | POLLHUP | POLLERR))
		console->disconnect = 1;
	      // disconnection requested?
	      if (console->disconnect)
		console_disconnect (console);
	    }
	}
      // check for new connections
      if (polls[0].revents & POLLIN)
	{
	  console_accept ();
	}
    }
  // free memory
  free (polls);
}

void console_command_print_bots (void *elem, void *opaq)
{
  CONSOLE *console;
  struct botnet_bot_s *bot;
  console = (CONSOLE *) opaq;
  bot = (struct botnet_bot_s *) elem;
  console_write (console, "%s(%d) ", bot->name, bot->channels);
}

void
console_command_bots (CONSOLE * console, const char *cmd, const char **args,
		      int argc)
{
  console_write (console, "Currently linked bots:");
  botnet_walker (console_command_print_bots, console);
  console_write (console, "\r\n");
}

void
console_command_quit (CONSOLE * console, const char *cmd, const char **args,
		      int argc)
{
  console_write (console, "Goodbye.\r\n");
  console->disconnect = 1;
}

void
console_command_help (CONSOLE * console, const char *cmd, const char **args,
		      int argc)
{
  struct console_command_s *cmdptr;
  size_t i;
  console_write (console, "Command list\r\n");
  for (i = 0; i < array_count (concommands); i++)
    {
      cmdptr = (struct console_command_s *) array_get_index (concommands, i);
      if (cmdptr->help)
	console_write (console, "  %-20s - %s\r\n", cmdptr->cmd,
		       cmdptr->help);
      else
	console_write (console, "  %-20s\r\n", cmdptr->cmd);
    }
  console_write (console, "End of list\r\n");
}

void
console_command_load (CONSOLE * console, const char *cmd, const char **args,
		      int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s module\r\n", cmd);
      return;
    }
  module_load (args[0]);
}

void
console_command_unload (CONSOLE * console, const char *cmd, const char **args,
			int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s module\r\n", cmd);
      return;
    }
  module_unload (args[0]);
}

void
console_command_lsmod (CONSOLE * console, const char *cmd, const char **args,
		       int argc)
{
  const struct module_info_s **list, **ptr;
  list = module_list ();
  for (ptr = list; *ptr; ptr++)
    console_write (console, "%-15s v%d.%d\r\n", (*ptr)->modname,
		   (*ptr)->major, (*ptr)->minor);
  free (list);
}

void
console_command_chat (NICK * nick, CHANNEL * channel, const char *cmd,
		      const char **args, int argc)
{
  puttext ("PRIVMSG %s :\001DCC CHAT CHAT %u %s\001\r\n", nick->nick,
	   ntohl (console_ip.s_addr), config_getval ("console", "port"));
}

/**
 * Attempt to connect to hub.
 * @param opaque - not used.
 */
void console_connect_hub (void *opaque)
{
  CONSOLE *new;
  int fd, val;
  struct addrinfo *info, hints;
  // we are already connected.
  if (conhub () != NULL)
    {
      timer_register (TIME + 60, console_connect_hub, NULL);
      return;
    }
  // setup console connection.
  memset (&hints, 0, sizeof hints);
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = console_proto ();
  if (getaddrinfo (config_getval ("console", "hubhost"),
		   config_getval ("console", "hubport"), &hints, &info))
    {
      print ("console: Unable to get host for hub bot - will not link.");
      return;
    }
  // create socket.
  if ((fd =
       socket (info->ai_family, info->ai_socktype, info->ai_protocol)) == -1)
    {
      print ("console: socket(%d,%d,%d) failed: %s", info->ai_family,
	     info->ai_socktype, info->ai_protocol, strerror (errno));
      freeaddrinfo (info);
      return;
    }
  // make is reuseaddr AND make it async for a while.
  val = 1;
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
  val = fcntl (fd, F_GETFL);
  fcntl (fd, F_SETFL, val | O_ASYNC);
  if (connect (fd, info->ai_addr, info->ai_addrlen))
    {
      if (errno != EINPROGRESS)
	{
	  timer_register (TIME + 60, console_connect_hub, NULL);
	  freeaddrinfo (info);
	  return;
	}
    }
  // make it back sync
  fcntl (fd, F_SETFL, val);
  freeaddrinfo (info);
  // allocate and setup 
  new = array_add (consoles, &fd);
  new->fd = fd;
  new->state = console_connect;
  new->dir = CONSOLE_CONNECT;
  new->handler = console_parse_bot;
  new->last = TIME;
  conhub_fd = fd;
  // start checking..
  timer_register (TIME + 60, console_connect_hub, NULL);
}

void
console_bot_pong (CONSOLE * console, const char *cmd, const char **args,
		  int argc)
{
  console->pinged = 0;
}

void
console_bot_ping (CONSOLE * console, const char *cmd, const char **args,
		  int argc)
{
  console_write (console, "pong\r\n");
}

void
console_bot_message (CONSOLE * console, const char *cmd, const char **args,
		     int argc)
{
  console_write_all (console, "%s\r\n", args[0]);
  console_relay (console, "%s %s\r\n", cmd, args[0]);
}

void
console_bot_mycount (CONSOLE * console, const char *cmd, const char **args,
		     int argc)
{
  if (!strcasecmp (args[0], con_myname))
    {
      int count;
      count = s_atoi (args[2]);
      botnet_set_channels (args[1], count);
    }
  else
    {
      console_relay (console, "mycount %s %s %s\r\n", args[0], args[1],
		     args[2]);
    }
}

void
console_bot_count (CONSOLE * console, const char *cmd, const char **args,
		   int argc)
{
  console_write (console, "mycount %s %s " FSO "\r\n", args[0], con_myname,
		 array_count (channel_get_array ()));
  console_relay (console, "count %s\r\n", args[0]);
}

void
console_bot_addchan (CONSOLE * console, const char *cmd, const char **args,
		     int argc)
{
  if (argc < 4)
    {
      return;
    }
  if (!strrfccmp (args[0], con_myname))
    {
      USER *user;
      CHANNEL *channel;
      channel = channel_create (args[2]);
      channel_set_service (channel, CHANNEL_SERV_ENFORCEBANS);
      channel->created = TIME;
      channel->lastop = TIME;
      channel->laston = TIME;
      channel->lastjoin = TIME;
      if (argc > 4)
	channel->key = strdup (args[4]);
      user = user_create (args[3]);
      user_set_channel_modes (user, channel,
			      USER_OWNER | USER_MASTER | USER_OP);
      console_write (console, "genok %s %s Channel %s added\r\n", args[1],
		     con_myname, args[2]);
    }
  else
    {
      if (argc == 5)
	{
	  console_relay (console, "addchan %s %s %s %s %s\r\n", args[0],
			 args[1], args[2], args[3], args[4]);
	}
      else
	{
	  console_relay (console, "addchan %s %s %s %s\r\n", args[0], args[1],
			 args[2], args[3]);
	}
    }
}

void
console_bot_genok (CONSOLE * console, const char *cmd,
		   const char **args, int argc)
{
  if (!strrfccmp (args[0], con_myname))
    {
      print ("Receipt from %s: %s", args[1], args[2]);
    }
  else
    {
      console_relay (console, "genok %s %s %s\r\n", args[0], args[1],
		     args[2]);
    }
}

void
console_command_restart (CONSOLE * console, const char *cmd,
			 const char **args, int argc)
{
  char *const *argv;
  switch (fork ())
    {
    case -1:
      console_write (console, "genok %s %s %s\r\n",
		     args[0], console_get_myname (), "Unable to restart");
      break;
    case 0:
      argv = main_get_argv ();
      sleep (2);
      execve (argv[0], argv, environ);
      /* fall-through */
    default:
      raise (SIGQUIT);
    }
}

void
console_command_chpass (CONSOLE * console, const char *cmd,
			const char **args, int argc)
{
  USER *victim;
  char *newpw;
  char hexpw[SHA_DIGEST_LENGTH * 2 + 1];
  if (argc < 3)
    {
      console_write (console, "Usage: %s yourpass username newpass\r\n", cmd);
      return;
    }
  if (!user_check_pass (console->user, args[0]))
    {
      console_write (console, "Wrong password\r\n");
      return;
    }
  if (!(victim = user_find (args[1])))
    {
      console_write (console, "No such user %s\r\n", args[1]);
      return;
    }
  newpw = sha1sum (args[2]);
  memcpy (victim->pass, newpw, SHA_DIGEST_LENGTH);
  bin2hex (hexpw, newpw, SHA_DIGEST_LENGTH);
  console_write (console, "Password updated\r\n");
  console_relay (console, "%s %s %s %s", "password", console_get_myname (),
		 victim->user, hexpw);
  free (newpw);
  memset (hexpw, 0, sizeof hexpw);
}

void
console_command_password (CONSOLE * console, const char *cmd,
			  const char **args, int argc)
{
  char *newpw;
  char hexpw[SHA_DIGEST_LENGTH * 2 + 1];
  if (argc < 3)
    {
      console_write (console, "Usage: %s oldpass newpass newpass\r\n", cmd);
      return;
    }
  if (!user_check_pass (console->user, args[0]))
    {
      console_write (console, "Old password did not match\r\n");
      return;
    }
  if (strcmp (args[1], args[2]))
    {
      console_write (console, "Password mismatch\r\n");
      return;
    }
  newpw = sha1sum (args[1]);
  memcpy (console->user->pass, newpw, SHA_DIGEST_LENGTH);
  console_write (console, "Password updated\r\n");
  bin2hex (hexpw, newpw, SHA_DIGEST_LENGTH);
  console_relay (console, "%s %s %s %s", "password", console_get_myname (),
		 console->user->user, hexpw);
  free (newpw);
  memset (hexpw, 0, sizeof hexpw);
}

void
console_command_bnrestart (CONSOLE * console, const char *cmd,
			   const char **args, int argc)
{
  if (argc < 1)
    {
      console_write (console, "Usage: %s targetmask\r\n", cmd);
      return;
    }
  console_relay (console, "%s %s %s %s", "restart", console_get_myname (),
		 args[0], args[1]);
}

void
console_command_bnloadunload (CONSOLE * console, const char *cmd,
			      const char **args, int argc)
{
  if (argc < 2)
    {
      console_write (console, "Usage: %s targetmask module\r\n", cmd);
      return;
    }
  if (!strcasecmp (cmd, "bnload"))
    {
      console_relay (console, "%s %s %s %s", "load", console_get_myname (),
		     args[0], args[1]);
    }
  else if (!strcasecmp (cmd, "bnunload"))
    {
      console_relay (console, "%s %s %s %s", "unload", console_get_myname (),
		     args[0], args[1]);
    }
}

void
console_bot_bnrestart (CONSOLE * console, const char *cmd,
		       const char **args, int argc)
{
  char *const *argv;
  if (argc < 2)
    return;
  console_relay (console, "%s %s %s\r\n", cmd, args[0], args[1]);
  if (!match (args[1], console_get_myname ()))
    {
      switch (fork ())
	{
	case -1:
	  console_write (console, "genok %s %s %s\r\n",
			 args[0], console_get_myname (), "Unable to restart");
	  break;
	case 0:
	  argv = main_get_argv ();
	  sleep (2);
	  execve (argv[0], argv, environ);
	  /* fall-through */
	default:
	  raise (SIGQUIT);
	}
    }
}

void
console_bot_password (CONSOLE * console, const char *cmd,
		      const char **args, int argc)
{
  USER *user;
  char password[SHA_DIGEST_LENGTH];
  if (argc < 2)
    return;
  console_relay (console, "%s %s %s %s\r\n", cmd, args[0], args[1], args[2]);
  if (!(user = user_find (args[1])))
    {
      console_write (console, "genok %s %s %s %s\r\n",
		     args[0], console_get_myname (), "No such user", args[1]);
    }
  else
    {
      hex2bin (password, args[2]);
      memcpy (user->pass, password, SHA_DIGEST_LENGTH);
      console_write (console, "genok %s %s %s %s\r\n",
		     args[0],
		     console_get_myname (),
		     "Password changed for", user->user);
      memset (password, 0, sizeof password);
    }
}

void
console_bot_bnloadunload (CONSOLE * console, const char *cmd,
			  const char **args, int argc)
{
  if (argc < 3)
    return;
  if (!match (args[1], console_get_myname ()))
    {
      if (!strcmp (cmd, "unload"))
	{
	  if (module_unload (args[2]))
	    {
	      console_write (console, "genok %s %s %s %s\r\n",
			     args[0],
			     console_get_myname (),
			     "Unable to unload module", args[2]);
	    }
	  else
	    {
	      console_write (console, "genok %s %s Module %s unloaded\r\n",
			     args[0], console_get_myname (), args[2]);
	    }
	}
      else if (!strcmp (cmd, "load"))
	{
	  if (module_load (args[2]))
	    {
	      console_write (console, "genok %s %s %s %s\r\n",
			     args[0],
			     console_get_myname (),
			     "Unable to load module", args[2]);
	    }
	  else
	    {
	      console_write (console, "genok %s %s Module %s loaded\r\n",
			     args[0], console_get_myname (), args[2]);
	    }
	}
    }
  console_relay (console, "%s %s %s %s\r\n", cmd, args[0], args[1], args[2]);
}


/**
 * Setup console system
 * @return 0 ok, 1 fail
 */
int console_setup (void)
{
  // make arrays
  if (!
      (consoles =
       array_create (sizeof (CONSOLE), console_comparator, console_eraser)))
    {
      print ("console: Unable to create consoles array");
      return 1;
    }
  concommands = array_create (sizeof (struct console_command_s),
			      console_command_comparator,
			      console_command_eraser);
  botcommands = array_create (sizeof (struct console_command_s),
			      console_command_comparator,
			      console_command_eraser);
  // try to bind to socket
  if (console_make_socket ())
    return 1;
  // register our various commands
  console_register_command ("bots", 0, console_command_bots,
			    "Lists linked bots");
  console_register_command ("quit", 0, console_command_quit,
			    "Quits from console");
  console_register_command ("help", 0, console_command_help,
			    "Displays command list");
  command_register ("chat", 0, 0, 0, 1, console_command_chat);
  console_register_command ("load", 0, console_command_load,
			    "Loads module into bot");
  console_register_command ("unload", 0, console_command_unload,
			    "Unloads module from bot");
  console_register_command ("lsmod", 0, console_command_lsmod,
			    "Lists modules");
  console_register_command ("restart", 0, console_command_restart,
			    "Restarts bot");
  console_register_command ("bnrestart", 0, console_command_bnrestart,
			    "Restarts other bots");
  console_register_command ("bnload", 0, console_command_bnloadunload,
			    "Botnet load module");
  console_register_command ("bnunload", 0, console_command_bnloadunload,
			    "Botnet unload module");
  console_register_command ("password", 0, console_command_password,
			    "Change your password");
  console_register_command ("chpass", 0, console_command_chpass,
			    "Change someone elses password");

  console_register_botcommand ("password", 0, console_bot_password);
  console_register_botcommand ("ping", 0, console_bot_ping);
  console_register_botcommand ("pong", 0, console_bot_pong);
  console_register_botcommand ("message", 1, console_bot_message);
  console_register_botcommand ("count", 0, console_bot_count);
  console_register_botcommand ("mycount", 0, console_bot_mycount);
  console_register_botcommand ("addchan", 0, console_bot_addchan);
  console_register_botcommand ("restart", 0, console_bot_bnrestart);
  console_register_botcommand ("load", 0, console_bot_bnloadunload);
  console_register_botcommand ("unload", 0, console_bot_bnloadunload);
  console_register_botcommand ("genok", 3, console_bot_genok);

  con_myname = config_getval ("console", "myname");
  log_register (console_log);
  // if we have a hub, try to connect to it.
  if (config_getval ("console", "hubhost"))
    timer_register (TIME + 5, console_connect_hub, NULL);
  return 0;
}

/**
 * Clean up all connections
 */
void console_cleanup (void)
{
  close (console_fd);
  array_destroy (consoles, NULL);
  consoles = NULL;
}

/**
 * Set my IP address. 
 * @param ip - ip address to use
 */
void console_set_ip (struct in_addr *ip)
{
  memcpy (&console_ip, ip, sizeof console_ip);
}

/**
 * Registers new console bot command.
 * @param command - command name
 * @param params - number of parameters wanted
 * @param cb - console command callback
 */
void
console_register_botcommand (const char *command, int params,
			     console_command_t cb)
{
  struct console_command_s *cmd;
  cmd = array_add (botcommands, command);
  if (cmd->handler)
    return;
  cmd->cmd = strdup (command);
  cmd->params = params;
  cmd->handler = cb;
  cmd->help = NULL;
}

/**
 * Unregisters bot command.
 * @param command - Command name
 */
void console_unregister_botcommand (const char *command)
{
  array_delete (botcommands, command);
}

/**
 * Exposes my name
 * @return my name
 */
const char *console_get_myname (void)
{
  return con_myname;
}
