/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/send.h
 *
 *  Copyright (C) 1992 Darren Reed
 *
 *  Copyright (C) 1990-2007 by the past and present ircd coders, and others.
 *  Refer to the documentation within doc/authors/ for full credits and copyrights.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: send.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

/*
 * "send.h". - Headers file.
 * 
 * all the send* functions are declared here.
 */

#ifndef SEND_H
#define SEND_H

#include "struct.h"

/* send all queued crap to aClient */
extern int send_queued (aClient *);

#include <stdarg.h>
#include "fdlist.h"

extern void init_send ();

extern void send_chatops (char *pattern, ...);
extern void send_globops (char *pattern, ...);
extern void send_operwall (aClient *, char *, char *);
extern void sendto_netinfo (char *pattern, ...);
extern void sendto_netglobal (char *pattern, ...);
extern void sendto_adminchat (char *pattern, ...);
extern void sendto_cschat (char *pattern, ...);

extern void sendto_connectnotice (char *pattern, ...);
extern void sendto_globalconnectnotice (char *pattern, ...);

extern void sendto_all_butone (aClient * one, aClient * from, char *pattern,
			       ...);
extern void sendto_channel_butone (aClient * one, aClient * from,
				   aChannel * channel_p, char *pattern, ...);
extern void sendto_channel_butserv (aChannel * channel_p, aClient * from,
				    char *pattern, ...);
extern void sendto_allchannelops_butone (aClient * one, aClient * from,
					 aChannel * channel_p, char *pattern,
					 ...);
extern void sendto_channeladmins_butone (aClient * one, aClient * from,
					 aChannel * channel_p, char *pattern,
					 ...);
extern void sendto_channelops_butone (aClient * one, aClient * from,
				      aChannel * channel_p, char *pattern, ...);
extern void sendto_channelhalfops_butone (aClient * one, aClient * from,
					  aChannel * channel_p, char *pattern,
					  ...);
extern void sendto_channelvoice_butone (aClient * one, aClient * from,
					aChannel * channel_p, char *pattern, ...);
extern void sendto_channelvoiceops_butone (aClient * one, aClient * from,
					   aChannel * channel_p, char *patern,
					   ...);
extern void sendto_common_channels (aClient * user, char *pattern, ...);
extern void send_quit_to_common_channels (aClient * from, char *reason);
extern void send_part_to_common_channels (aClient * from, char *reason);
extern void sendto_fdlist (fdlist * listp, char *pattern, ...);
extern void sendto_locops (char *pattern, ...);
extern void sendto_match_butone (aClient * one, aClient * from, char *mask,
				 int what, char *pattern, ...);
extern void sendto_match_servs (aChannel * channel_p, aClient * from,
				char *format, ...);
extern void sendto_one (aClient * to, char *pattern, ...);
extern void sendto_ops (char *pattern, ...);
extern void sendto_ops_butone (aClient * one, aClient * from, char *pattern,
			       ...);
extern void sendto_prefix_one (aClient * to, aClient * from, char *pattern,
			       ...);

extern void sendto_realops_lev (int lev, char *pattern, ...);
extern void sendto_realops (char *pattern, ...);
extern void sendto_serv_butone (aClient * one, char *pattern, ...);
extern void sendto_wallops_butone (aClient * one, aClient * from,
				   char *pattern, ...);
extern void sendto_gnotice (char *pattern, ...);

extern void ts_warn (char *pattern, ...);

extern void sendto_tsmode_servs(int tsmode, aChannel *channel_p, aClient *from,
                                 char *pattern, ...);

extern void sendto_ssjoin5_servs (aChannel * channel_p, aClient * from,
				 char *pattern, ...);
extern void sendto_pressjoin3_servs (aChannel * channel_p, aClient * from,
				     char *pattern, ...);
extern void sendto_oldssjoin_servs (aChannel * channel_p, aClient * from,
				    char *pattern, ...);
extern void sendto_noquit_servs_butone (int noquit, aClient * one,
					char *pattern, ...);
extern void sendto_clientcapab_servs_butone (int clientcapab, aClient * one,
					char *pattern, ...);


extern void vsendto_fdlist (fdlist * listp, char *pattern, va_list vl);
extern void vsendto_one (aClient * to, char *pattern, va_list vl);
extern void vsendto_prefix_one (aClient * to, aClient * from, char *pattern,
				va_list vl);
extern void vsendto_realops (char *pattern, va_list vl);

extern void flush_connections ();
extern void dump_connections ();
#endif
