#include "chanmod.h"

void chanmod_command_setchan (NICK * nick, CHANNEL * channel, const char *cmd,
			      const char **args, int argc)
{
  if (argc < 2)
    {
      puttext ("NOTICE %s :Usage: %s service on/off/param\r\n", nick->nick,
	       cmd);
      return;
    }
  if ((nick->user->admin == 1) && (!strcasecmp (args[0], "chanserv")))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel_unset_service (channel, CHANNEL_SERV_NOSERV);
	  puttext ("NOTICE %s :Q/L support enabled\r\n", nick->nick);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel_set_service (channel, CHANNEL_SERV_NOSERV);
	  puttext ("NOTICE %s :Q/L support disabled\r\n", nick->nick);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if ((nick->user->admin == 1) && (!strcasecmp (args[0], "permanent")))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel->permanent = 1;
	  puttext ("NOTICE %s :Channel made permanent\r\n", nick->nick);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel->permanent = 0;
	  puttext ("NOTICE %s :Channel made dynamic\r\n", nick->nick);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if ((nick->user->admin == 1) && (!strcasecmp (args[0], "disabled")))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel->disabled = 1;
	  putserv ("PART %s :Channel disabled\r\n", channel->channel);
	  puttext ("NOTICE %s :Channel disabled\r\n", nick->nick);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel->disabled = 0;
	  if (channel->key)
	    putserv ("JOIN %s %s\r\n", channel->channel, channel->key);
	  else
	    putserv ("JOIN %s\r\n", channel->channel);
	  puttext ("NOTICE %s :Channel enabled\r\n", nick->nick);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}

    }
  else if (!strcasecmp (args[0], "bitchmode"))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel_set_service (channel, CHANNEL_SERV_BITCHMODE);
	  puttext ("NOTICE %s :Bitchmode turned on for %s\r\n", nick->nick,
		   channel->channel);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel_unset_service (channel, CHANNEL_SERV_BITCHMODE);
	  puttext ("NOTICE %s :Bitchmode turned off for %s\r\n", nick->nick,
		   channel->channel);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if (!strcasecmp (args[0], "lamercontrol"))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel_set_service (channel, CHANNEL_SERV_LAMER);
	  puttext ("NOTICE %s :Lamercontrol turned on for %s\r\n", nick->nick,
		   channel->channel);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel_unset_service (channel, CHANNEL_SERV_LAMER);
	  puttext ("NOTICE %s :Lamercontrol turned off for %s\r\n",
		   nick->nick, channel->channel);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if (!strcasecmp (args[0], "forcetopic"))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel_set_service (channel, CHANNEL_SERV_TOPIC);
	  puttext ("NOTICE %s :Topic forcing turned on for %s\r\n",
		   nick->nick, channel->channel);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel_unset_service (channel, CHANNEL_SERV_TOPIC);
	  puttext ("NOTICE %s :Topic forcing turned off for %s\r\n",
		   nick->nick, channel->channel);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if (!strcasecmp (args[0], "enforcebans"))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel_set_service (channel, CHANNEL_SERV_ENFORCEBANS);
	  puttext ("NOTICE %s :Ban enforcing turned on for %s\r\n",
		   nick->nick, channel->channel);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel_unset_service (channel, CHANNEL_SERV_ENFORCEBANS);
	  puttext ("NOTICE %s :Ban enforcing turned off for %s\r\n",
		   nick->nick, channel->channel);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if (!strcasecmp (args[0], "autovoice"))
    {
      if (!strcasecmp (args[1], "on"))
	{
	  channel_set_service (channel, CHANNEL_SERV_AUTOVOICE);
	  puttext ("NOTICE %s :Autovoice turned on for %s\r\n", nick->nick,
		   channel->channel);
	}
      else if (!strcasecmp (args[1], "off"))
	{
	  channel_unset_service (channel, CHANNEL_SERV_AUTOVOICE);
	  puttext ("NOTICE %s :Autovoice turned off for %s\r\n", nick->nick,
		   channel->channel);
	}
      else
	{
	  puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
		   nick->nick, args[1]);
	}
    }
  else if (!strcasecmp (args[0], "joinflood"))
    {
      if (!strcasecmp (args[1], "on"))
        {
          channel_set_service (channel, CHANNEL_SERV_JOIN_FLOOD_PROTECT);
          puttext ("NOTICE %s :Join flood protect turned on for %s\r\n", nick->nick,
                   channel->channel);
        }
      else if (!strcasecmp (args[1], "off"))
        {
          channel_unset_service (channel, CHANNEL_SERV_JOIN_FLOOD_PROTECT);
          puttext ("NOTICE %s :Join flood protect turned off for %s\r\n", nick->nick,
                   channel->channel);
        }
      else
        {
          puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
                   nick->nick, args[1]);
        }
    }
  else if (!strcasecmp (args[0], "noiphosts"))
    {
      if (!strcasecmp (args[1], "on"))
        {
          channel_set_service (channel, CHANNEL_SERV_NO_IP);
          puttext ("NOTICE %s :Disallowing IP hosts on %s\r\n", nick->nick,
                   channel->channel);
        }
      else if (!strcasecmp (args[1], "off"))
        {
          channel_unset_service (channel, CHANNEL_SERV_NO_IP);
          puttext ("NOTICE %s :Permitting IP hosts on %s\r\n", nick->nick,
                   channel->channel);
        }
      else
        {
          puttext ("NOTICE %s :Invalid parameter '%s': Must be on or off\r\n",
                   nick->nick, args[1]);
        }
    }
  else if (!strcasecmp (args[0], "modes"))
    {
      unsigned long keep;
      unsigned long drop;
      unsigned long mode;
      int i;
      int dir;
      const char *cp;
      dir = 1;
      i = 1;
      keep = drop = 0;
      for (cp = args[1]; *cp; cp++)
	{
	  if ((*cp == '+'))
	    dir = 1;
	  else if ((*cp == '-'))
	    dir = 0;
	  else if ((*cp == 'd'))
	    continue;		// prevent impossible modes
	  else if ((*cp == 'o'))
	    continue;
	  else if ((*cp == 'v'))
	    continue;
	  else if ((*cp == 'b'))
	    continue;
	  if ((mode = modechar2mode (*cp)) == 0)
	    continue;
	  if (dir && (mode == MODE_l))
	    {
	      i++;
	      if (i < argc)
		{
		  if ((channel->limit = s_atoi (args[i])) == 0)
		    continue;
		}
	      else
		{
		  continue;
		}
	    }
	  if (dir && (mode == MODE_k))
	    {
	      i++;
	      if (i < argc)
		{
		  if (channel->key != NULL)
		    free (channel->key);
		  channel->key = strdup (args[i]);
		  putmode (channel, "+k", channel->key);
		}
	      else
		{
		  continue;
		}
	    }
	  if (dir)
	    keep |= mode;
	  else
	    drop |= mode;
	}
      if (keep & drop)
	drop = drop & ~(keep);
      // remove cycles
      if ((keep & MODE_s) && (keep & MODE_p))
	keep = ~(MODE_p);
      channel->modes_keep = keep;
      channel->modes_drop = drop;
      irc_fix_modes (channel);
      puttext ("NOTICE %s :Changed channel modes\r\n", nick->nick);
    }
  else
    {
      if (nick->user->admin)
	{
	  puttext
	    ("NOTICE %s :Invalid service '%s': Must be disabled, chanserv, bitchmode, lamercontrol, autovoice, forcetopic, enforcebans or modes\r\n",
	     nick->nick, args[0]);
	}
      else
	{
	  puttext
	    ("NOTICE %s :Invalid service '%s': Must be bitchmode, lamercontrol, autovoice, forcetopic, enforcebans or modes\r\n",
	     nick->nick, args[0]);
	}
    }
}
