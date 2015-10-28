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
/**
 * Xchange for handlers for server input
A */

#include "hbs.h"

/**
 * Contains lots of handlers, a list is provided here
 * void server_handle_ping(SERVER *server, const char **args, int argc);
 *  - pingpong
 * void server_handle_001(SERVER *server, const char **args, int argc);
 *  - welcome to IRC
 * void server_handle_nickinuse(SERVER *server, const char **args, int argc);
 *  - your nick is in use
 * void server_handle_error(SERVER *server, const char **args, int argc);
 *  - ERROR message
 * void server_handle_userip(SERVER *server, const char **args, int argc);
 *  - Your IP address is
 */

struct server_message_handler_s
{
  char *token;
  struct server_message_handler_list_s
  {
    server_message_handler_t handler;
    struct server_message_handler_list_s *next;
  } *handler;
  size_t nhandler;
};

ARRAY server_handlers = 0;

/**
 * Compares handler name and handler struct
 * @param key - handler name
 * @param elem - handler struct
 * @return comparison
 */
int server_handler_comparator (const void *key, const void *item)
{
  const char *token;
  const struct server_message_handler_s *handler;
  token = (const char *) key;
  handler = (const struct server_message_handler_s *) item;
  return strcasecmp (token, handler->token);
}

/**
 * Deletes handler name from handler
 * @param item - handler
 * @param opaque - ignored
 */
void server_handler_eraser (void *item, void *opaque)
{
  struct server_message_handler_s *handler;
  handler = (struct server_message_handler_s *) item;
  free (handler->token);
  if (handler->handler)
    {
      struct server_message_handler_list_s *ptr;
      while ((ptr = handler->handler))
	{
	  handler->handler = handler->handler->next;
	  free (ptr);
	}
    }
}

/**
 * Registers a new server message handler, or appends a new handler to old
 * handler.
 * @param token - message to listen
 * @param handlerptr - handler to call
 */
void
server_register_handler (const char *token,
			 server_message_handler_t handlerptr)
{
  struct server_message_handler_s *handler;
  struct server_message_handler_list_s *ptr;
  if (server_handlers == NULL)
    server_handlers = array_create (sizeof (struct server_message_handler_s),
				    server_handler_comparator,
				    server_handler_eraser);
  handler =
    (struct server_message_handler_s *) array_add (server_handlers, token);

  if (handler->token == NULL)
    handler->token = strdup (token);

  if (handler->handler)
    {
      ptr = malloc (sizeof (struct server_message_handler_list_s));
      ptr->next = handler->handler;
    }
  else
    {
      ptr = malloc (sizeof (struct server_message_handler_list_s));
      ptr->next = NULL;
    }
  ptr->handler = handlerptr;
  handler->handler = ptr;
}

/**
 * Removes message handler from token.
 * @param token - message to listen
 * @param handlerptr - handler to call
 */
void
server_unregister_handler (const char *token,
			   server_message_handler_t handlerptr)
{
  struct server_message_handler_s *handler;
  struct server_message_handler_list_s *ptr, *prev, *next;
  if (server_handlers == NULL)
    return;
  handler =
    (struct server_message_handler_s *) array_find (server_handlers, token);
  // removes the handler
  prev = NULL;
  ptr = handler->handler;
  while (ptr)
    {
      next = ptr->next;
      if (ptr->handler == handlerptr)
	{
	  if (prev)
	    prev->next = ptr->next;
	  if (ptr == handler->handler)
	    handler->handler = ptr->next;
	  free (ptr);
	}
      else
	{
	  prev = ptr;
	}
      ptr = next;
    }
}

/**
 * Called when event occurs
 * @param server - Current server
 * @param token - message token
 * @param args - arguments
 * @param argc - argument count
 */
void
server_fire_event (SERVER * server, const char *token, const char **args,
		   int argc)
{
  struct server_message_handler_s *handler;
  struct server_message_handler_list_s *ptr;

  if (server_handlers == NULL)
    return;
  // find handlers and call 'em all
  handler =
    (struct server_message_handler_s *) array_find (server_handlers, token);
  if (handler)
    {
      ptr = handler->handler;
      while (ptr)
	{
	  if ((*ptr->handler) (server, args, argc) == handler_drop_msg)
	    break;
	  ptr = ptr->next;
	}
    }
}

handler_retval_t
server_handle_ping (SERVER * server, const char **args, int argc)
{
  putfast ("PONG %s\r\n", args[2]);
  return handler_pass_msg;
}

handler_retval_t
server_handle_001 (SERVER * server, const char **args, int argc)
{
  const char *umodes;
  server->name = strdup (args[0]);
  server->cnick = strdup (args[2]);
  /* Tell everyone that we are successfully connected */
  server->status = server_ok;
  server_fire_event (server, "connected", NULL, 0);
  server_release_oidentd ();
  umodes = config_getval ("server_common", "umodes");
  putserv ("USERIP %s\r\n", server->cnick);
  if (umodes)
    {
      putserv ("MODE %s %s\r\n", args[2], umodes);
    }
  irc_set_me (args[2]);
  return handler_pass_msg;
}

void server_try_nick (void *opaque)
{
  putserv ("NICK :%s\r\n", server_common.nicks[0]);
}

handler_retval_t
server_handle_nickinuse (SERVER * server, const char **args, int argc)
{
  if (server->cnick != NULL)
    {
      timer_register (TIME + 30, server_try_nick, NULL);
      return handler_pass_msg;
    }
  server_common.cnick++;
  if (server_common.cnick == server_common.nnicks)
    server_common.cnick = 0;
  putserv ("NICK :%s\r\n", server_common.nicks[server_common.cnick]);
  timer_register (TIME + 30, server_try_nick, NULL);
  return handler_pass_msg;
}

handler_retval_t
server_handle_error (SERVER * server, const char **args, int argc)
{
  print ("%s: ERROR %s", server_get_name (), args[2]);
  if (!match ("*too fast*", args[2]))
    {
      print ("Throttle detected, waiting for +90 seconds");
      server_set_wait (TIME + 90);
    }
  return handler_pass_msg;
}

handler_retval_t
server_handle_userip (SERVER * server, const char **args, int argc)
{
  struct in_addr ip;
  char *cp;
  // ip is on 3rd param
  if ((cp = strchr (args[3], '@')))
    {
      cp++;
      inet_pton (AF_INET, cp, &ip);
      console_set_ip (&ip);
    }
  return handler_pass_msg;
}

/**
 * Sets handlers up
 */
void server_setup_handlers (void)
{
  server_register_handler ("340", server_handle_userip);
  server_register_handler ("ERROR", server_handle_error);
  server_register_handler ("PING", server_handle_ping);
  server_register_handler ("001", server_handle_001);
  server_register_handler ("433", server_handle_nickinuse);
}
