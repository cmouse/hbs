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
 * The SwitchBoard for nick <-> channel <-> user
 */

#include "hbs.h"

// forward decls

void console_command_chat(NICK*,CHANNEL*,const char*,const char**,int);
void irc_channel_unprotect(void *param);

// My nick
NICK *me = 0;

// Hide messages?
int irc_hide_messages = 0;

/**
 * Returns my nick
 * @return nick structure 
 */
NICK *irc_get_me(void) {
  return me;
}

/**
 * Set my nick
 * @param nick - My (new) nick
 */
void irc_set_me(const char *nick) {
  if (me) 
    nick_rename(me,nick);
  else
    me = nick_create(nick);
}

/**
 * Possibly fix topic.
 */ 
void irc_fix_topic(CHANNEL *c, const char *source) {
  char **idx;
  NICK *n;
  /* Only if such thing is wanted */
  if (c->topic == NULL) {
    // we have a topic? No. set it.
    if (c->ctopic) {
      c->topic = strdup(c->ctopic);
    }
    return;
  }
  // topic forcing off?
  if (!(channel_has_service(c,CHANNEL_SERV_TOPIC)))
    {
      if (c->ctopic) {
	if (c->topic) free(c->topic);
	c->topic = strdup(c->ctopic);
      }
    }
  // check who set it.
  idx = split_idx2array(source);
  if (idx) {
    n = nick_find(idx[0]);
    free(idx[0]);
    free(idx);
  } else {
    n = NULL;
  }
  if (n) {
    /* Do not fight with certain people, like myself */
    if ((n == me) || n->ircop || (user_get_channel_modes(n->user,c) & USER_MASTER)) {
      if (c->ctopic) {
	if (c->topic) free(c->topic);
	c->topic = strdup(c->ctopic);
      }
      return;
    }
  }
  // no point continuing if not synced or no ops yet 
  if (!c->synced || !c->hasop) return;
  // check topic and update
  if (c->ctopic && c->topic) {
    if (strcmp(c->topic,c->ctopic)) {
      putserv("TOPIC %s :%s\r\n",c->channel,c->topic);
    }
  } else if (c->topic) {
    putserv("TOPIC %s :%s\r\n",c->channel,c->topic);
  }
}

/**
 * Fix unwanted modes and wanted modes on channel
 * @param c - Channel
 */
void irc_fix_modes(CHANNEL *c) {
  size_t i;
  char limit[20];
  char tmp[3];
  char *modestr;
  unsigned long modes;
  // Find out modes to drop
  modes = c->modes & c->modes_drop;
  if (modes) {
    modestr = mode2modestr(modes);
    // drop 'em one at a time
    for(i=0;i<strlen(modestr);i++) {
      snprintf(tmp,3,"-%c",modestr[i]);
      if (modestr[i] == 'k')
	putmode(c,tmp,c->key);
      else 
	putmode(c,tmp,NULL);
    }
    free(modestr);
  }
  // Find out modes we should have but don't have. Mask here means
  // that all modes that are set are not set, and then those which are
  // not set, are set. And those which should be set are returned.
  modes = ~(c->modes) & c->modes_keep;
  if (modes) {
    // set modes, one at a time
    modestr = mode2modestr(modes);
    for(i=0;i<strlen(modestr);i++) {
      snprintf(tmp,3,"+%c",modestr[i]);
      if (modestr[i] == 'k')
	putmode(c,tmp,c->key);
      else if (modestr[i] == 'l') {
	snprintf(limit,sizeof limit,"%d",c->limit);
	putmode(c,tmp,limit);
      } else 
        putmode(c,tmp,NULL);
    }
    free(modestr);
  }
}

/**
 * Process nick. This is called in various occasions.
 * @param nick - Nick to process
 */
void irc_process_nick(NICK *nick, int joined) {
  int authed;
  size_t i;
  struct channel_ban_s *ban;
  BAN *iban;
  // if the user joined, we want to voice him at the same time
  authed = joined;
  // if not authed, try to auth by Q auth.  
  if (!nick->user) {
    if ((nick->user = user_find(nick->acct))) {
      print("%s authed as %s",nick->nick,nick->user->user);
      nick->user->lastseen = TIME;
      authed = 1;
    }
  }
  /* Only if we know this one */
  if (nick->user) {
    for(i=0;i<nick->user->nchannels;i++) {
      // skip myself.
      if (nick == irc_get_me()) continue;
      // only if modes & not banned
      if ((nick->user->channels[i].modes != 0) && 
	  !(nick->user->channels[i].modes & USER_BANNED)) 
	// update lastjoin
	nick->user->channels[i].channel->lastjoin = TIME;
      // if I have ops
      if (nick->user->channels[i].channel->hasop) {
	// and user should be banned
	if (nick->user->channels[i].modes & USER_BANNED) {
	  // ban him, if he is not ircop
	  char *mask;
	  if (nick->ircop == 0) {
	    mask = nick_create_mask(nick,mask_smart);
	    putmode(nick->user->channels[i].channel,"+b",mask);
	    channel_add_iban(nick->user->channels[i].channel,mask,"lamer",NULL,TIME,TIME+3600,0);   
	    free(mask);
	  }
	}
	// check ops et al.
	else if (authed && (nick->user->channels[i].modes & USER_OP))
	  putmode(nick->user->channels[i].channel,"+o",nick->nick);
	else if (authed && (nick->user->channels[i].modes & USER_VOICE))
	  putmode(nick->user->channels[i].channel,"+v",nick->nick);
	nick->user->channels[i].channel->lastjoin = TIME;
      }
    }
  }
  // check all channels for the nick. in case he has ill-gotten ops.
  for(i=0;i<nick->nchannels;i++) {
    // check that channel has bitchmode
    // has ops in the channel (both bot and nick)
    // is not ircop
    // is not myself
    // and user should not have ops
    if ((channel_has_service(nick->channels[i].channel,CHANNEL_SERV_BITCHMODE)) &&
	(nick->channels[i].channel->hasop) && 
	(nick->channels[i].modes & NICK_OP) &&
	(nick->ircop == 0) &&
	(nick != irc_get_me()) && 
	!(user_get_channel_modes(nick->user,nick->channels[i].channel) & USER_OP))
      putmode(nick->channels[i].channel,"-o",nick->nick);
    if ((ban = channel_check_ban(nick->channels[i].channel,nick))) {
      // ensure it can be done
      if (channel_has_service(nick->channels[i].channel,CHANNEL_SERV_ENFORCEBANS) || channel_check_iban(nick->channels[i].channel,nick)) {
        const char * reason = "You are BANNED from this channel";
        if (ban->iban && ban->iban->reason)
          reason = ban->iban->reason;
	putkick(nick->channels[i].channel,nick,reason);
      }
      // set ban if matches internal ban
    } else if ((iban = channel_check_iban(nick->channels[i].channel,nick))) {
      // try to ban if not ircop
      if ((nick != irc_get_me())&&!(nick->ircop)) {
	putmode(nick->channels[i].channel,"+b",iban->mask);
      }
    }

    // ban the user if the user has an bare IP, and bare ip not wanted, and user is not authed to services
    if (channel_has_service(nick->channels[i].channel,CHANNEL_SERV_NO_IP) && nick->acct == NULL) {
      // check if user's host is IPv4/IPv6 host. 
      unsigned char data[16]; // we ignore this
      if (inet_pton(AF_INET, nick->host, data) == 1 || inet_pton(AF_INET6, nick->host, data) == 1) {
        char *mask;
        mask = nick_create_mask(nick,mask_smart);
        putmode(nick->channels[i].channel,"+b",mask);
        channel_add_iban(nick->channels[i].channel,mask,"noiphosts","Your reverse is not working and you are not authed to Q",TIME,TIME+500,0);   
        free(mask);
      }
    }
  }
}

/**
 * Called when we get ops.
 * @param channel - Channel where this marvellous thing occured
 */
void irc_handle_gotops(CHANNEL *channel) {
  struct channel_ban_s *ban;
  BAN *iban;
  size_t i;
  // delay if not synced yet. Syncing will invoke this too.
  if (channel->synced == 0) return;
  // Find out if we need to remove any bans that match me.
  for(i=0;i<channel->nnicks;i++) {
    if (channel->nicks[i].nick == irc_get_me()) {
      if ((ban = channel_check_ban(channel,channel->nicks[i].nick)))
	putmode(channel,"-b",ban->mask);
      continue;
    }
    // find out if we should ban any users
    if (channel->nicks[i].nick->user) {
      if (user_get_channel_modes(channel->nicks[i].nick->user,
				 channel) & USER_BANNED) {
	char *mask;
	if (channel->nicks[i].nick->ircop == 0) {
	  mask = nick_create_mask(channel->nicks[i].nick,mask_smart);
	  putmode(channel,"+b",mask);
	  free(mask);
	}
      }
      // or op some users
      else if (!(channel->nicks[i].modes & NICK_OP) &&
	       (user_get_channel_modes(channel->nicks[i].nick->user,
				       channel)
		& USER_OP))
	putmode(channel,"+o",channel->nicks[i].nick->nick);
      // or voice
      else if (!(channel->nicks[i].modes & (NICK_VOICE|NICK_OP)) &&
	       (user_get_channel_modes(channel->nicks[i].nick->user,
				       channel)
		& USER_VOICE))
	putmode(channel,"+v",channel->nicks[i].nick->nick);
      // or perhaps try ban someone
    }
    // or deop someone
    if ((channel_has_service(channel,CHANNEL_SERV_BITCHMODE)) &&
	(channel->hasop) &&
	(channel->nicks[i].nick != irc_get_me()) && 
	(channel->nicks[i].modes & NICK_OP) &&
	(channel->nicks[i].nick->ircop == 0) &&
	!(user_get_channel_modes(channel->nicks[i].nick->user,channel) & USER_OP))
      putmode(channel,"-o",channel->nicks[i].nick->nick);
    
    if ((ban = channel_check_ban(channel,channel->nicks[i].nick)))
      if (channel_has_service(channel,CHANNEL_SERV_ENFORCEBANS) || channel_check_iban(channel,channel->nicks[i].nick)) {
        const char * reason = "You are BANNED from this channel";
        if (ban->iban && ban->iban->reason)
          reason = ban->iban->reason;
        putkick(channel,channel->nicks[i].nick,reason);
      }
    if ((iban = channel_check_iban(channel,channel->nicks[i].nick))) {
      putmode(channel,"+b",iban->mask);
    }
  }
  // fix modes and topic
  irc_fix_modes(channel);
  // no, this won't bork. The topic check was called by topicis already
  irc_fix_topic(channel,NULL);
}

/**
 * Handle part message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_part(SERVER *server, const char **args, int argc) {
  size_t i;
  CHANNEL *c;
  NICK *nick;
  char **idx;

  if (argc < 3) return handler_pass_msg;

  // find up channel
  if (!(c = channel_find(args[2]))) {
    return handler_pass_msg;
  }
  // find up part source.
  idx = split_idx2array(args[0]);
  if (idx) {
    nick = nick_find(idx[0]);
    free(idx[0]);
    free(idx);
  } else {
    print("irc: Received malformed part source %s",args[0]);
    return handler_pass_msg;
  }
  // fix part
  if (nick) {
    nick_unset_channel_modes(nick,c);
    channel_unset_nick_modes(c,nick);
    if (nick == me) {
      // IMPORTANT! If this was ME, we need to clean up loads of stuff
      c->onchan = 0;
      c->hasop = 0;
      if (c->ctopic) free(c->ctopic);
      c->ctopic = NULL;
      if (c->disabled == 0) {
	if (c->key)
	  putserv("JOIN %s %s\r\n",c->channel,c->key);
	else
	  putserv("JOIN %s\r\n",c->channel);
	print("irc: Uups! Parted %s - going right back in",c->channel);
      }
      for(i=0;i<c->nnicks;i++) {
	nick_unset_channel_modes(c->nicks[i].nick,c);
	if (c->nicks[i].nick->nchannels == 0)
	  nick_delete(c->nicks[i].nick->nick);
      }
      for(i=0;i<c->nbans;i++) {
        free(c->bans[i].mask);
        free(c->bans[i].placedby);
      }
      free(c->bans);
      free(c->nicks);
      c->nicks = NULL;
      c->nnicks = 0;
      c->bans = NULL;
      c->nbans = 0;
    } else {
      // this is boring, just delete nick if I must.
      if (nick->nchannels == 0) 
	nick_delete(nick->nick);
    }
  }
  return handler_pass_msg;
}

/**
 * Handle quit message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_quit(SERVER *server, const char **args, int argc) {
  char **idx;

  if (argc < 3) return handler_pass_msg;

  // this is very simple. if it's us, we do it in 'disconnected' phase
  idx = split_idx2array(args[0]);
  if (idx) {
    if (nick_find(idx[0]) != me)
      nick_delete(idx[0]);
    free(idx[0]);
    free(idx);
  } else {
    print("irc: Received malformed quit source %s",args[0]);
  }
  return handler_pass_msg;
}

/**
 * Handle kick message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_kick(SERVER *server, const char **args, int argc) {
  size_t i;
  CHANNEL *c;
  NICK *nick;
  if (argc < 4) return handler_pass_msg;

  if (!(c = channel_find(args[2]))) {
    print("irc: Received kick for unknown channel %s",args[2]);
    return handler_pass_msg;
  }
  // VERY similar to part, the args just has changed.
  nick = nick_find(args[3]);
  if (nick) {
    nick_unset_channel_modes(nick,c);
    channel_unset_nick_modes(c,nick);
    if (nick == me) {
      c->onchan = 0;
      c->hasop = 0;
      if (c->ctopic) free(c->ctopic);
      c->ctopic = NULL;
      print("irc: I was kicked from %s by %s",c->channel,args[0]);
      for(i=0;i<c->nnicks;i++) {
        nick_unset_channel_modes(c->nicks[i].nick,c);
        if (c->nicks[i].nick->nchannels == 0)
          nick_delete(c->nicks[i].nick->nick);
      }
      // free all bans as well, mmkay?
      for(i=0;i<c->nbans;i++) {
	free(c->bans[i].mask);
	free(c->bans[i].placedby);
      }
      free(c->bans);
      free(c->nicks);
      c->nicks = NULL;
      c->nnicks = 0;
      c->bans = NULL;
      c->nbans = 0;
    } else {
      // drop nick if needed
      if (nick->nchannels == 0)
        nick_delete(nick->nick);
    }
  }  
  return handler_pass_msg;
}

/**
 * Handle nick message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_nick(SERVER *server, const char **args, int argc) {
  NICK *nick;
  char **idx;

  if (argc < 3) return handler_pass_msg;

  idx = split_idx2array(args[0]);
  // simple as eating bread
  if (idx) {
    nick = nick_find(idx[0]);
    if (nick == NULL) {
      nick = nick_create(args[2]);
      nick->ident = strdup(idx[1]);
      nick->host = strdup(idx[2]);
    } else {
      nick_rename(nick,args[2]);
    }
    free(idx[0]);
    free(idx);
  } else {
    print("irc: Malformed NICK message source %s",args[0]);
  }
  return handler_pass_msg;
}

void irc_handle_ctcp(NICK *nick, char **args, int argc) {
  struct tm *tm;
  time_t t;
  if (!strcmp(args[0],"VERSION")) {
    puttext("NOTICE %s :\001VERSION HBS 1.0 by cmouse\001\r\n",nick->nick);
  } else if (!strcmp(args[0],"ACTION")) {
    return;
  } else if (!strcmp(args[0],"TIME")) {
    t = TIME;
    tm = localtime(&t);
    puttext("NOTICE %s :\001TIME %s\001\r\n",nick->nick,asctime(tm));
  } else if (!strcmp(args[0],"PING")) {
    if (argc > 2) {
      puttext("NOTICE %s :\001PING %s %s\001\r\n",nick->nick,args[1],args[2]);
    } else if (argc > 1) {
      puttext("NOTICE %s :\001PING %s\001\r\n",nick->nick,args[1]);
    } else {
      puttext("NOTICE %s :\001PING\001\r\n",nick->nick);
    }
  } else if (!strcmp(args[0],"CHAT")) {
    console_command_chat(nick,NULL,"chat",NULL,0);
  }
  print("Received CTCP %s from %s",args[0],nick->nick);
}

char *irc_extract_ctcp(NICK *nick, const char *msg) {
  char *rv,*ch1,*ch2,*och;
  char **args;
  int argc;
  rv = strdup(msg);
  if (!strchr(rv,'\001')) return rv;
  och = rv;
  while((ch1 = strchr(och,'\001'))) {
    // do not support broken CTCP
    if ((ch2 = strchr(ch1+1,'\001'))==NULL) break;
    *ch1 = '\0';
    *ch2 = '\0';
    if (!string_split(ch1+1,0,&args,&argc)) {
      char *cp;
      for(cp=args[0];*cp;cp++) *cp = toupper(*cp);
      irc_handle_ctcp(nick,args,argc);
    }
    memmove(ch1,ch2+1,strlen(ch2+1)+1);
    och = ch1;
  }
  rv = realloc(rv, strlen(rv)+1);
  return rv;
}

/**
 * Handle channel/private message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_privmsg(SERVER *server, const char **args, int argc) {
  NICK *nick;
  char **idx,*msg;

  if (argc < 4) return handler_pass_msg;

  idx = split_idx2array(args[0]);
  nick = NULL;
  if (idx) {
    nick = nick_find(idx[0]);
    free(idx[0]);
    free(idx);
  }
  // we do not speak to strangers and to ourselves
  if (!nick || (nick == irc_get_me())) return handler_pass_msg;
  if (nick->user) 
	  nick->user->lastseen = TIME;
  msg = irc_extract_ctcp(nick,args[3]);

  if (strlen(msg)==0) {
    // just a ctcp
    free(msg);
    return handler_pass_msg;
  }

  // if it wasn't from channel just toss it into parser.
  if ((*args[2] != '#') &&
      (*args[2] != '&') &&
      (*args[2] != '+')) {
    // no parse? put it on console.
    if (command_try_parse(nick,channel_find(args[2]),msg)) {
      // actually, we don't put it on to console unless required
      if (irc_hide_messages == 0)
	print("[%s] %s",args[0],msg);
    }
  } else {
    CHANNEL *c;
    c = channel_find(args[2]);
    // try parse. if ircop, NICK_OP and channel has lamer service
    // do not even process.
    if (command_try_parse(nick,c,msg) &&
	(nick->ircop == 0) &&
	!(nick_get_channel_modes(nick,c) & NICK_OP) &&
	(user_get_channel_modes(nick->user,c) == 0) &&
	(channel_has_service(c,CHANNEL_SERV_LAMER))) {
      // update lamer control system
      char md[MD5_DIGEST_LENGTH];
      struct nick_channel_s *tmp;
      size_t i;
      char *mask;
      tmp = NULL;
      // 15 SECOND TIMEOUT
      for(i=0;i<nick->nchannels && (tmp == NULL);i++) 
	if (nick->channels[i].channel == c) tmp = nick->channels + i;
      if (TIME - tmp->lamer.laststamp > c->lamercontrol.maxtime) {
	memset(tmp->lamer.last,0,sizeof tmp->lamer.last);
	tmp->lamer.repeat = 0;
	tmp->lamer.lines = 0;
      }
      tmp->lamer.lines++;
      md5sum(msg,strlen(msg),md);
      if (!memcmp(tmp->lamer.last,md,sizeof md)) 
	tmp->lamer.repeat++;
      else {
	memcpy(tmp->lamer.last,md,sizeof md);
	tmp->lamer.repeat = 0;
      }
      tmp->lamer.laststamp = TIME;
      if (tmp->lamer.repeat > c->lamercontrol.maxrepeat+1) {
	if ((mask = nick_create_mask(nick,mask_smart))) {
	  channel_add_iban(c,mask,"lamer control","You were told not to repeat yourself", TIME,TIME+3600,0);
	  putmode(c,"+b",mask);
	  free(mask);
	}	
      } else if (tmp->lamer.repeat > c->lamercontrol.maxrepeat) {
	puttext("NOTICE %s :Do not repeat\r\n",nick->nick);
      }
      if (tmp->lamer.lines > c->lamercontrol.maxlines + 1) {
	if ((mask = nick_create_mask(nick,mask_smart))) {
          channel_add_iban(c,mask,"lamer control","You are flooding the channel",TIME,TIME+3600,0);
          putmode(c,"+b",mask);
          free(mask);
        }    
      } else if (tmp->lamer.lines > c->lamercontrol.maxlines) 
	puttext("NOTICE %s :Stop flooding\r\n",nick->nick);
    }
  }
  free(msg);
  return handler_pass_msg;
}

/**
 * Handle notice message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_notice(SERVER *server, const char **args, int argc) {
  // display in console if not channel wide
  if (argc < 4) return handler_pass_msg;
  if (irc_hide_messages) return handler_pass_msg;
  if ((*args[2] != '#') &&
      (*args[2] != '&') &&
      (*args[2] != '+'))
    print("-%s- %s",args[0],args[3]);
  return handler_pass_msg;
}

/**
 * Handle someone getting op or voice.
 * @param c - Channel
 * @param dir - + or -
 * @param mode - mode 
 * @param target - target individual
 * @param ircop - is ircop doing this?
 */
void irc_handle_opvoice(CHANNEL *c, int dir, unsigned long mode, const char *target, int ircop) {
  NICK *nick;

  if (!(nick = nick_find(target))) nick = nick_create(target);
  switch(mode) {
  case MODE_o:
    if (dir) {
      // this is +o
      nick_set_channel_modes(nick,c,nick_get_channel_modes(nick,c)|NICK_OP);
      // normal bitchmode check
      if ((channel_has_service(c,CHANNEL_SERV_BITCHMODE)) &&
	  (nick->ircop == 0) &&
          (ircop == 0) &&
          (nick != irc_get_me()) &&
	  (c->synced) &&
	  !(user_get_channel_modes(nick->user,c) & USER_OP))
	putmode(c,"-o",nick->nick);
    } else {
      // otherwise just update things
      nick_set_channel_modes(nick,c,nick_get_channel_modes(nick,c)&(~NICK_OP));
    }
    // if it's me and we got ops
    if (nick == me) {
      if ((c->hasop == 0) && (dir == 1)) {
	// update stuff
	c->hasop = 1;
	c->lastop = TIME;
	irc_handle_gotops(c);
      } else if (dir == 0) {
	// get us back opped.
	c->hasop = 0;
	chanserv_message(c,requestop);
      }
    }
    break;
  case MODE_v:
    // update information only. 
    if (dir) {
      nick_set_channel_modes(nick,c,nick_get_channel_modes(nick,c)|NICK_VOICE);
    } else {
      nick_set_channel_modes(nick,c,nick_get_channel_modes(nick,c)&(~NICK_VOICE));
    }
    break;
  }
}

/**
 * Handles ban setting
 * @param c - Channel
 * @param mask - Ban mask
 */
void irc_channel_ban(CHANNEL *c, const char *mask) {
  size_t i;
  struct channel_ban_s *b = channel_check_ban_mask(c, mask);

  // if the ban matches ME, bail out.
  if (!nick_match_mask(irc_get_me(),mask)) return;
  // find out everyone involved.
  for(i=0;i<c->nnicks;i++) {
    if (c->nicks[i].nick == irc_get_me()) continue;
    if (!nick_match_mask(c->nicks[i].nick,mask) && (c->nicks[i].nick->ircop == 0)) { 
     if (channel_has_service(c,CHANNEL_SERV_ENFORCEBANS) || channel_check_iban_mask(c, mask)) { 
        const char * reason = "You are BANNED from this channel";
        if (b && b->iban->reason)
          reason = b->iban->reason;
       putkick(c,c->nicks[i].nick,reason);
      }
    }
  }
}

/**
 * Handle mode message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_mode(SERVER *server, const char **args, int argc) {
  NICK *src;
  int argp;
  unsigned long mode;
  int dir;
  char **idx;
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  dir = 1;

  // we do like nothing but channel modes
  if ((*args[2] != '#') &&
      (*args[2] != '&') &&
      (*args[2] != '+')) return handler_pass_msg;
  // parse source
  src = NULL;
  if ((idx = split_idx2array(args[0]))) {
    src = nick_find(idx[0]);
    free(idx[0]);
    free(idx);
  }
  // find out channel
  if (!(c = channel_find(args[2]))) {
    print("irc: Received mode for unknown channel %s",args[2]);
    return handler_pass_msg;
  }

  argp = 3;

  /* Parse mode line */
  for(;*args[3];args[3]++) {
    if (*args[3] == '+') {
      dir = 1;
      continue;
    } else if (*args[3] == '-') {
      dir = 0;
      continue;
    } else if ((mode = modechar2mode(*args[3])) == 0) continue;
    switch(mode) {
    case MODE_b:
      // handle bans
      if (argp < argc) argp++; else break;
      if (dir) {
	channel_add_ban(c,args[argp],args[0],TIME);
	irc_channel_ban(c,args[argp]);
	if (src && (src->ircop == 0) && !nick_match_mask(irc_get_me(),args[argp]))
	  putmode(c,"-b",args[argp]);
      } else {
	channel_del_ban(c,args[argp]);
      }
      break;
      // handle op and voice
    case MODE_o:
    case MODE_v:
      if (argp < argc) argp++; else break;
      irc_handle_opvoice(c,dir,mode,args[argp],(src?src->ircop:0));
      break;
    case MODE_k:
      // handle key
      if (argp < argc) argp++; else break;
      if (dir) {
	if (strcmp(args[argp],"*")) {
	  if (c->key) free(c->key);
	  c->key = strdup(args[argp]);
	}
	c->modes |= MODE_k;
      } else {
	if (c->key) free(c->key);
	c->key = NULL;
	c->modes &= (~MODE_k);
      }
      break;
    case MODE_l:
      // and handle limit
      if (dir) {
	if (argp < argc) argp++; else break;
	c->limit = s_atoi(args[argp]);
	c->modes |= MODE_l;
      } else {
	c->limit = 0;
	c->modes &= (~MODE_l);
      }
      break;
    default:
      // just append mode
      if (dir) 
	c->modes |= mode;
      else
	c->modes &= (~mode);
    }
  }     
  // fix modes if opped.
  if (c->hasop) irc_fix_modes(c);
  return handler_pass_msg;
}

/**
 * ask for ops and do initial WHOX
 * @param opaque - ignore 
 */
void irc_ask_ops(void *opaque) {
  CHANNEL *c;
  c = channel_find((char*)opaque);
  free(opaque);
  if (c == NULL) return;
  c->whoed = TIME;
  putwhox(c->channel,"c%%cuhnfa");
  if (c->hasop == 0)
    chanserv_message(c,requestop);
}

/**
 * Handle join message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_join(SERVER *server, const char **args, int argc) {
  CHANNEL *c;
  char uname[MAX_USERNAME_LEN+1];
  NICK *nick;
  char **idx;

  if (argc < 3) return handler_pass_msg;
  
  if (!(c = channel_find(args[2]))) {
    print("irc: Received join for unknown channel %s",args[2]);
    return handler_pass_msg;
  }

  if (channel_has_service(c, CHANNEL_SERV_JOIN_FLOOD_PROTECT)) {
    if (TIME - c->join_flood_timer > 10)
      c->join_flood_counter = 0;

    c->join_flood_counter++;
    if (c->join_flood_protect == 0 && c->join_flood_counter > 3) {
      putmode(c, "+M", NULL);
      timer_register(TIME + 60, irc_channel_unprotect, strdup(c->channel));
      c->join_flood_protect = 1;
    }
  }

  // find out who caused this
  idx = split_idx2array(args[0]);
  if (idx) {
    nick = nick_create(idx[0]);
    // update nick struct if needed
    if (!nick->ident) nick->ident = strdup(idx[1]);
    if (!nick->host) nick->host = strdup(idx[2]);
    // find out account name
    if (!nick->acct && strstr(idx[2],NICK_AUTHED_HOSTMASK)) {
      sscanf(idx[2],"%[^.]",uname);
      nick->acct = strdup(uname);
    }
    // if it's me then do all the necessary things
    if (nick == me) {
      size_t i;
      c->laston = TIME;
      c->onchan = 1;
      print("Joined channel %s",c->channel);
      putserv("MODE %s\r\n",c->channel);
      putserv("MODE %s +b\r\n",c->channel);
      for(i=0;i<c->nbans;i++) {
              free(c->bans[i].mask);
              free(c->bans[i].placedby);
      }
      free(c->bans);
      c->bans = NULL;
      c->nbans = 0;
      timer_register(TIME+3,irc_ask_ops,strdup(c->channel));
    } else if (!nick->acct) {
      // do not do WHOX if WHOX on it's way
      // here is a possible race condition but it's very small
      if (c->synced) 
	putwhox(nick->nick,"n%%cuhnfa");
    }
    // update nick/channel linkings
    nick_set_channel_modes(nick,c,NICK_ONCHAN);
    channel_set_nick_modes(c,nick,NICK_ONCHAN);
    irc_process_nick(nick,1);
    // handle autovoice
    if (c->hasop && (nick != me))
      if (channel_has_service(c,CHANNEL_SERV_AUTOVOICE))
	putmode(c,"+v",nick->nick);
    free(idx[0]);
    free(idx);
  } else {
    print("irc: Malformed JOIN message source %s",args[0]);
  }
  return handler_pass_msg;
}  

/**
 * Handle topic message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_topic(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 3) return handler_pass_msg;

  if (!(c = channel_find(args[2]))) {
    print("irc: Received topic for unknown channel %s",args[2]);
    return handler_pass_msg;
  }
  // change current topic
  if ((argc < 4) || (strlen(args[3])<1)) {
    if (c->ctopic) free(c->ctopic);
    c->ctopic = NULL;
  } else {
    c->ctopic = strdup(args[3]);
  }
  // fix if needed
  irc_fix_topic(c,args[0]);
  return handler_pass_msg;
}

/**
 * Handle mode-is message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_modeis(SERVER *server, const char **args, int argc) {
  int argp;
  unsigned long mode;
  int dir;
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  dir = 1;

  // only channel modes are liked
  if ((*args[3] != '#') &&
      (*args[3] != '&') &&
      (*args[3] != '+')) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received mode for unknown channel %s",args[3]);
    return handler_pass_msg;
  }

  argp = 4;

  /* Parse mode line */
  for(;*args[4];args[4]++) {
    if (*args[4] == '+') {
      dir = 1;
      continue;
    } else if (*args[4] == '-') {
      dir = 0;
      continue;
    } else if ((mode = modechar2mode(*args[4])) == 0) continue;
    switch(mode) {
      // keys
    case MODE_k:
      if (argp < argc) argp++; else break;
      if (dir) {
        if (strcmp(args[argp],"*")) {
          if (c->key) free(c->key);
          c->key = strdup(args[argp]);
        }
        c->modes |= MODE_k;
      } else {
        if (c->key) free(c->key);
        c->key = NULL;
        c->modes &= (~MODE_k);
      }
      break;
      // limits
    case MODE_l:
      if (dir) {
        if (argp < argc) argp++; else break;
        c->limit = s_atoi(args[argp]);
        c->modes |= MODE_l;
      } else {
        c->limit = 0;
        c->modes &= (~MODE_l);
      }
      break;
      // others
    default:
      if (dir)
        c->modes |= mode;
      else
        c->modes &= (~mode);
    }
  }
  // fix modes if possible
  if (c->hasop) irc_fix_modes(c);
  return handler_pass_msg;
}

/**
 * Handle normal WHO message. NB: In QUAKENET (namely asuka servers) this 
 *  should NOT HAPPEN EVER. If it happens the bot will cease to operate.
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_who(SERVER *server, const char **args, int argc) {
  CHANNEL *c;
  NICK *nick;
  int flags;
  char uname[MAX_USERNAME_LEN+1];

  if (argc < 9) return handler_pass_msg;

#ifndef NO_WARN_ON_WHO
  print("WARNING! I received WHO message instead of WHOX - BAD thing");
#endif
  c = channel_find(args[3]);
  nick = nick_create(args[7]);

  // update nick structurE
  if (nick) {
    if (!nick->ident) nick->ident = strdup(args[4]);
    if (!nick->host) nick->host = strdup(args[5]);
    if (!nick->acct && strstr(args[5],NICK_AUTHED_HOSTMASK)) {
      sscanf(args[5],"%[^.]",uname);
      nick->acct = strdup(uname);
    }
    // if some channel was provided use it too
    flags = 0;
    for(;*args[8];args[8]++) {
      // process channel flags. IRCOP status is very nice to know
      if (*args[8] == '+') flags |= NICK_VOICE;
      else if (*args[8] == '@') flags |= NICK_OP;
      else if (*args[8] == '*') nick->ircop = 1;
    }
    // because of the above IRCOP check this is done here.
    if (c) {
      nick_set_channel_modes(nick,c,flags);
      channel_set_nick_modes(c,nick,flags);
      if (!nick_match_mask(nick,CHANSERV_Q_IDX))
        channel_set_service(c,CHANNEL_SERV_Q);
      else if (!nick_match_mask(nick,CHANSERV_L_IDX))
        channel_set_service(c,CHANNEL_SERV_L);
    }
    // process nick as well
    irc_process_nick(nick,0);
  }
  return handler_pass_msg;
}

/**
 * Handle WHOX message. 
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_whox(SERVER *server, const char **args, int argc) {
  /* Proper WHOX, nice */
  CHANNEL *c;
  NICK *nick;
  int flags;

  if (argc < 9) return handler_pass_msg;

  c = channel_find(args[3]);
  nick = nick_create(args[6]);

  // create and update nick structure
  if (nick) {
    if (!nick->ident) nick->ident = strdup(args[4]);
    if (!nick->host) nick->host = strdup(args[5]);
    if (!nick->acct && strcmp(args[8],"0")) {
      nick->acct = strdup(args[8]);
    }
    flags = 0;
    for(;*args[7];args[7]++) {
      if (*args[7] == '+') flags |= NICK_VOICE;
      else if (*args[7] == '@') flags |= NICK_OP;
      else if (*args[7] == '*') nick->ircop = 1; // ircop test
    }
    // if some channel was provided update that too
    if (c) {
      nick_set_channel_modes(nick,c,flags);
      channel_set_nick_modes(c,nick,flags);
      if (!nick_match_mask(nick,CHANSERV_Q_IDX)) 
	channel_set_service(c,CHANNEL_SERV_Q);
      else if (!nick_match_mask(nick,CHANSERV_L_IDX))
	channel_set_service(c,CHANNEL_SERV_L);
    }
    // process nick
    irc_process_nick(nick,0);
  }
  return handler_pass_msg;
}

/**
 * Handle topic-is message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_topicis(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 5) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received topic for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  // just update current topic
  if (c->ctopic) free(c->ctopic);
  c->ctopic = strdup(args[4]);
  return handler_pass_msg;
}

/**
 * Handle topic-set-by message
 * this will naturally follow topic-is message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_topicsetby(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received topic-set-by for unknown channel %s",args[2]);
    return handler_pass_msg;
  }
  // fix topic if needed
  irc_fix_topic(c,args[3]);
  return handler_pass_msg;
}

/**
 * Handle no-topic message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_notopic(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 3) return handler_pass_msg;

  if (!(c = channel_find(args[2]))) {
    print("irc: Received no-topic for unknown channel %s",args[2]);
    return handler_pass_msg;
  }
  // Remove topic.
  if (c->ctopic) free(c->ctopic);
  c->ctopic = NULL;
  // Fix topic if needed.
  irc_fix_topic(c,NULL);
  return handler_pass_msg;
}

/**
 * Handle names message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_names(SERVER *server, const char **args, int argc) {
  NICK *nick;
  CHANNEL *c;
  char **names,*cp;
  int nnames,i,flags;

  if (argc < 6) return handler_pass_msg;

  /* handle names request is easy. */
  if (!(c = channel_find(args[4]))) {
    print("irc: Received names for unknown channel %s",args[4]);
    return handler_pass_msg;
  }
  // this is a split and go
  string_split(args[5],0,&names,&nnames);
  for(i=0;i<nnames;i++) {
    // for each nick
    flags = 0;
    for(cp = names[i];*cp;cp++) {
      if (*cp == '+') flags |= NICK_VOICE;
      else if (*cp == '@') flags |= NICK_OP;
      else break;
    }
    // create and update nick structure
    nick = nick_create(cp);
    if (nick == me) {
      if (flags & NICK_OP) {
	// update lastop if we have ops
	c->lastop = TIME;
	c->hasop = 1;
      }
    }
    // update linking
    nick_set_channel_modes(nick,c,flags);
    channel_set_nick_modes(c,nick,flags);
  }
  // free temporary memory
  if (names) {
    free(names[0]);
    free(names);
  }
  return handler_pass_msg;
}

/**
 * Handle ban-is message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_banis(SERVER *server, const char **args, int argc) {
  CHANNEL *c;
  struct channel_ban_s *b;

  if (argc < 7) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received ban list for unknown channel %s",args[3]);
    return handler_pass_msg;
  }

  // update bans for the channel. 
  channel_add_ban(c,args[4],args[5],(time_t)s_atoll(args[6]));
  b = channel_check_ban_mask(c,args[4]); // should not be null

  // if ban directed to me, remove it
  if (irc_get_me() && !nick_match_mask(irc_get_me(),args[4]) && c->hasop)
    putmode(c,"-b",args[4]);
  else if (c->hasop) {
    // enforce bans
    size_t i;
    for(i=0;i<c->nnicks;i++) {
      if (!nick_match_mask(c->nicks[i].nick,args[4])) {
	if (channel_has_service(c,CHANNEL_SERV_ENFORCEBANS) || channel_check_iban_mask(c,args[4])) {
          const char * reason = "You are BANNED from this channel";
          if (b && b->iban->reason)
            reason = b->iban->reason;
	  putkick(c,c->nicks[i].nick,reason);
	}
      }
    }
  }
  return handler_pass_msg;
}

/**
 * Handle disconnect message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_disconnect(SERVER *server, const char **args, int argc) {
  // delete all nicks
  nick_flush();
  me = NULL;
  // reset all channels
  channel_reset();
  return handler_pass_msg;
}

/**
 * Handle new-host message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_newhost(SERVER *server, const char **args, int argc) {
  if (argc < 4) return handler_pass_msg;

  if (me->host) free(me->host);
  // update mi host
  me->host = strdup(args[3]);
  return handler_pass_msg;
}

/**
 * Handle too-many-channels message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_toomanychan(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received error for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  print("irc: Disabling excess channel %s",c->channel);
  c->disabled = 1;
  return handler_pass_msg;
}

/**
 * Handle banned message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_banned(SERVER *server, const char **args, int argc) {
  CHANNEL *c;
  
  if (argc < 4) return handler_pass_msg;
  
  if (!(c = channel_find(args[3]))) {
    print("irc: Received you-are-banned for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  // try to get invitation for the channel... of course invite is wrong here..
  // if channel has Q...
  if (channel_has_service(c, CHANNEL_SERV_Q)) {
	  chanserv_message(c,unbanall);
  } else {
	  chanserv_message(c,invite);
  }
  return handler_pass_msg;
}

/**
 * Handle fullchan message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_fullchan(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received error for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  chanserv_message(c,invite);
  return handler_pass_msg;
}

/**
 * Handle inviteonly message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_inviteonly(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;
  
  if (!(c = channel_find(args[3]))) {
    print("irc: Received error for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  chanserv_message(c,invite);
  return handler_pass_msg;
}

/**
 * Handle need-key message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_needkey(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received error for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  chanserv_message(c,invite);
  return handler_pass_msg;
}

/**
 * Handle invite message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_invite(SERVER *server, const char **args, int argc) {
  CHANNEL *c;

  if (argc < 4) return handler_pass_msg;

  if (!(c = channel_find(args[3]))) {
    print("irc: Received invite for unknown channel %s",args[3]);
    return handler_pass_msg;
  }
  // join, naturally..
  putserv("JOIN %s\r\n",c->channel);
  return handler_pass_msg;
}

/**
 * Handle end-of-who message
 * @param server - Current server
 * @param args - arguments
 * @param argc - argument count
 */
handler_retval_t irc_handle_endofwho(SERVER *server, const char **args, int argc) {
  CHANNEL *c;
  char *buf,*ptr,*nptr;

  if (argc < 4) return handler_pass_msg;

  /* Only care for channel who */
  if ((*args[3] != '#')&&
      (*args[3] != '&')&&
      (*args[3] != '+')) return handler_pass_msg;
  buf = strdup(args[3]);
  ptr = buf;
  // make all end-of-who channels synced.
  while(ptr) {
    nptr = strchr(ptr,',');
    if (nptr) {
      *nptr = '\0';
      nptr++;
    }
    if (!(c = channel_find(ptr))) 
      continue;
    // synchronize channel
    if (c->synced == 0) {
      c->synced = 1;
      irc_handle_gotops(c);
    }
    ptr = nptr;
  }
  free(buf);
  return handler_pass_msg;
}

/**
 * Timed weed process. Done every day.
 * @param opaque - ignore
 */
void irc_timer_weed(void *opaque) {
  irc_weed();
  timer_register(TIME+86400,irc_timer_weed,NULL);
}

/**
 * Timed save all files process. Done every hour.
 * @param opaque - ignored.
 */
void irc_timer_save(void *opaque) {
  irc_store();
  timer_register(TIME+3600,irc_timer_save,NULL);
}

/**
 * Set up IRC system.
 * @return 1 if failed, 0 if ok
 */
int irc_setup(void) {
  if (channel_load(config_getval("global","channelfile"))) return 1;
  if (user_load(config_getval("global","userfile"))) return 1;
  command_setup();
  irc_hide_messages = s_atoi(config_getval("global","hidemessages"));
  /* setup "few" handlers */
  server_register_handler("NICK", irc_handle_nick);
  server_register_handler("PRIVMSG", irc_handle_privmsg);
  server_register_handler("NOTICE", irc_handle_notice);
  server_register_handler("MODE", irc_handle_mode);
  server_register_handler("JOIN", irc_handle_join);
  server_register_handler("TOPIC", irc_handle_topic);
  server_register_handler("PART", irc_handle_part);
  server_register_handler("QUIT", irc_handle_quit);
  server_register_handler("KICK", irc_handle_kick);
  server_register_handler("INVITE", irc_handle_invite);
  server_register_handler("315", irc_handle_endofwho);
  server_register_handler("324", irc_handle_modeis);
  server_register_handler("352", irc_handle_who);
  server_register_handler("354", irc_handle_whox);
  server_register_handler("332", irc_handle_topicis);
  server_register_handler("331", irc_handle_notopic);
  server_register_handler("353", irc_handle_names);
  server_register_handler("367", irc_handle_banis);
  server_register_handler("396", irc_handle_newhost);
  server_register_handler("405", irc_handle_toomanychan);
  server_register_handler("465", irc_handle_banned);
  server_register_handler("466", irc_handle_banned);
  server_register_handler("471", irc_handle_fullchan);
  server_register_handler("473", irc_handle_inviteonly);
  server_register_handler("474", irc_handle_banned);
  server_register_handler("475", irc_handle_needkey);
  server_register_handler("DISCONNECTED", irc_handle_disconnect);
  timer_register(TIME+86400,irc_timer_weed,NULL);
  timer_register(TIME+3600,irc_timer_save,NULL);
  chanserv_init();
  return 0;
}

/**
 * Store all files
 */
void irc_store(void) {
  channels_store(config_getval("global","channelfile"));
  user_store(config_getval("global","userfile"));
}

/**
 * Weed a user
 * @param u - User
 * @return 0 if not weeded, 1 if weeded
 */
int irc_weed_user(USER *u) {
  size_t i;
  // do not remove admins
  if (u->admin || u->bot) return 0;
  // DO NOT remove banned users
  for(i=0;i<u->nchannels;i++) 
    if (u->channels[i].modes & USER_BANNED) return 0;
  if (TIME - u->lastseen > MAX_AGE_SEEN) {
    print("WEED: Removing %s - not seen for a long time",u->user);
    user_delete(u->user);
    return 1;
  }
  if (u->nchannels == 0) {
    print("WEED: Removing %s - has no channels",u->user);
    user_delete(u->user);
    return 1;
  }
  return 0;
}
   
/**
 * Weed a channel
 * @param c - Channel to weed
 * @return 0 if not weeded, 1 if weeded
 */
int irc_weed_channel(CHANNEL *c) {
  int ops;
  size_t i;
  if (c->permanent) return 0;
  if ((c->hasop == 0) && (TIME - c->lastop > MAX_AGE_NOOPS)) {
    print("WEED: Removing %s - no ops for a long time",c->channel);
    channel_delete(c->channel);
    return 1;
  }
  if ((c->onchan == 0) && (TIME - c->laston > MAX_AGE_NOTON)) {
    print("WEED: Removing %s - not on for a long time",c->channel);
    channel_delete(c->channel);
    return 1;
  }
  ops = 0;
  for(i=0;i<c->nnicks;i++)
    if (c->nicks[i].modes & NICK_OP) ops++;
  // last join is useful only if there are no registered users
  if ((TIME - c->lastjoin > MAX_AGE_NOJOIN) &&
      (ops == 0)) {
    print("WEED: Removing %s - no one has joined for a long time",c->channel);
    channel_delete(c->channel);
    return 1;
  }
  return 0;
}

/**
 * Weed user and channel files. Why here? Because IRC does handle NICK and
 * CHANNEL and USER files all the time. Natural place.
 */
void irc_weed(void) {
  CHANNEL *c;
  USER *u;
  size_t i,dc,du;
  size_t nchans;
  // To avoid problems: DO NOT RUN WEED IF THE BOT IS _NOT_ CONNECTED
  if (!server_isconnected()) {
   print("WEED: Cancelled - bot not connected to IRC");
   return;
  }
  print("Weed process started");
  /* Remove channels, then users - not forgetting to check nicks */
  // Ensure that long sessions are not removed by accident
  dc = du = 0;
  nick_touch_users();
  nchans = array_count(channel_get_array());
  // weed all channels first.
  for(i=0;i<nchans;i++) {
    c = *(CHANNEL**)array_get_index(channel_get_array(),i);
    if (!c->synced) {
      print("WEED: %s not synced yet - skipping...", c->channel);
      continue;
    }
    if (irc_weed_channel(c)) {
      dc++;
      i--;
      nchans--;
    }
  }    
  nchans = array_count(user_get_array());
  // weed all users. If owner gets removed, channel stays. Until tomorrow.
  for(i=0;i<nchans;i++) {
    u = *(USER**)array_get_index(user_get_array(),i);
    if (irc_weed_user(u)) {
      du++;
      i--;
      nchans--;
    }
  }
  // Details of the weed process.
  print("Weed process terminated - Removed " FSO " channels and " FSO " users",dc,du);
}

/* Removes +M from a protected channel */
void irc_channel_unprotect(void *param) 
{
  char *chan = (char*)param;
  CHANNEL *c = channel_find(chan);
  free(chan);

  if (c)  
    putmode(c, "-M", NULL);
} 
