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
 *
 * Main inclusion files
 * defines all prototypes and structures etc. needed by the bot
 */

#ifndef HBS_H
#define HBS_H 1

#define _GNU_SOURCE 1

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
# define strchr index
# define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
# define memcpy(d, s, n) bcopy ((s), (d), (n))
# define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif
#include <ctype.h>
#include <netdb.h>
#include <stdarg.h>
#include <limits.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
# include <sys/time.h>
# else
# include <time.h>
# endif
#endif
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <pwd.h>
#include <signal.h>
#include <dlfcn.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#if SIZEOF_SIZE_T == SIZEOF_INT
 #define FSO "%u"
#else
 #define FSO "%lu"
#endif

#pragma GCC visibility push(default)

#include "modes.h"
#include "ircd_chattr.h"
#include "channel.h"

// Some very useful defines indeed

#define VISIBLE __attribute__((visibility("default")))

// Find out current unix timestamp
#define TIME time((time_t*)NULL)

// Maximum message length to server DO NOT ALTER!
#define SERVER_MLEN 512
// Maximum message length to chanserv
#define CHANSERV_MESSAGE_LEN 300
// Per network config
#ifdef IN_QNET
 #define CHANSERV_MESSAGE_TARGET_Q "Q@cserve.quakenet.org"
 #define CHANSERV_MESSAGE_TARGET_L "L@lightweight.quakenet.org"
 #define CHANSERV_Q_IDX "Q!TheQBot@cserve.quakenet.org"
 #define CHANSERV_L_IDX "L!TheLBot@lightweight.quakenet.org"
 #define NICK_AUTHED_HOSTMASK ".users.quakenet.org"
#endif
#ifdef IN_HNET
 #define CHANSERV_MESSAGE_TARGET_Q "Q@theqbot.helpersnet.eu.org"
 #define CHANSERV_MESSAGE_TARGET_L "L@lightweight.helpersnet.eu.org"
 #define CHANSERV_Q_IDX "Q!TheQBot@theqbot.helpersnet.eu.org"
 #define CHANSERV_L_IDX "L!TheLBot@lightweight.helpersnet.eu.org"
 #define NICK_AUTHED_HOSTMASK ".users.helpersnet.eu.org"
#endif
// Max nickname length
#define MAX_NICKLEN 32
// Max username length (must be <= NICKLEN)
#define MAX_USERNAME_LEN 32
// Max hostname length
#define MAX_HOST_LEN 255
// Max channel name length
#define MAX_CHANNELNAME_LEN 255
// Max channel key length
#define MAX_CHANNELKEY_LEN 30
// Max length of a single mode str (+gsgsga)
#define MAX_MODESTRING_LEN 30
// Default channel ban time
#define DEF_CHANBAN_TIME 3600
// Default lamercontrol ban time
#define LAMERCONTROL_DEF_BAN 600
// Default reset time for a single record
#define LAMERCONTROL_RESET_TIME 15
// Default max flood value (kick value is +1)
#define LAMERCONTROL_MAX_FLOOD 5
// Default max repeat value (kick value is +1)
#define LAMERCONTROL_MAX_REPEAT 2
// Console buffer length
#define CONSOLE_BUFLEN 1024
// Console defines
#define CONSOLE_CONNECT 0x1
#define CONSOLE_ACCEPT 0x2
#define CONSOLE_MAX_AUTH_WAIT 60
#define CONSOLE_MAX_USER_IDLE 7200
#define CONSOLE_MAX_BOT_IDLE 60
// Default ban time
#define DEF_BAN_TIME 7200
// Maximum modes per line on output
#define MAX_MODES_PER_LINE 6
// Max whox users per output line
#define MAX_USER_WHOX_PER_LINE 20
// Max whox channels per output line
#define MAX_CHANNEL_WHOX_PER_LINE 10
// Max mode parameter string length
#define MAX_MODEPARAM_LEN 300
// Who sweep period
#define CHANNEL_WHO_PERIOD 300
// Oidentd config file
#define OIDENTD_FILE ".oidentd.conf"

// Nick status 
#define NICK_OP 0x8
#define NICK_HALFOP 0x4
#define NICK_VOICE 0x2
#define NICK_ONCHAN 0x1

// User levels
#define USER_OWNER 0x10
#define USER_MASTER 0x8
#define USER_OP 0x4
#define USER_VOICE 0x2
#define USER_BANNED 0x1

// Max seen age
#define MAX_AGE_SEEN 86400*30

// Some defines
#define s_atoi(val) (val == NULL?0:atoi((const char*)val))
#define s_atoll(val) (val == NULL?0:strtoll((const char*)val,NULL,10))

typedef enum handler_retval_enum {
  handler_pass_msg,
  handler_drop_msg
} handler_retval_t;

// Forward declarations and type defines
struct nick_s;
struct user_s;
struct channel_s;
struct server_s;
struct module_info_s;
struct ban_s;
struct console_s;
struct botnet_bot_s;

typedef struct nick_s NICK;
typedef struct user_s USER;
typedef struct channel_s CHANNEL;
typedef struct server_s SERVER;
typedef struct ban_s BAN;
typedef struct console_s CONSOLE;

typedef struct module_info_s (*module_info_t)(void);
typedef int (*module_init_t)(void);

// Callback type defines

/* key, element */
typedef int (*array_comparator_t)(const void *,const void *);
/* element, opaque */
typedef void (*array_walker_t)(void *, void *);
/* time, format, arguments */
typedef void (*log_callback_t)(const char *, const char *, va_list);
/* server,args,argc */
typedef handler_retval_t (*server_message_handler_t)(SERVER *, const char **, int);
/* nick,channel,command,args,argc */
typedef void (*command_handler_t)(NICK *, CHANNEL *, const char *, const char **, int);
/* console, command, args, argc */
typedef void (*console_command_t)(CONSOLE *, const char *, const char **, int);
/* filedescriptor */
typedef int (*xpoll_callback_t)(int);

// Enumerations

typedef enum {mask_nuh,
              mask_h,
              mask_smart} nick_masktype_t;

typedef enum {requestop,
              unbanall,
              invite,
              topic,
              chanlev} chanserv_request_t;

// Structures

#ifdef IN_ARRAY_C
typedef struct array_s {
  char *items;
  size_t nmembs;
  size_t size;
  size_t amembs;
  array_comparator_t comparator;
  array_walker_t eraser;
} *ARRAY;
#else
typedef void *ARRAY;
#endif

struct botnet_bot_s {
  char *name;
  int channels;
};

struct module_info_s {
  const char *modname; // module name
  int major; // major version
  int minor; // minor version 
  module_init_t load; // load callback
  module_init_t unload; // unload callback
};

struct nick_s {
  char *nick; // nickname!*@*
  char *ident; // *!ident@*
  char *host; // *!*@host
  char *acct; // Q account name
  int ircop; // is irc operator
  USER *user; // user structure

  struct nick_channel_s {
    CHANNEL *channel;  // channel pointer
    int modes; // status in channel

    struct lamer_control_s {
      int repeat; // repeats
      int lines; // flood-lines
      char last[MD5_DIGEST_LENGTH]; // last line
      time_t laststamp; // last spoke
    } lamer; // lamercontrol
  } *channels; // channels
  size_t nchannels; // number of channels

};

struct user_s {
  char *user; // username
  char pass[SHA_DIGEST_LENGTH]; // hashed password
  
  time_t lastseen; // last seen
  int admin; // is admin
  int bot; // is bot

  struct user_channel_s {
    CHANNEL *channel; // channel pointer
    int modes; // channel level
  } *channels;
  size_t nchannels; // number of channels
};

struct channel_s { 
  char *channel; // channel name
 
  unsigned long modes_keep; // modes to force
  unsigned long modes_drop; // modes to remove
  unsigned long modes; // current modes
  unsigned long services; // active services

  int limit; // channel limit
  char *key; // channel key
  
  time_t created; // when created
  time_t lastop; // when had ops last
  time_t laston; // when last was on chan
  time_t lastjoin; // when last join occured
  time_t whoed; // when was last who sent

  time_t join_flood_timer; // timer for join floods
  int join_flood_counter; // counter for join floods
			  // if there are too many joins
			  // within 5 seconds, +M is set
			  // if floodprotect is set.
  int join_flood_protect; // if +M has been set. 

  int onchan; // on channel?
  int hasop; // have ops?
  int permanent; // is permanent
  int disabled; // is disabled
  int synced;  // is synchronized
  char *topic; // my topic
  char *ctopic; // current topic
  
  struct channel_nick_s {
    NICK *nick; // nick pointer
    int modes; // nick modes (op,voice...)
  } *nicks;
  size_t nnicks; // number of nicks

  struct channel_ban_s {
    char *mask; // channel ban mask
    char *placedby; // who placed it
    time_t when; // when it was placed
    struct ban_s *iban; // if there is an internal ban, link it.
  } *bans;
  size_t nbans; // number of bans

  struct ban_s {
    char *mask; /* ban mask */
    char *placedby; /* who placed it */
    char *reason; /* reason for ban, can act as kick reason */
    time_t when; /* when the ban was set */
    time_t expires; /* when the ban expirs (0 = never) */
    int sticky; /* excempt from dynabans */
  } *ibans;

  struct lamer_control_setup_s {
    int maxrepeat;
    int maxlines;
    int maxtime;
  } lamercontrol;

  size_t nibans; // number of internal bans
};

struct server_common_s {
  char **nicks; // nick array
  size_t nnicks; // number of nicks
  size_t cnick; // current nick
  char *realname; // realname
  char *ident; /* For oidentd support */
};

extern struct server_common_s server_common;
extern int tcp_protocol_number;

struct server_s {
  char *cnick; // current nick
  char *host; // hostname
  char *port; // port number
  char *name; // server name
  char *pass; // server password
  struct addrinfo *addrinfo; // getaddrinfo
  struct addrinfo *curaddr; // current addrinfo
  struct sockaddr_in bindaddr; // address to bind 
  int fd; // socket
  int disconnect; // should we disconnect
  FILE *ident; // ident file pointer
  time_t timeout; // timeout counter

  enum {
    server_disconnected, 
    server_oident,
    server_preconnect,
    server_connected,
    server_ok
  } status; // server status

  struct ob_s {
    struct timeval lr; /* Last reset */
    int msec; /* Reset msecs */
    int mlines; /* Max lines */
    int clines; /* Current lines */
  } ob;

  char buffer[SERVER_MLEN+1]; // input buffer

#ifdef SIZEOF_UNSIGNED_LONG_LONG
  unsigned long long rx; // status info of read bytes
  unsigned long long tx; // status info of sent bytes
#else
  unsigned long rx; 
  unsigned long tx;
#endif
  SERVER *next; // pointer to next server
};

struct console_s {
  USER *user; // user pointer
  void (*handler)(struct console_s*,const char*); // current message handler
  char buffer[CONSOLE_BUFLEN+1]; // input buffer
  int fd; // socket
  int disconnect; // should we disconnect
  int dir; // for botnet
  time_t last; // last message
  int pinged; // has bot been pinged

  enum {
    console_connect,
    console_new,
    console_auth,
    console_ok } state; // console socket status
  struct console_text_queue_s {
    char *text;
    struct console_text_queue_s *next; // output lines
  } *start,*stop;
};

ARRAY array_create(size_t size, array_comparator_t cmp, array_walker_t eraser);
void *array_add(ARRAY array, const void *key);
void *array_find(ARRAY array, const void *key);
int array_delete(ARRAY array, const void *key);
void array_destroy(ARRAY array, void *opaque);
int array_drop(ARRAY array, const void *key);
void array_walk(ARRAY array, array_walker_t walker, void *opaque);
void *array_get_index(ARRAY array, size_t element);
size_t array_count(ARRAY array);
void array_clear(ARRAY array);

int strrfccmp(const char *a, const char *b);
int match(const char *mask, const char *string);
int rfc_match(const char *mask, const char *string);
unsigned long modechar2mode(char modechar);
char *mode2modestr(unsigned long modes);
unsigned long modestr2mode(const char *modestr);
char **split_idx2array(const char *idx);
int string_split(const char *message, int count, char ***args, int *argc);
char server_gen_letter(void);
char *fill_template(const char *template);
char *sha1sum(const char *src);
char *sha1sum_hex(const char *src);
void md5sum(const char *src, size_t srclen, char *dest);
void bin2hex(char *result, const char *bin, size_t len);
void hex2bin(char *result, const char *hex);
const char *int_to_base64(unsigned int val);
void *fixed_array_add(void *array, void *elem, size_t esize, size_t ecount);
void *fixed_array_delete(void *array, size_t esize, size_t epos, size_t ecount);

NICK *nick_create(const char *nick);
NICK *nick_find(const char *nick);
void nick_rename(NICK *nick, const char *newnick);
void nick_delete(const char *nick);
void nick_set_channel_modes(NICK *nick, CHANNEL *channel, int modes);
int nick_get_channel_modes(NICK *nick, CHANNEL *channel);
void nick_unset_channel_modes(NICK *nick, CHANNEL *channel);
void nick_flush(void);
char *nick_create_mask(NICK *n, nick_masktype_t masktype);
int nick_match_mask(NICK *n, const char *mask);
void nick_touch_users(void);
void nick_remove_user(USER *u);

CHANNEL *channel_create(const char *channel);
CHANNEL *channel_find(const char *channel);
void channel_delete(const char *channel);
void channel_set_nick_modes(CHANNEL *channel, NICK *nick, int modes);
int channel_get_nick_modes(CHANNEL *channel, NICK *nick);
void channel_unset_nick_modes(CHANNEL *channel, NICK *nick);
void channel_reset(void);
int channel_load(const char *file);
int channels_store(const char *file);
void channel_del_iban(CHANNEL *c, const char *mask);
void channel_add_iban(CHANNEL *c, const char *mask, const char *who, const char *reason, time_t when, time_t expires, int sticky);
void channel_del_ban(CHANNEL *c, const char *mask);
void channel_add_ban(CHANNEL *c, const char *mask, const char *who, time_t when);
struct channel_ban_s *channel_check_ban(CHANNEL *c, NICK *nick);
struct channel_ban_s *channel_check_ban_mask(CHANNEL *c, const char *mask);
BAN *channel_check_iban(CHANNEL *c, NICK *nick);
BAN *channel_check_iban_mask(CHANNEL *c, const char *mask);
void channel_clean_bans(CHANNEL *c);
ARRAY channel_get_array(void);

USER *user_create(const char *user);
USER *user_find(const char *user);
void user_delete(const char *user);
void user_set_channel_modes(USER *user, CHANNEL *channel, int modes);
int user_get_channel_modes(USER *user, CHANNEL *channel);
void user_unset_channel_modes(USER *user, CHANNEL *channel);
int user_load(const char *file);
int user_store(const char *file);
void user_walk(array_walker_t walker, void *opaque);
int user_check_pass(USER *user, const char *pw);
ARRAY user_get_array(void);

void putfast(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void putmode(CHANNEL *channel, char *mode, const char *param);
void putkick(CHANNEL *channel, NICK *nick, const char *reason);
void putwhox(const char *target, const char *params);
void putserv(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void puttext(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
char *server_deque_next_message(void);
void server_queue_clean(void);

void log_register(log_callback_t cb);
void log_free(void);
void vprint(const char *fmt, va_list va);
void print(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

void server_setup();
void server_add(const char *host, const char *port, const char *pass, const char *bind, int msec, int bytes);
void server_disconnect(void);
int server_poll(void);
int server_isconnected(void);
void server_check(void);
void server_release_oidentd(void);
void server_add_nick(const char *nick);
const char *server_get_name(void);
void server_set_wait(time_t t);
SERVER *server_get_current(void);

void server_register_handler(const char *token, server_message_handler_t handlerptr);
void server_unregister_handler(const char *token, server_message_handler_t handlerptr);
void server_fire_event(SERVER *server, const char *token, const char **args, int argc);
void server_setup_handlers(void);

int config_read(const char *file);
const char *config_getval(const char *sect, const char *key);
void config_free(void);

int irc_setup(void);
void irc_store(void);
void irc_set_me(const char *nick);
NICK *irc_get_me(void);
void irc_fix_modes(CHANNEL *c);
xmlNodePtr xmlGetNodeByName(xmlNodePtr node, const char *name);
xmlNodePtr xmlGetNodeWithAttrValue(xmlNodePtr node, const char *name, const char *attr, const char *value);
const char *xmlGetNodeAttrValue(xmlNodePtr node, const char *attr);
const char *xmlAttrGetValue(xmlAttrPtr ptr);
const char *xmlNodeGetValue(xmlNodePtr ptr);
char *xmlNodeGetLat1Value(xmlNodePtr ptr);
void irc_process_nick(NICK *nick, int joined);
void irc_weed(void);

int chanserv_message(CHANNEL *channel, chanserv_request_t request, ...);
int chanserv_init(void);

typedef void (*timer_callback_t)(void *);
int timer_register(time_t when, timer_callback_t cb, void *opaque);
void timers_run(void);

int command_register(const char *command, int needchan, int params,
                     int umask, int admin, command_handler_t handler);	     
int command_unregister(const char *command);
int command_try_parse(NICK *nick, CHANNEL *channel, const char *line);
void command_setup(void);

int module_setup(void);
int module_load(const char *module);
int module_unload(const char *module);
int modules_load(const char *file);
const struct module_info_s **module_list(void);
void modules_unload(void);
void module_run_ops(void);

void console_register_command(const char *command, int params, console_command_t cb, const char *help);
void console_unregister_command(const char *command);
void console_write_all(CONSOLE *src, const char *fmt, ...) __attribute__((format (printf, 2, 3)));
void console_vwrite(CONSOLE *console, const char *fmt, va_list va);
void console_write(CONSOLE *console, const char *fmt, ...) __attribute__((format (printf, 2, 3)));
void console_relay_speak(CONSOLE *src, CONSOLE *console, const char *fmt, ...) __attribute__((format (printf, 3, 4)));
void console_relay(CONSOLE *src, const char *fmt, ...) __attribute__((format (printf, 2, 3)));
void console_register_botcommand(const char *command, int params, console_command_t cb);
void console_unregister_botcommand(const char *command);
const char *console_get_myname(void);
void console_disconnect(CONSOLE *console);
void console_poll(void);
int console_setup(void);
void console_cleanup(void);
void console_set_ip(struct in_addr *ip);
extern void console_parse_bot(CONSOLE *console, const char *message);
void console_relay(CONSOLE *src, const char *fmt, ...);

void xpoll_register_fd(int fd, xpoll_callback_t read, xpoll_callback_t write,
		       xpoll_callback_t close);
void xpoll_unregister_fd(int fd);
void xpoll_unregister_cb(xpoll_callback_t func);
void xpoll(void);

void botnet_init(void);
int botnet_has_bot(const char *name);
int botnet_add_bot(const char *name);
void botnet_set_channels(const char *name, int count);
void botnet_remove_bot(const char *name);
void botnet_walker(array_walker_t walker, void *opaque);

char * const * main_get_argv();
int main_get_runcond();

#pragma GCC visibility pop

#ifndef HAVE_SRANDDEV
void sranddev(void);
#endif
#endif
