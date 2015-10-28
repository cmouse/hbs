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

// should the bot run or die (if this turns to 0, the bot quits)
int main_runcond = 1;
// how many times we are signalled.
int main_sigged = 0;
// what is our log file called
FILE *main_logfile = 0;
// do we have tty to log to
int main_tty = 1;
// tcp protocol number
int tcp_protocol_number;
// main args
char *const *main_argv;

/**
 * Screen logger. Prints log messages to screen.
 * @param time - Time string
 * @param fmt - format string
 * @param va - format arguments;
 */
void screenlogger (const char *time, const char *fmt, va_list va)
{
  if (!main_tty)
    return;
  fprintf (stdout, "[%s] ", time);
  vfprintf (stdout, fmt, va);
  fprintf (stdout, "\n");
}

/**
 * File logger. Prints log messages to a file.
 * @param time - Time string
 * @param fmt - format string
 * @param va - format arguments;
 */
void filelogger (const char *time, const char *fmt, va_list va)
{
  if (main_logfile != NULL)
    {
      fprintf (main_logfile, "[%s] ", time);
      vfprintf (main_logfile, fmt, va);
      fprintf (main_logfile, "\n");
      fflush (main_logfile);
    }
}

/**
 * Signals are sent here such as sigint et al.
 * @param sig - Signal number
 * @return possibly 0
 */
RETSIGTYPE main_siggood (int sig)
{
  /* Tell bot to shut down */
  main_runcond = 0;
  if (main_sigged > 3)
    abort ();
  else if (main_sigged > 2)
    print ("Signal once more and I call abort...");
  main_sigged++;
#if RETSIGTYPE != void
  return 0;
#endif
}

/**
 * Sets up signal handlers and which signals to ignore.
 */
void main_signals (void)
{
  signal (SIGHUP, SIG_IGN);
  signal (SIGINT, main_siggood);
  signal (SIGTERM, main_siggood);
  signal (SIGQUIT, main_siggood);
  signal (SIGSTOP, SIG_IGN);
  signal (SIGCONT, SIG_IGN);
  signal (SIGPIPE, SIG_IGN);
}

/**
 * Tells the bot to die.
 * @param console - Client
 * @param cmd - command
 * @param args - argument string
 * @param argc - argument count
 */
void
main_concommand_quit (CONSOLE * console, const char *cmd,
		      const char **args, int argc)
{
  print ("Bot shutdown authorized by %s from console", console->user->user);
  main_runcond = 0;
}

void
main_command_quit (NICK * nick, CHANNEL * channel, const char *cmd,
		   const char **args, int argc)
{
  print ("Bot shutdown authorized by %s[%s]", nick->nick, nick->user->user);
  main_runcond = 0;
}

/**
 * Sets up global variable tcp_protocol_number
 */
void main_set_tcp_protocol_number ()
{
  struct protoent *ent;
  if ((ent = getprotobyname ("tcp")))
    {
      tcp_protocol_number = ent->p_proto;
      endprotoent ();
    }
  else
    {
      tcp_protocol_number = 6;
    }
}

int
main_parse_args (int argc, char *const argv[], char **configfile,
		 int *foreground)
{
  signed char opt;
  *configfile = strdup ("./hbs.conf");
  *foreground = 0;
  while ((opt = getopt (argc, argv, "fc:")) != -1)
    {
      switch (opt)
	{
	case 'f':
	  *foreground = 1;
	  break;
	case 'c':
	  free (*configfile);
	  *configfile = strdup (optarg);
	  break;
	default:
	  printf ("Errorneus command line option '%c'\r\n", optopt);
	  return 1;
	}
    }
  return 0;
}

/**
 * Main function of the bot.
 */
int main (int argc, char *const argv[], char *envp[])
{
  pid_t pid;
  const char *modfile;
  const char *logfile;
  char *configfile;
  int foreground;
  // test XML version. This is done because libxml wants this.
  LIBXML_TEST_VERSION;
  main_argv = argv;
  log_register (screenlogger);
  log_register (filelogger);
  // setup signals
  main_signals ();
  // parse cmdline
  if (main_parse_args (argc, argv, &configfile, &foreground))
    {
      return 1;
    }
  // read config file
  if (config_read (configfile))
    return 1;
  // setup logfile
  logfile = config_getval ("global", "logfile");
  if (logfile)
    main_logfile = fopen (logfile, "a");
  // setup servers
  server_setup ();
  // setup modules
  if (module_setup ())
    {
      print ("Error during setup");
      return 1;
    }
  // setup IRC
  if (irc_setup ())
    {
      print ("Error during setup");
      return 1;
    }
  // setup console system
  if (console_setup ())
    {
      print ("Error during setup");
      return 1;
    }
  // load all modules
  if ((modfile = config_getval ("global", "modules")))
    if (modules_load (modfile))
      {
	print ("Error during setup");
	return 1;
      }
  // setup botnet
  botnet_init ();
  // register two commands
  command_register ("quit", 0, 0, 0, 1, main_command_quit);
  console_register_command ("die", 0, main_concommand_quit,
			    "Instructs the bot to terminate");
  // Go to background
  if (!foreground)
    {
      switch ((pid = fork ()))
	{
	case -1:
	  perror ("fork()");
	  exit (1);
	case 0:
	  fclose (stdin);
	  fclose (stdout);
	  fclose (stderr);
	  setsid ();
	  main_tty = 0;
	  break;
	default:
	  {
	    const char *pidfilec;
	    FILE *pidfile;
	    printf ("Gone to lurk in the background under pid %u\n", pid);
	    if ((pidfilec = config_getval ("global", "pidfile")))
	      {
		if ((pidfile = fopen (pidfilec, "w")))
		  {
		    fprintf (pidfile, "%u\n", pid);
		    fclose (pidfile);
		  }
		else
		  {
		    perror ("fopen(pidfile)");
		  }
	      }
	  }
	  return 0;
	}
    }
  // start main loop
  while (main_runcond)
    {
      struct timespec sleep_time = {0,10000000};
      sigset_t sig_tmp;
      // check that we are connected
      server_check ();
      // poll servers
      server_poll ();
      // poll consoles
      console_poll ();
      // run all timers
      timers_run ();
      // check extra polls
      xpoll ();
      // check for module ops
      module_run_ops ();
      // wait a little while. reduces load.
      sigfillset(&sig_tmp);
      pselect(0, NULL, NULL, NULL, &sleep_time, &sig_tmp);
    }
  // shutdown bot.
  print ("Bot shutdown sequence started");
  // To make sure shutdown gets into people's consoles
  console_poll ();
  // disconnect server
  server_disconnect ();
  // unload modules
  modules_unload ();
  // cleanup console
  console_cleanup ();
  // store user/channel files
  irc_store ();
  // remove config
  config_free ();
  print ("Bot shutdown complete");
  // close logfile
  if (main_logfile)
    fclose (main_logfile);
  main_logfile = NULL;
  return 0;
}

char *const *main_get_argv ()
{
  return main_argv;
}

int main_get_runcond ()
{
  return main_runcond;
}
