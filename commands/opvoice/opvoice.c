#include "hbs.h"
#include "opvoice.h"

const struct module_info_s opvoice_info_s;

const struct module_info_s VISIBLE *opvoice_info (void)
{
  return &opvoice_info_s;
}

int opvoice_load (void)
{
  command_register ("op", 1, 0, USER_OP, 0, opvoice_command_op);
  command_register ("deop", 1, 0, USER_OP, 0, opvoice_command_deop);
  command_register ("voice", 1, 0, USER_OP | USER_VOICE, 0,
		    opvoice_command_voice);
  command_register ("devoice", 1, 0, USER_OP | USER_VOICE, 0,
		    opvoice_command_devoice);
  return 0;
}

int opvoice_unload (void)
{
  command_unregister ("op");
  command_unregister ("deop");
  command_unregister ("voice");
  command_unregister ("devoice");
  return 0;
}

const struct module_info_s opvoice_info_s = {
  "opvoice",
  1,
  0,
  opvoice_load,
  opvoice_unload
};
