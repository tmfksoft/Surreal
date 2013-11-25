/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/channel.h
 *
 *  Copyright (C) 1990 Jarkko Oikarinen
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
 *  $Id: channel.h 985 2007-02-04 20:51:36Z shadowmaster $
 */


#ifndef	__channel_include__
# define __channel_include__
# define find_channel(chname, channel_p) (hash_find_channel(chname, channel_p))
# define CREATE 1		/* whether a channel should be created or just
				 * tested for existance */
# define MODEBUFLEN		200	/* max modebuf we consider from users */
# define REALMODEBUFLEN		512	/* max actual modebuf */
# define NullChn 		((aChannel *) NULL)
# define ChannelExists(n) 	(find_channel(n, NullChn) != NullChn)
#	include "msg.h"
#	define	MAXMODEPARAMS	(MAXPARA-4)
#	define MAXTSMODEPARAMS  (MAXPARA-5)
#define MAXMODEPARAMSUSER 6
#endif
