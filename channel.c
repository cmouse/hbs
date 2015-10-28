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
#include "hbs.h"

/* 
 * Channels are added to array by ptr to avoid problems
 */

ARRAY channels = 0;

/**
 * Channel comparator for array
 * @param a - CHANNEL **channel
 * @param b - const char *key
 */
int channel_cmp(const void *a, const void *b) {
  const CHANNEL *n;
  const char *s;
  n = *(const CHANNEL**)b;
  s = (const char*)a;
  return strrfccmp(n->channel,s);
}

/**
 * Used to remove channel from all users
 * @param elem - USER **user
 * @param opaque - CHANNEL *channel
 */
void channel_delchan_walker(void *elem, void *opaque) {
  CHANNEL *channel;
  USER *user;
  user = *(USER**)elem;
  channel = (CHANNEL*)opaque;
  // Removes user's flags
  user_unset_channel_modes(user,channel);
}

/**
 * Channel eraser called by array_delete et al.
 * @param a - CHANNEL **channel
 * @param o - unused.
 */
void channel_free(void *a, void *o) {
  size_t i;
  CHANNEL *n;
  n = *(CHANNEL**)a;
  /* Remove all users to channel links */
  user_walk(channel_delchan_walker,n);
  /* Remove all nicks */
  if (n->nicks) {
    for(i=0;i<n->nnicks;i++) {
      nick_unset_channel_modes(n->nicks[i].nick,n);
      if (n->nicks[i].nick->nchannels == 0)
	nick_delete(n->nicks[i].nick->nick);
    }
    free(n->nicks);
  }
  /* Remove all bans */
  if (n->bans) {
    for(i=0;i<n->nbans;i++) {
      free(n->bans[i].mask);
      free(n->bans[i].placedby);
    }
    free(n->bans);
  }
  /* Internal bans */
  if (n->ibans) {
    for(i=0;i<n->nibans;i++) {
      free(n->ibans[i].mask);
      free(n->ibans[i].placedby);
      if (n->ibans[i].reason)
        free(n->ibans[i].reason);
    }
    free(n->ibans);
  }
  /* Free topic */
  if (n->topic) free(n->topic);
  if (n->ctopic) free(n->ctopic);
  /* Key */
  if (n->key) free(n->key);
  /* Channel name */
  free(n->channel);
  /* Pointer itself */
  free(n);
  /* And the array_delete will remove pointer to pointer */
}

/**
 * Used to create a new channel 
 * @param channel - Channel name to create
 * @return Pointer to a channel structure
 */
CHANNEL *channel_create(const char *channel) {
  CHANNEL **ptr;
  CHANNEL *new;
  // Init channel system if needed 
  if (channels == NULL) {
    channels = array_create(sizeof(CHANNEL*),channel_cmp,channel_free);
  }
  // Add new channel (*ptr will be non-null if channel already exists)
  ptr = array_add(channels,channel);
  /* Already fixed */
  if ((*ptr) != NULL) return *ptr;
  // Allocate space for channel structure
  new = (*ptr) = malloc(sizeof(CHANNEL));
  memset(new,0,sizeof(CHANNEL));
  // Setup and return pointer
  new->channel = strdup(channel);
  return new;
}

/**
 * Looks for channel by name
 * @param channel - Channel name to look up
 * @return pointer to channel structure if found, NULL otherwise
 */
CHANNEL *channel_find(const char *channel) {
  CHANNEL **ptr;
  if (channel == NULL) return NULL;
  /* Just call array find */
  if ((ptr = array_find(channels,channel)) == NULL) return NULL;
  /* Return the encapsulated pointer */
  return *ptr;
}

/**
 * Deletes a channel 
 */
void channel_delete(const char *channel) {
  putserv("PART %s\r\n",channel);
  array_delete(channels,channel);
}

/**
 * Sets up link between channel and nick with modes
 * @param channel - Channel to link
 * @param nick - nick to link
 * @param modes - status
 */
void channel_set_nick_modes(CHANNEL *channel, NICK *nick, int modes) {
  size_t n;
  // Looks nick up, in case it's already here.
  for(n=0;n<channel->nnicks;n++) {
    if (channel->nicks[n].nick == nick) {
      channel->nicks[n].modes = modes;
      return;
    }
  }
  // Adds new entry to table
  channel->nnicks++;
  channel->nicks = realloc(channel->nicks,
			   sizeof(struct channel_nick_s)*channel->nnicks);
  channel->nicks[channel->nnicks-1].nick = nick;
  channel->nicks[channel->nnicks-1].modes = modes;
}

/**
 * Returns the possible modes for nick on a channel. 0 means not on chan
 * and above those are all the modes user can have. See hbs.h for more.
 * @param channel - Channel to use
 * @nick - Nick to lookup
 * @return nick's modes on this channel
 */
int channel_get_nick_modes(CHANNEL *channel, NICK *nick) {
  size_t n;
  for(n=0;n<channel->nnicks;n++) {
    if (channel->nicks[n].nick == nick) {
      return channel->nicks[n].modes;
    }
  }
  return 0;
}

/**
 * Removes nick from channel. Note that this alone is not enough, and the
 * channel must be removed from nick too.
 * @param channel - Channel to use 
 * @param nick - Nick to remove
 */
void channel_unset_nick_modes(CHANNEL *channel, NICK *nick) {
  size_t n;
  if (channel == NULL) return;
  for(n=0;n<channel->nnicks;n++) {
    if (channel->nicks[n].nick == nick) {
      // if this wasn't last of array, move everything "left" by 1
      if (n < channel->nnicks-1) 
	memmove(channel->nicks+n,channel->nicks+n+1,sizeof(struct channel_nick_s)*(channel->nnicks - n - 1));
      channel->nnicks--;
      // reallocate if necessary.
      if (channel->nnicks > 0)
	channel->nicks = realloc(channel->nicks,channel->nnicks*sizeof(struct channel_nick_s));
      else {
	channel->nicks = NULL;
	channel->nnicks = 0;
      }
    }
  }
}

/**
 * Resets and cleans up all channel structures
 */
void channel_reset(void) {
  CHANNEL *c;
  size_t i,t;
  if (channels) {
    // Go thru all channels
    for(i=0;i<array_count(channels);i++) {
      c = *(CHANNEL**)array_get_index(channels,i);
      // Free all bans
      for(t=0;t<c->nbans;t++) {
	free(c->bans[t].mask);
	free(c->bans[t].placedby);
      }
      c->bans = NULL;
      c->nbans = 0;
      // As the reset is called only when nicks are being reset too
      // we can cheat death here.
      free(c->nicks);
      // Ensure we can start from clean plate
      c->nicks = NULL;
      c->whoed = 0;
      c->nnicks = 0;
      c->onchan = 0;
      c->hasop = 0;
      c->modes = 0;
      c->synced = 0;
      // Note that TOPIC is not freed, only CURRENT topic
      if (c->ctopic) free(c->ctopic);
      c->ctopic = NULL;
    }
  }
}

/**
 * Called by run_timers when it is time to check channels. This is also
 * called by chanserv.c when it's time to join channels.
 *
 * @param opaque - ignored
 */
void channels_check(void *opaque) {
  CHANNEL *c;
  char *keybuf;
  char *joinbuf;
  size_t i;
  keybuf = joinbuf = NULL;
  // For all channels
  for(i=0;i<array_count(channels);i++) {
    c = *(CHANNEL**)array_get_index(channels,i);
    // If the channel is indeed disabled, skip all checks
    if (c->disabled == 0) {
      // Not on channel? Join channel
      if (c->onchan == 0) {
	// reallocate joinbuf.
	if (joinbuf) 
	  joinbuf = realloc(joinbuf,strlen(joinbuf)+strlen(c->channel)+2);
	else {
	  joinbuf = realloc(joinbuf,+strlen(c->channel)+2);
	  *joinbuf = '\0';
	}
	// append a channel to it.
	strcat(joinbuf,",");
	strcat(joinbuf,c->channel);
	// Do the same for keybuf, if needed
	if (c->key) {
	  if (keybuf)
	    keybuf = realloc(keybuf,strlen(keybuf)+strlen(c->key)+2);
	  else {
	    keybuf = realloc(keybuf,strlen(c->key)+2);
	    *keybuf = '\0';
	  }
	  strcat(keybuf,",");
	  strcat(keybuf,c->key);
	}
      } else if (c->hasop == 0) {
	// ask for ops
	chanserv_message(c,requestop);
      }
      channel_clean_bans(c);
      // And eventually, do a periodical WHO once in a while
      if (c->onchan && (TIME - c->whoed > CHANNEL_WHO_PERIOD)) {
	putwhox(c->channel,"c%%cuhnfa");
	c->whoed = TIME;
      }
    }
  }
  // Somewhere we should joinna?
  if (joinbuf) {
    // We have keyssa?
    if (keybuf) {
      putserv("JOIN %s %s\r\n",joinbuf+1,keybuf+1);
      free(keybuf);
    } else {
      putserv("JOIN %s\r\n",joinbuf+1);
    }
    free(joinbuf);
  }
  // recheck in 30 seconds
  timer_register(TIME + 30, channels_check, NULL);
}

void channels_join(void) {
  // Check channels after 3 seconds, to allow propagation around IRC network
  // Reduces chance of QUIT :registered. While it does not bother us,
  // it bothers others.
  timer_register(TIME + 3, channels_check, NULL);
}

/**
 * Subparser for mode string for a channel
 * @param channel - Channel to load
 * @param modestr - modestr to parse
 */
void channel_load_modes(CHANNEL *channel, const char *modestr) {
  // for + modes
  unsigned long keep;
  // for - modes
  unsigned long drop;
  // pointer
  unsigned long *ptr;

  // Initially all goes to keep
  keep = drop = 0;
  ptr = &keep;

  for(;*modestr;modestr++) {
    // switch ptr if needed
    switch(*modestr) {
    case '+':
      ptr = &keep;
      break;
    case '-':
      ptr = &drop;
      break;
    default:
      // append mode to ptr
      *ptr |= modechar2mode(*modestr); 
    }
  }
  // update channel structure
  channel->modes_keep = keep;
  channel->modes_drop = drop;
}

/**
 * Subparser to read in channel lamer control settings
 *
 * @param channel - Channel to use
 * @param bans - xml document to use
 */
void channel_load_lamercontrol(CHANNEL *channel, xmlNodePtr lamercontrol) {
  xmlNodePtr ptr;
  ptr = lamercontrol->children;
  while(ptr) {
    if (ptr->type == XML_ELEMENT_NODE) {
      if (!strcasecmp((const char*)ptr->name,"maxrepeat")) {
	channel->lamercontrol.maxrepeat = s_atoi(xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"maxlines")) {
	channel->lamercontrol.maxlines = s_atoi(xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"maxtime")) {
	channel->lamercontrol.maxtime = s_atoi(xmlNodeGetValue(ptr));
      }
    }
    ptr = ptr->next;
  }
}

/**
 * Subparser to read in channel bans
 * 
 * @param channel - Channel to use
 * @param bans - xml document to use
 */
void channel_load_bans(CHANNEL *channel, xmlNodePtr bans) {
  BAN ban;
  xmlNodePtr ptr,ptr2;
  ptr = bans->children;
  while(ptr) {
    // look for element notdes (these contain useful stuff)
    if (ptr->type == XML_ELEMENT_NODE) {
      ptr2 = ptr->children;
      // clean up ban structure
      memset(&ban,0,sizeof ban);
      // process ban entry
      while(ptr2) {
	if (ptr2->type == XML_ELEMENT_NODE) {
	  if (!strcasecmp((const char*)ptr2->name,"mask")) 
	    ban.mask = xmlNodeGetLat1Value(ptr2);
	  else if (!strcasecmp((const char*)ptr2->name,"placedby")) 
            ban.placedby = xmlNodeGetLat1Value(ptr2);
	  else if (!strcasecmp((const char*)ptr2->name,"when")) 
	    ban.when = (time_t)s_atoll((char*)xmlNodeGetValue(ptr2));
	  else if (!strcasecmp((const char*)ptr2->name,"expires"))
	    ban.expires = (time_t)s_atoll((char*)xmlNodeGetValue(ptr2));
	  else if (!strcasecmp((const char*)ptr2->name,"sticky"))
	    ban.sticky = (s_atoi(xmlNodeGetValue(ptr2))?1:0);
          else if (!strcasecmp((const char*)ptr2->name,"reason"))
            ban.reason = xmlNodeGetLat1Value(ptr2);
	}
	// while it lasts
	ptr2 = ptr2->next;
      }
      // add internal ban 
      channel_add_iban(channel,ban.mask,ban.placedby,ban.reason,ban.when,ban.expires,ban.sticky);
      if (ban.mask) free(ban.mask);
      if (ban.placedby) free(ban.placedby);
      if (ban.reason) free(ban.reason);
    }
    // next entry
    ptr = ptr->next;
  }
}

/**
 * Main channel parser. Parses xmlNodePtr into a channel.
 * @param channel - xml document describing a channel
 */
void channel_load_channel(xmlNodePtr channel) {
  CHANNEL *chan;
  char *cptr;
  xmlNodePtr ptr;
  ptr = xmlGetNodeByName(channel,"name");
  // attempt to create a channel
  if (ptr) {
    cptr = xmlNodeGetLat1Value(ptr);
    chan = channel_create(cptr);
    if (chan == NULL) {
      print("channel: Unable to load channel %s - skipping",cptr);
      free(cptr);
      return;
    }
    free(cptr);
  } else {
    // no name?!? skip this bastard.
    print("channel: Unable to load channel name - skipping record");
    return;
  }
  // make some default assumptions
  chan->lamercontrol.maxtime = LAMERCONTROL_RESET_TIME;
  chan->lamercontrol.maxlines =  LAMERCONTROL_MAX_FLOOD;
  chan->lamercontrol.maxrepeat =  LAMERCONTROL_MAX_REPEAT;
  ptr = channel->children;
  while(ptr) {
    if (ptr->type == XML_ELEMENT_NODE) {
      if (!strcasecmp((const char*)ptr->name,"modes")) {
	// handle modes
	cptr = xmlNodeGetLat1Value(ptr);
	channel_load_modes(chan,cptr);
	free(cptr);
      } else if (!strcasecmp((const char*)ptr->name,"name")) {
	/* ignore, handled above */
      } else if (!strcasecmp((const char*)ptr->name,"limit")) {
	// channel limit
	chan->limit = s_atoi(xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"key")) {
	// key
	chan->key = strdup(xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"created")) {
	// creation time (see below for fix)
	chan->created = (time_t)s_atoll((const char*)xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"laston")) {
	// last on channel time
	chan->laston = (time_t)s_atoll((const char*)xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"lastop")) {
	// last opped time
	chan->lastop = (time_t)s_atoll((const char*)xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"lastjoin")) {
	// last join time
	chan->lastjoin = (time_t)s_atoll((const char*)xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"services")) {
	// active services (like autovoice or lamercontrol)
	chan->services = (unsigned long)s_atoll((const char*)xmlNodeGetValue(ptr));
      } else if (!strcasecmp((const char*)ptr->name,"permanent")) {
	// is channel permanent (will not be touched by weed)
	chan->permanent = (s_atoi(xmlNodeGetValue(ptr))?1:0);
      } else if (!strcasecmp((const char*)ptr->name,"disabled")) {
	// is channel disabled (will not join)
	chan->disabled = (s_atoi(xmlNodeGetValue(ptr))?1:0);
      } else if (!strcasecmp((const char*)ptr->name,"topic")) {
	const char *topic;
	// topic, the intriguing problem
	// it will be read in as hexadecimal string
	topic = (const char*)xmlNodeGetValue(ptr);
	chan->topic = malloc(strlen(topic)/2+2);
	memset(chan->topic,0,strlen(topic)/2+2);
	hex2bin(chan->topic,topic);
      } else if (!strcasecmp((const char*)ptr->name,"bans")) {
	// run subparser for bans
	channel_load_bans(chan,ptr);
      } else if (!strcasecmp((const char*)ptr->name,"lamercontrol")) {
	// run subparser for lamer control
	channel_load_lamercontrol(chan,ptr);
      } else {
	// alert if rogue parameters are used.
	print("Unknown channel property %s - skipping",ptr->name);
      }
    }
    // next property
    ptr = ptr->next;
  }
  /* Fix missing creation info */
  if (chan->created == 0) chan->created = TIME;
}

/**
 * Loads XML file and parses channels out of it.
 * @param file - XML file to load
 */
int channel_load(const char *file) {
  // document
  xmlDocPtr xmlDoc;
  // pointer to doc
  xmlNodePtr ptr;
  // read in the file
  xmlDoc = xmlReadFile(file, NULL, XML_PARSE_NONET|XML_PARSE_NOCDATA);
  // if failed, warn and return 1
  if (xmlDoc == NULL) {
   xmlErrorPtr error;
   error = xmlGetLastError();
   if (error) {
    char *msg,*ch;
    msg = strdup(error->message);
    if ((ch = strrchr(msg,'\n'))) *ch = '\0';
    if ((ch = strrchr(msg,'\r'))) *ch = '\0';
    print("Config: Unable to parse %s: line %d: %s",file,error->line,msg);
    free(msg);
   } else {
    print("Config: Unable to parse %s",file);
   }
   return 1;
  }
  // get the root of doc
  ptr = xmlDocGetRootElement(xmlDoc);
  // and because it happens to be sgml tag, go to it's siblings
  ptr = ptr->children;
  while(ptr) {
    // parse all ELEMENT nodes, others do not contain sane data for us
    if (ptr->type == XML_ELEMENT_NODE) {
      channel_load_channel(ptr);	
    }      
    // next channel
    ptr = ptr->next;
  }
  // release memory
  xmlFreeDoc(xmlDoc);
  return 0;
}

/**
 * Store channel in XML format. General idea here is to write only that
 * which cannot be done by default values.
 * @param c - channel name
 * @param o - FILE *file
 */
void channel_store(void *c, void *o) {
  char *str;
  size_t i;
  CHANNEL *channel;
  FILE *fout;
  channel = *(CHANNEL**)c;
  fout = (FILE*)o;
  // start of entry
  fprintf(fout,"\t<channel>\n");
  // put name first
  fprintf(fout,"\t\t<name><![CDATA[%s]]></name>\n",channel->channel);
  // if we have modes
  if (channel->modes_keep || channel->modes_drop) {
    // print modestring
    fprintf(fout,"\t\t<modes>");
    // keep modes
    if (channel->modes_keep) {
      str = mode2modestr(channel->modes_keep);
      fprintf(fout,"+%s",str);
      free(str);
    }
    // drop modes
    if (channel->modes_drop) {
      str = mode2modestr(channel->modes_drop);
      fprintf(fout,"-%s",str);
      free(str);
    }
    // end of tag
    fprintf(fout,"</modes>\n");
  }
  // write limit
  if (channel->limit)
    fprintf(fout,"\t\t<limit>%d</limit>\n",channel->limit);
  // write topic
  if (channel->topic) {
    char *ntopic;
    ntopic = malloc(strlen(channel->topic)*2+1);
    memset(ntopic,0,strlen(channel->topic)*2+1);
    bin2hex(ntopic,channel->topic,strlen(channel->topic));
    fprintf(fout,"\t\t<topic><![CDATA[%s]]></topic>\n",ntopic);
    free(ntopic);
  }
  // channel key
  if (channel->key)
    fprintf(fout,"\t\t<key><![CDATA[%s]]></key>\n",channel->key);
  // active services
  if (channel->services)
    fprintf(fout,"\t\t<services>%lu</services>\n",channel->services);
  // creation time
  if (channel->created)
    fprintf(fout,"\t\t<created>%lu</created>\n",channel->created);
  // last op time
  if (channel->lastop)
    fprintf(fout,"\t\t<lastop>%lu</lastop>\n",channel->lastop);
  // last on channel time
  if (channel->laston)
    fprintf(fout,"\t\t<laston>%lu</laston>\n",channel->laston);
  // last joining time
  if (channel->lastjoin)
    fprintf(fout,"\t\t<lastjoin>%lu</lastjoin>\n",channel->lastjoin);
  // if channel is exempt from weed
  if (channel->permanent)
    fprintf(fout,"\t\t<permanent>%d</permanent>\n",channel->permanent);
  // if channel is disabled
  if (channel->disabled)
    fprintf(fout,"\t\t<disabled>%d</disabled>\n",channel->disabled);
  // write all internal bans
  if (channel->ibans) {
    fprintf(fout,"\t\t<bans>\n");
    for(i=0;i<channel->nibans;i++) {
      fprintf(fout,"\t\t\t<ban>\n");
      // mask
      fprintf(fout,"\t\t\t\t<mask><![CDATA[%s]]></mask>\n",
	      channel->ibans[i].mask);
      // who placed it
      fprintf(fout,"\t\t\t\t<placedby><![CDATA[%s]]></placedby>\n",
	      channel->ibans[i].placedby);
      if (channel->ibans[i].reason)
	fprintf(fout,"\t\t\t\t<reason><![CDATA[%s]]></reason>\n",
              channel->ibans[i].reason);
      // when it was placed
      fprintf(fout,"\t\t\t\t<when>%lu</when>\n",channel->ibans[i].when);
      // does it expire
      if (channel->ibans[i].expires)
	fprintf(fout,"\t\t\t\t<expires>%lu</expires>\n",
		channel->ibans[i].expires);
      // issit sticky
      if (channel->ibans[i].sticky)
	fprintf(fout,"\t\t\t\t<sticky>%d</sticky>\n",channel->ibans[i].sticky);
      fprintf(fout,"\t\t\t</ban>\n");
    }
    fprintf(fout,"\t\t</bans>\n");
  }
  fprintf(fout,"\t\t<lamercontrol>\n");
  fprintf(fout,"\t\t\t<maxrepeat>%d</maxrepeat>\n",channel->lamercontrol.maxrepeat);
  fprintf(fout,"\t\t\t<maxlines>%d</maxlines>\n",channel->lamercontrol.maxlines);
  fprintf(fout,"\t\t\t<maxtime>%d</maxtime>\n",channel->lamercontrol.maxtime);
  // end channel
  fprintf(fout,"\t\t</lamercontrol>\n");
  fprintf(fout,"\t</channel>\n");
}

/**
 * Write all channels in XML format
 * @param file - File to write into
 * @return 0 ok, 1 failed
 */
int channels_store(const char *file) {
  FILE *fout;
  /* Begin write process, we write the XML by ourselves... */
  if ((fout = fopen(file,"w")) == NULL) return 1;
  fprintf(fout,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<channels>\n");
  array_walk(channels, channel_store, fout);
  fprintf(fout,"</channels>\n");
  fclose(fout);
  return 0;
}

/**
 * Deletes internal ban(s) matching mask from channel
 * @param c - Channel
 * @param mask - Removal mask
 */
void channel_del_iban(CHANNEL *c, const char *mask) {
  size_t i;
  for(i=0;i<c->nibans;i++) {
    /* Ban matches */
    if (!rfc_match(mask,c->ibans[i].mask)) {
      // Remove ban
      free(c->ibans[i].mask);
      free(c->ibans[i].placedby);
      if (c->ibans[i].reason)
        free(c->ibans[i].reason);
      c->ibans = fixed_array_delete(c->ibans, sizeof(struct ban_s), i,
				    c->nibans);
      c->nibans--;
      i--;
    }
  }
}

/**
 * Delete normal ban, same as above but different array
 * @param c - Channel
 * @param mask - Removal mask
 */
void channel_del_ban(CHANNEL *c, const char *mask) {
  size_t i;
  for(i=0;i<c->nbans;i++) {
    if (!rfc_match(mask, c->bans[i].mask)) {
      c->bans = fixed_array_delete(c->bans, sizeof(struct channel_ban_s),
				   i, c->nbans);
      c->nbans--;
      i--;
    }
  }
}

/**
 * Adds a new channel ban (not internal, set possibly by someone else)
 * @param c - Channel
 * @param mask - Ban mask
 * @param who - Who set it
 * @param when - When was it set (to comply with banis)
 */
void channel_add_ban(CHANNEL *c, const char *mask, const char *who, time_t when) {
  struct channel_ban_s ban;
  // delete all bans that could possibly conflict with this
  channel_del_ban(c,mask);

  // add a ban
  ban.mask = strdup(mask);
  ban.placedby = strdup(who);
  ban.when = when;
  ban.iban = channel_check_iban_mask(c,ban.mask);

  c->bans = fixed_array_add(c->bans, &ban, sizeof(ban), c->nbans);
  c->nbans++;
}

/**
 * Adds a new channel internal ban which is enforced
 * @param c - Channel
 * @param mask - Ban mask
 * @param who - Who set it
 * @param reason - Why was it set (can be NULL)
 * @param when - When was it set (to comply with banis)
 * @param expires - When the ban expires
 * @param sticky - Should ban be stuck (exempt of cleanup)
 */
void channel_add_iban(CHANNEL *c, const char *mask, const char *who, const char *reason, time_t when, time_t expires, int sticky) {
  struct ban_s ban;
  /* Refuse to add expired bans */
  if ((expires > 0) && (expires < TIME)) return;
  channel_del_iban(c,mask);
  ban.mask = strdup(mask);
  ban.placedby = strdup(who);
  ban.when = when;
  ban.reason = strdup(reason);
  // expiration == 0 means permanent ban
  ban.expires = expires;
  ban.sticky = sticky;
  c->ibans = fixed_array_add(c->ibans, &ban, sizeof(ban), c->nibans);
  c->nibans++;
}

/**
 * Checks channel's internal bans against nick. If match, return ban
 * @param c - Channel
 * @param nick - Nick
 * @return iban structure if found or NULL
 */
BAN *channel_check_iban(CHANNEL *c, NICK *nick) {
  BAN *ban;
  size_t i;
  char *mask;
  if (!(mask = nick_create_mask(nick,mask_nuh))) return NULL;
  ban = channel_check_iban_mask(c, mask);
  free(mask);
  return ban;
}

/**
 * Checks if there is a matching ban against a mask. First match returned
 * @param c - Channel
 * @param mask - Mask to check against
 * @return iban structure of NULL if not found
 */
BAN *channel_check_iban_mask(CHANNEL *c, const char *mask) {
  BAN *ban;
  size_t i;
  ban = NULL;
  for(i=0;i<c->nibans;i++) {
    if (!rfc_match(c->ibans[i].mask,mask)) {
      ban = &(c->ibans[i]);
      break;
    }
  }
  if (ban) {
    if ((ban->expires > 0) && (ban->expires < TIME)) {
      channel_del_iban(c,ban->mask);
      return NULL;
    }
  }
  return ban;
}

/**
 * Checks channel's bans against nick. If match, return ban
 * @param c - Channel
 * @param nick - Nick
 * @return ban structure if found or NULL
 */
struct channel_ban_s *channel_check_ban(CHANNEL *c, NICK *nick) {
  struct channel_ban_s *ban;
  size_t i;
  char *mask;
  if (!(mask = nick_create_mask(nick,mask_nuh))) return NULL;
  ban = channel_check_ban_mask(c,mask);
  free(mask);
  return ban;
}

struct channel_ban_s *channel_check_ban_mask(CHANNEL *c, const char *mask) {
  struct channel_ban_s *ban;
  int i;
  ban = NULL;
  for(i=0;i<c->nbans;i++) {
    if (!rfc_match(c->bans[i].mask,mask)) {
      ban = &c->bans[i];
      break;
    }
  }
  return ban;
}

/**
 * Cleans up ban list. Internal bans are removed here, and normal bans
 * when the -b hits channel.
 * @param c - Channel
 * @param 
 */
void channel_clean_bans(CHANNEL *c) {
  struct channel_ban_s *ban;
  BAN *iban;
  size_t i;
  i = 0;
  // go thru all bans
  while(i<c->nibans) {
    iban = c->ibans+i;
    // if it is expired, remove it 
    if ((iban->expires > 0) && (iban->expires < TIME)) {
      // send a -b to make sure it's not on channel
      putmode(c,"-b",iban->mask);
      free(c->ibans[i].mask);
      free(c->ibans[i].placedby);
      c->ibans = fixed_array_delete(c->ibans, sizeof(struct ban_s),
				    i, c->nibans);
      // stay put to make sure you check the next ban
      i--;
      c->nibans--;
    } else {
      // move forward.
      i++;
    }
  }
  i = 0;
  // channel bans are just -b'd because it'll remove them.
  // maybe one should check that this is not an iban?
  while(i<c->nbans) {
    //lookup iban
    size_t t = 0;
    ban = c->bans+i;
    while(t<c->nibans) {
            iban = c->ibans+t;
            if (!strrfccmp(ban->mask, iban->mask)) {
		    if (iban->sticky) goto nextban;
	    }
	    t++;
    }
    if (TIME - ban->when > DEF_CHANBAN_TIME)
      putmode(c,"-b",ban->mask);
nextban: i++;
  }
}

/**
 * Allows accessing all channel entries via walker. 
 * @param walker - walker function
 * @param opaque - parameter to pass for walker
 */ 
void channel_walk(array_walker_t walker, void *opaque) {
  array_walk(channels,walker,opaque);
}

/**
 * Returns the channel ARRAY
 * @return channels array
 */ 
ARRAY channel_get_array(void) {
  return channels;
}
