#include "hbs.h"

int user_level_to_mode(const char *level);
const char *user_mode_to_level(int level);
void user_command_adduser(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_command_deluser(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_command_chanlev(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_concommand_chanlev(CONSOLE *console, const char *cmd, const char **args, int argc);
void user_command_admin(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_command_unadmin(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_command_lamer(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_command_password(NICK *nick, CHANNEL *channel, const char *cmd, const char **args, int argc);
void user_command_whois(CONSOLE *console, const char *cmd, const char **args, int argc);
