#include "hbs.h"

void ban_command_ban(NICK *nick, CHANNEL *channel, const char *cmd,
		     const char **args, int argc);
void ban_command_unban(NICK *nick, CHANNEL *channel, const char *cmd,
		       const char **args, int argc);
void ban_command_banlist(NICK *nick, CHANNEL *channel, const char *cmd,
			 const char **args, int argc);
void ban_command_permban(NICK *nick, CHANNEL *channel, const char *cmd,
			 const char **args, int argc);
void ban_command_stick(NICK *nick, CHANNEL *channel, const char *cmd,
		                         const char **args, int argc);
void ban_command_unstick(NICK *nick, CHANNEL *channel, const char *cmd,
		                         const char **args, int argc);
