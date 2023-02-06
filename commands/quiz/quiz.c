#include "hbs.h"

#define QUIZTIME 30
#define QUIZFILE "quiz.txt"
#define QUIZCHAN "#advice.quiz"

const struct module_info_s quiz_info_s;
static unsigned long last_q = 0;
static char quiz_a[512] = { 0 };
static int quiz_enabled = 0;

void quiz_stop_quiz (void *opaque)
{
  if (strlen (quiz_a) && (last_q == (long) opaque))
    {
      puttext ("PRIVMSG %s :No one knew the answer which was %s\r\n",
	       QUIZCHAN, quiz_a);
      memset (quiz_a, 0, sizeof quiz_a);
    }
}

handler_retval_t quiz_handle_privmsg (SERVER * server, const char **args,
				      int argc)
{
  char nick[MAX_NICKLEN + 1];
  char *cptr;
  strncpy (nick, args[0],
	   (sizeof nick > strlen (args[0]) ? strlen (args[0]) : sizeof nick));
  if ((cptr = strchr (nick, '!')))
    *cptr = '\0';
  if (strlen (quiz_a) && !strncasecmp (args[3], quiz_a, strlen (quiz_a)))
    {
      /* CORRECT ANSWER!!! */
      memset (quiz_a, 0, sizeof quiz_a);
      puttext ("PRIVMSG %s :Well done %s! Correct answer\r\n", args[2], nick);
      return handler_drop_msg;
    }
  return handler_pass_msg;
}

void quiz_command_quiz (NICK * nick, CHANNEL * channel, const char *cmd,
			const char **args, int argc)
{
  FILE *f;
  unsigned long c, r, i;
  char q[512];
  char *cptr;
  /* Handle quiz enable / disable */
  if ((nick_get_channel_modes (nick, channel) & NICK_OP) == NICK_OP)
    {
      if (argc)
	{
	  if (!strcasecmp (args[0], "enable"))
	    {
	      if (!quiz_enabled)
		{
		  quiz_enabled = 1;
		  puttext ("NOTICE %s :Quiz enabled\r\n", nick->nick);
		}
	      else
		{
		  puttext ("NOTICE %s :Quiz already enabled\r\n", nick->nick);
		}
	    }
	  else if (!strcasecmp (args[0], "disable"))
	    {
	      if (quiz_enabled)
		{
		  memset (quiz_a, 0, sizeof quiz_a);
		  quiz_enabled = 0;
		  puttext ("NOTICE %s :Quiz disabled\r\n", nick->nick);
		}
	      else
		{
		  puttext ("NOTICE %s :Quiz already disabled\r\n",
			   nick->nick);
		}
	    }
	  else
	    {
	      puttext
		("NOTICE %s :Invalid command '%s': must be one of enable,disable\r\n",
		 nick->nick, args[0]);
	    }
	  return;
	}
    }
  if (!quiz_enabled)
    {
      puttext ("NOTICE %s :Quiz not enabled at the moment\r\n", nick->nick);
      return;
    }
  if (strlen (quiz_a))
    {
      puttext ("NOTICE %s :Quiz already running...\r\n", nick->nick);
      return;
    }
  if (strrfccmp (channel->channel, QUIZCHAN))
    {
      puttext ("NOTICE %s :Applicable only on %s\r\n", nick->nick, QUIZCHAN);
      return;
    }
  if (!(f = fopen (QUIZFILE, "r")))
    {
      puttext ("NOTICE %s :Quiz file failed to open\r\n", nick->nick);
      return;
    }
  c = 0;
  fgets (q, sizeof q, f);
  sscanf (q, "%ld", &c);
  if (c)
    {
      r = last_q;
      while (r == last_q)
	r = (random () % c) + 1;
      last_q = r;
      for (i = 0; i < r; i++)
	{
	  fgets (q, sizeof q, f);
	  fgets (quiz_a, sizeof quiz_a, f);
	}
      if ((cptr = strchr (quiz_a, '\n')))
	*cptr = '\0';
      if ((cptr = strchr (quiz_a, '\r')))
	*cptr = '\0';
      puttext
	("PRIVMSG %s :Quiz question (%d seconds, answer is %zu chars): %s",
	 channel->channel, QUIZTIME, strlen (quiz_a), q);
      timer_register (TIME + QUIZTIME, quiz_stop_quiz, (void *) r);
    }
  fclose (f);
}

const struct module_info_s VISIBLE *quiz_info (void)
{
  return &quiz_info_s;
}

int quiz_load (void)
{
  command_register ("quiz", 1, 0, 0, 0, quiz_command_quiz);
  server_register_handler ("PRIVMSG", quiz_handle_privmsg);
  srandom ((int) (TIME ^ getpid ()));
  return 0;
}

int quiz_unload (void)
{
  command_unregister ("quiz");
  server_unregister_handler ("PRIVMSG", quiz_handle_privmsg);
  return 0;
}

const struct module_info_s quiz_info_s = {
  "quiz",
  1,
  0,
  quiz_load,
  quiz_unload
};
