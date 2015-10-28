#include "hbs.h"

const struct module_info_s test_info_s;

const struct module_info_s VISIBLE *test_info (void)
{
  return &test_info_s;
}

void test_command (NICK * nick, CHANNEL * channel, const char *cmd,
		   const char **args, int argc)
{
  puttext ("PRIVMSG %s :\001ACTION has invoked test_command()\001\r\n",
	   channel->channel);
}

int test_load (void)
{
  print ("Loaded test module - version %d.%d", test_info_s.major,
	 test_info_s.minor);
  command_register ("test", 1, 0, 0, 0, test_command);
  return 0;
}

int test_unload (void)
{
  print ("Unloaded test module - version %d.%d", test_info_s.major,
	 test_info_s.minor);
  command_unregister ("test");
  return 0;
}


const struct module_info_s test_info_s =
  { "test", 1, 0, test_load, test_unload };
