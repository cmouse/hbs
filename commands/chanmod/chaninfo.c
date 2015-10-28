#include "chanmod.h"

void chanmod_command_chaninfo (NICK * nick, CHANNEL * channel,
			       const char *cmd, const char **args, int argc)
{
  char services[300];
  char *keep, *drop;
  keep = drop = NULL;
  if (channel->modes_keep)
    keep = mode2modestr (channel->modes_keep);

  if (channel->modes_drop)
    drop = mode2modestr (channel->modes_drop);

  puttext ("NOTICE %s :Information on %s\r\n", nick->nick, channel->channel);
  puttext ("NOTICE %s : Modes: %c%s%c%s\r\n", nick->nick, (keep ? '+' : ' '),
	   (keep ? keep : ""), (drop ? '-' : ' '), (drop ? drop : ""));
  if (keep)
    free (keep);
  if (drop)
    free (drop);
  services[0] = '\0';
  strcat (services, " Services: bitchmode ");
  if (channel_has_service (channel, CHANNEL_SERV_BITCHMODE))
    strcat (services, "on");
  else
    strcat (services, "off");
  strcat (services, ", autovoice ");
  if (channel_has_service (channel, CHANNEL_SERV_AUTOVOICE))
    strcat (services, "on");
  else
    strcat (services, "off");
  strcat (services, ", lamercontrol ");
  if (channel_has_service (channel, CHANNEL_SERV_LAMER))
    strcat (services, "on");
  else
    strcat (services, "off");
  strcat (services, ", enforcebans ");
  if (channel_has_service (channel, CHANNEL_SERV_ENFORCEBANS))
    strcat (services, "on");
  else
    strcat (services, "off");
  strcat (services, ", topic forcing ");
  if (channel_has_service (channel, CHANNEL_SERV_TOPIC))
    strcat (services, "on");
  else
    strcat (services, "off");
  puttext ("NOTICE %s :%s\r\n", nick->nick, services);
  services[0] = '\0';
  strcat (services, "          join flood protect "); 
  if (channel_has_service (channel, CHANNEL_SERV_JOIN_FLOOD_PROTECT))
    strcat (services, "on");
  else
    strcat (services, "off");
  strcat (services, ", disallow IP hosts ");
  if (channel_has_service (channel, CHANNEL_SERV_NO_IP))
    strcat (services, "on");
  else
    strcat (services, "off");
  puttext ("NOTICE %s :%s\r\n", nick->nick, services);
}
