#include "hbs.h"

void chanmod_command_addchan(NICK *nick, CHANNEL *channel, const char *cmd,
			     const char **args, int argc);
void chanmod_command_delchan(NICK *nick, CHANNEL *channel, const char *cmd,
			      const char **args, int argc);
void chanmod_command_setchan(NICK *nick, CHANNEL *channel, const char *cmd,
			     const char **args, int argc);
void chanmod_command_chaninfo(NICK *nick, CHANNEL *channel, const char *cmd,
			      const char **args, int argc);
