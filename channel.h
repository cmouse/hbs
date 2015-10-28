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
/**
 *
 * Channel specific defines
 *
 */

#ifndef CHANNEL_H
#define CHANNEL_H 1

/* Channel has Q bot */
#define CHANNEL_SERV_Q 0x1
/* Channel has L bot */
#define CHANNEL_SERV_L 0x2
/* Channel has lamer control */
#define CHANNEL_SERV_LAMER 0x4
/* Channel should enforce bans (not used) */
#define CHANNEL_SERV_ENFORCEBANS 0x8
/* Channel should use bitchmode (deop non-ops) */
#define CHANNEL_SERV_BITCHMODE 0x10
/* Channel ops should be protected (not used) */
#define CHANNEL_SERV_PROTECTOPS 0x20
/* People with +o should be automatically opped (auto-op on by default) */
#define CHANNEL_SERV_AUTOOP 0x40
/* People joining should get voice */
#define CHANNEL_SERV_AUTOVOICE 0x80
/* Channel's Q/L is not used */
#define CHANNEL_SERV_NOSERV 0x100
/* Channel's topic is forced */
#define CHANNEL_SERV_TOPIC 0x200
/* channel's topic is manual */
#define CHANNEL_SERV_DIRECT_TOPIC 0x400
/* channel has join flood protect */
#define CHANNEL_SERV_JOIN_FLOOD_PROTECT 0x800
/* channel disallows IP hosts */
#define CHANNEL_SERV_NO_IP 0x1000
/* Test if service active */
#define channel_has_service(CHANNEL,SERV) (CHANNEL->services & SERV)
/* Deactivate service */
#define channel_unset_service(CHANNEL,SERV) (CHANNEL->services &= (~SERV))
/* Activate service */
#define channel_set_service(CHANNEL,SERV) (CHANNEL->services |= SERV)

/* Weed stuff */
/* Remove after no ops for this many seconds */
#define MAX_AGE_NOOPS 86400*7
/* Remove after not on for this many seconds */
#define MAX_AGE_NOTON 86400*3
/* Remove after no authed join for this many seconds */
#define MAX_AGE_NOJOIN 86400*14

#endif
