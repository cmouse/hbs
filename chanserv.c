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
 * Takes care of Q (L is decommissioned)
 * 
 * chanserv_message(channel, request, params);
 *
 * what are params, one might ask...
 * well, my dear, they are simply parameters for the request
 * we use va_list for this thing... 
 *
 * PLEASE READ THE FOLLOWING LIST FOR PARAM DETAILS
 *
 * requestop #channel
 * unbanall #channel
 * invite #channel
 * topic #channel newtopic
 * chanlev #channel user modes
 */

#include "hbs.h"

/* Define Q message masks */

// Authentication related
#define CHANSERV_AUTH_FAILED "Username or password incorrect."
#define CHANSERV_AUTH_SUCCESS "You are now logged in as*"

// Command related
#define CHANSERV_INSUFFICIENT_PERMS "You do not have sufficient access on *"
#define CHANSERV_BAD_CHANNEL "Channel * is unknown or suspended."

// Internal chanserv stuff
struct chanserv_status_s {
  // has AUTH line been sent
  int is_auth_sent;
  // are we authed
  int is_authed;
  // has channels been already joined
  int has_joined;
  // q user name
  char *quser;
  // q password
  char *qpass;
  // last used channel (for Q requests)
  struct last_channel_s {
    char *channel;
    struct last_channel_s *next;
  } *last_channel;
  // last used request
  chanserv_request_t last_request;
} chanserv_status = {0,0,0,0,0,0,0};

void channels_join(void);

/**
 * A simple setup.
 */
void chanserv_setup(void) {
  chanserv_status.quser = strdup(config_getval("chanserv","user"));
  chanserv_status.qpass = strdup(config_getval("chanserv","pass"));
}

/**
 * Handle connected message from server module
 * @param server - Current server
 * @param args - argument string (will be NULL)
 * @param argc - argument count (will be 0)
 */
handler_retval_t chanserv_handle_connected(SERVER *server __attribute__((unused)), const char **args __attribute__((unused)), int argc __attribute__((unused))) {
  putserv("AUTH %s %s\r\n",chanserv_status.quser,chanserv_status.qpass);
  chanserv_status.is_auth_sent = 1;
  return handler_pass_msg;
}

/**
 * Handle disconnected message from server module
 * @param server - Current server
 * @param args - argument string (will be NULL)
 * @param argc - argument count (will be 0)
 */
handler_retval_t chanserv_handle_disconnected(SERVER *server, const char **args, int argc) {
  struct last_channel_s *ptr;
  // reset status
  while((ptr = chanserv_status.last_channel)) {
    chanserv_status.last_channel = chanserv_status.last_channel->next;
    free(ptr->channel);
    free(ptr);
  }
  chanserv_status.last_channel = NULL;
  chanserv_status.last_request = 0;
  chanserv_status.is_authed = 0;
  chanserv_status.is_auth_sent = 0;
  chanserv_status.has_joined = 0;
  return handler_pass_msg;
}

/**
 * Try authing again.
 * @param opaque - Ignored
 */
void chanserv_reauth(void *opaque) {
  putserv("AUTH %s %s\r\n",chanserv_status.quser,chanserv_status.qpass);
  chanserv_status.is_auth_sent = 1;
}

/**
 * Ups. No Q there then. Fine. 
 * @param server - Current server
 * @param args - Arguments like who where when
 * @param argc - argument count
 */
handler_retval_t chanserv_handle_401(SERVER *server, const char **args, int argc) {
  /* this means Q wasn't here... launch timer... */
  if (!strrfccmp(args[3],CHANSERV_MESSAGE_TARGET_Q)) {
    if (!chanserv_status.has_joined) {
      channels_join();
      chanserv_status.has_joined = 1;
    }
    /* bad bad thing if we weren't authed. */
    if (!chanserv_status.is_authed) {
      chanserv_status.is_auth_sent = 0;
      timer_register(TIME + 60, chanserv_reauth, NULL);
    }
  }
  return handler_pass_msg;
}

/**
 * Parse various whines from Q
 * @param server - Current server
 * @param args - Argument string, from NOTICE to text
 * @param argc - Argument count, should be 4.
 */
handler_retval_t chanserv_handle_notice(SERVER *server, const char **args, int argc) {
  /* handles NOTICEs from services */
  if (strrfccmp(args[0], CHANSERV_Q_IDX))
    {
      /* not our biznis */
      return handler_pass_msg;
    }
  /* match the strings gotten from services - we are basically only interested
   * in few types - unable to auth - not available - no perms
   */
  if (!chanserv_status.has_joined) {
    channels_join();
    chanserv_status.has_joined = 1;
  }
  // No +o , doh.
  if (!match(CHANSERV_INSUFFICIENT_PERMS,args[3])) {
    CHANNEL *c;
    char *chan,*ch;
    // message contains channel
    ch = strchr(args[3],'#');
    if (ch) {
      chan = strdup(ch);
      if ((ch = strchr(chan, ' '))) *ch = '\0';
      if ((c = channel_find(chan))) {
       // kindly check for last command to find out if we really ought to do it
       if (chanserv_status.last_request == topic) {
        // oh well, no topic then...
        channel_set_service(c,CHANNEL_SERV_DIRECT_TOPIC);
       } else if (chanserv_status.last_request == unbanall) {
        chanserv_message(c, invite);
        // tough luck.
       } else {
	channel_unset_service(c,CHANNEL_SERV_Q);
	print("Q on %s has no +o for me - disabling support",c->channel);
       }
      }
      free(chan);
    }
    // unable to authenticate.
  } else if (!match(CHANSERV_AUTH_FAILED,args[3])) {
    print("Unable to authenticate network service - check setup!");
    // authed to Q.
  } else if (!match(CHANSERV_BAD_CHANNEL,args[3])) {
    CHANNEL *c;
    char *chan,*ch;
    /* Q tells channel nowadays */ 
    ch = strchr(args[3],'#');
    if (ch) {
      chan = strdup(ch);
      if ((ch = strchr(chan, ' '))) *ch = '\0';
      if ((c = channel_find(chan))) {
        channel_unset_service(c,CHANNEL_SERV_Q);
        print("Q on %s has no +o for me - disabling support",c->channel);
      }
      free(chan);
    }
  } else if (!match(CHANSERV_AUTH_SUCCESS,args[3])) {
    print("Authenticated to channel service");
    chanserv_status.is_authed = 1;
    // something was done.
  } else if (!match("*Done*",args[3])) {
    // and in case it was unbanall, go to chan
    if (chanserv_status.last_request == unbanall) {
      putserv("JOIN %s\r\n",chanserv_status.last_channel->channel);
    }
  } else {
    return handler_pass_msg;
  }
  // free stuff.
  if (chanserv_status.last_channel) {
    struct last_channel_s *ptr;
    free(chanserv_status.last_channel->channel);
    ptr = chanserv_status.last_channel;
    chanserv_status.last_channel = chanserv_status.last_channel->next;
    free(ptr);
  }
  return handler_drop_msg;
}

/**
 * Send request to channel service on channel
 * @param channel - Channel to user
 * @param request - request type
 * @param ... - optional parameters
 * @return -1 if no chanserv, 0 ok
 */
int chanserv_message(CHANNEL *channel, chanserv_request_t request, ...) {
  const char *target;
  va_list va;
  if (chanserv_status.is_authed == 0) return -1;

  target = (const char*)NULL;
  // find out who we should be speaking to
  if (channel_has_service(channel,CHANNEL_SERV_NOSERV)) return -1;
  if (request == topic && 
      channel_has_service(channel,CHANNEL_SERV_DIRECT_TOPIC)) return -1;
  if (channel_has_service(channel,CHANNEL_SERV_Q)) 
    target = CHANSERV_MESSAGE_TARGET_Q;
  if (!target) return -1;

  struct last_channel_s *ptr;

  // update last channel, just in case 
  ptr = chanserv_status.last_channel;
  while(ptr && ptr->next) ptr = ptr->next;
  if (ptr) {
    ptr->next = malloc(sizeof(struct last_channel_s));
    ptr = ptr->next;
    ptr->channel = strdup(channel->channel);
  } else {
    chanserv_status.last_channel = malloc(sizeof(struct last_channel_s));
    ptr = chanserv_status.last_channel;
    ptr->channel = strdup(channel->channel);
  }
  ptr->next = NULL;

  // act on request
  switch(request) {
  case requestop:
    putserv("PRIVMSG %s :OP %s\r\n",target,channel->channel);
    break;
  case unbanall:
    putserv("PRIVMSG %s :UNBANALL %s\r\n",target,channel->channel);
    break;
  case topic:
    va_start(va,request);
    putserv("PRIVMSG %s :SETTOPIC %s %s\r\n",target,channel->channel,va_arg(va,char*));
    va_end(va);
    break;
  case invite:
    putserv("PRIVMSG %s :INVITE %s\r\n",target,channel->channel);
    break;
  default:
    return -1;
  }
  return 0;
}

/**
 * Initialized the channel service kit.
 * @return 0 always
 */
int chanserv_init(void) {
  server_register_handler("001",chanserv_handle_connected);
  server_register_handler("DISCONNECTED",chanserv_handle_disconnected);
  server_register_handler("NOTICE",chanserv_handle_notice);
  server_register_handler("401",chanserv_handle_401);
  chanserv_setup();
  return 0;
}
