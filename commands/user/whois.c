/**
 * This only works in DCC.. sorry
 * 
 * template
 *
 * USER          (has password) (is admin)
 *  Channel               Role
 *   #help                 Owner
 *   #test                 Master
 *   #haters               Banned
 * End of record
 */

#include "user.h"

void user_command_whois (CONSOLE * console, const char *cmd,
			 const char **args, int argc)
{
  size_t i;
  USER *victim;
  if (argc < 1)
    {
      console_write (console, "Usage: %s user\r\n", cmd);
      return;
    }
  if (!(victim = user_find (args[0])))
    {
      console_write (console, "No such user %s\r\n", args[0]);
      return;
    }
  // Oki
  console_write (console, "%-15s %s %s\r\n", victim->user,
		 (*victim->pass ? "(has password)" : "(has no password)"),
		 (victim->admin ? "(is admin)" : "(is not admin)"));
  console_write (console, " %-20s %s\r\n", "Channel", "Role");
  for (i = 0; i < victim->nchannels; i++)
    console_write (console, "  %-20s %s\r\n",
		   victim->channels[i].channel->channel,
		   user_mode_to_level (victim->channels[i].modes));
  console_write (console, "End of record\r\n");
}
