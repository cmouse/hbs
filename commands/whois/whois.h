#include "hbs.h"

const char *whois_perm(USER *user, CHANNEL *channel);
const char *whois_admin(USER *user);
void whois_command_whoami(NICK *nick, CHANNEL *channel, const char *cmd,
			  const char **args, int argc);
void whois_command_whois(NICK *nick, CHANNEL *channel, const char *cmd,
			 const char **args, int argc);

