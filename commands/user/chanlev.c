#include "user.h"

struct user_walker_s
{
  CHANNEL *channel;
  char **owner;
  char **master;
  char **op;
  char **voice;
  char **banned;
  size_t nowner;
  size_t nmaster;
  size_t nop;
  size_t nvoice;
  size_t nbanned;
};

#define USER_ADD_TO_CONTROL(user,level,nlevel) \
control->nlevel++;\
control->level = realloc(control->level,sizeof(char**)*control->nlevel);\
control->level[control->nlevel-1] = user->user

void user_walker (void *elem, void *opaque)
{
  struct user_walker_s *control;
  USER *user;
  int flags;
  user = *(USER **) elem;
  control = (struct user_walker_s *) opaque;

  flags = user_get_channel_modes (user, control->channel);
  if (flags)
    {
      if (flags & USER_OWNER)
	{
	  USER_ADD_TO_CONTROL (user, owner, nowner);
	}
      else if (flags & USER_MASTER)
	{
	  USER_ADD_TO_CONTROL (user, master, nmaster);
	}
      else if (flags & USER_OP)
	{
	  USER_ADD_TO_CONTROL (user, op, nop);
	}
      else if (flags & USER_VOICE)
	{
	  USER_ADD_TO_CONTROL (user, voice, nvoice);
	}
      else if (flags & USER_BANNED)
	{
	  USER_ADD_TO_CONTROL (user, banned, nbanned);
	}
    }
}

void user_conprint_chanlev (CONSOLE * console, char **list, size_t lsize,
			    const char *suffix)
{
  size_t i;
  for (i = 0; i < lsize; i++)
    console_write (console, " %-34s %s\r\n", list[i], suffix);
}

void user_print_chanlev (NICK * nick, char **list, size_t lsize,
			 const char *prefix)
{
  size_t i;
  char buf[300];
  sprintf (buf, "\002%s\002: ", prefix);
  i = 0;
  while (i < lsize)
    {
      if (300 - strlen (buf) < strlen (list[i]))
	{
	  puttext ("NOTICE %s :%s\r\n", nick->nick, buf);
	  sprintf (buf, "\002%s\002: ", prefix);
	}
      strcat (buf, list[i]);
      strcat (buf, " ");
      i++;
    }
  puttext ("NOTICE %s :%s\r\n", nick->nick, buf);
}

void user_concommand_chanlev (CONSOLE * console, const char *cmd,
			      const char **args, int argc)
{
  struct user_walker_s user;
  if (argc < 1)
    {
      console_write (console, "Usage: %s #channel\r\n", cmd);
      return;
    }
  memset (&user, 0, sizeof user);
  if (!(user.channel = channel_find (args[0])))
    {
      console_write (console, "No such channel %s\r\n", args[0]);
      return;
    }
  user_walk (user_walker, &user);
  console_write (console, "Access control list for %s\r\n",
		 user.channel->channel);
  console_write (console, "%-34s %s\r\n", "Username", "Role");
  if (user.owner)
    {
      user_conprint_chanlev (console, user.owner, user.nowner, "owner");
      free (user.owner);
    }
  if (user.master)
    {
      user_conprint_chanlev (console, user.master, user.nmaster, "master");
      free (user.master);
    }
  if (user.op)
    {
      user_conprint_chanlev (console, user.op, user.nop, "op");
      free (user.op);
    }
  if (user.voice)
    {
      user_conprint_chanlev (console, user.voice, user.nvoice, "voice");
      free (user.voice);
    }
  if (user.banned)
    {
      user_conprint_chanlev (console, user.banned, user.nbanned, "banned");
      free (user.banned);
    }
  console_write (console, "End of record\r\n");
}

void user_command_chanlev (NICK * nick, CHANNEL * channel, const char *cmd,
			   const char **args, int argc)
{
  struct user_walker_s user;
  memset (&user, 0, sizeof user);
  if (argc > 0)
    {
      if (!(user.channel = channel_find (args[0])))
	{
	  puttext ("NOTICE %s :No such channel %s\r\n", nick->nick, args[0]);
	  return;
	}
    }
  else
    {
      user.channel = channel;
    }
  user_walk (user_walker, &user);
  if (user.owner)
    {
      user_print_chanlev (nick, user.owner, user.nowner, "owner(s)");
      free (user.owner);
    }
  if (user.master)
    {
      user_print_chanlev (nick, user.master, user.nmaster, "master(s)");
      free (user.master);
    }
  if (user.op)
    {
      user_print_chanlev (nick, user.op, user.nop, "op(s)");
      free (user.op);
    }
  if (user.voice)
    {
      user_print_chanlev (nick, user.voice, user.nvoice, "voice(s)");
      free (user.voice);
    }
  if (user.banned)
    {
      user_print_chanlev (nick, user.banned, user.nbanned, "banned");
      free (user.banned);
    }
  puttext ("NOTICE %s :Total " FSO " owners, " FSO " masters, " FSO " ops, "
	   FSO " voices, " FSO " banned\r\n", nick->nick, user.nowner,
	   user.nmaster, user.nop, user.nvoice, user.nbanned);
}
