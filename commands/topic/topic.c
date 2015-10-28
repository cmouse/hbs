#include "topic.h"

const struct module_info_s topic_info_s;

const struct module_info_s VISIBLE *topic_info (void)
{
  return &topic_info_s;
}

int topic_load (void)
{
  command_register ("settopic", 1, 1, USER_MASTER, 0, topic_command_settopic);
  command_register ("reftopic", 1, 0, USER_OP | USER_VOICE, 0,
		    topic_command_reftopic);
  return 0;
}

int topic_unload (void)
{
  command_unregister ("settopic");
  command_unregister ("reftopic");
  return 0;
}

const struct module_info_s topic_info_s = {
  "topic",
  1,
  0,
  topic_load,
  topic_unload
};
