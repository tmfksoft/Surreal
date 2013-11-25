/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/supported.h
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
 *  $Id: supported.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#ifndef INCLUDED_supported_h
#define INCLUDED_supported_h

#include "config.h"
#include "channel.h"
#include "dynconf.h"

#define CASEMAP "ascii"
#define STD "i-d"

/*
     CHANMODES[=A,B,C,D]

     The CHANMODES specifies the modes that may be set on a channel.
     These modes are split into four categories, as follows:

     Type A: Modes that add or remove an address to or from a list.
          These modes always take a parameter when sent by the server to
          a client; when sent by a client, they may be specified without
          a parameter, which requests the server to display the current
          contents of the correspending list on the channel to the
          client.

     Type B: Modes that change a setting on the channel.  These modes
          always take a parameter.

     Type C: Modes that change a setting on the channel.  These modes
          take a parameter only when set; the parameter is absent when
          the mode is removed.

     Type D: Modes that change a setting on the channel.  These modes
          never take a parameter.

     If the server sends any addition types after these 4, the client
     should ignore them; this is intended to allow future extension of
     this token.

     The IRC server should not list modes in CHANMODES which are also
     present in the PREFIX parameter; however, for completeness, modes
     described in PREFIX may be treated as type B modes.

     If the server sends a mode which is missing from both CHANMODES and
     PREFIX, the client should treat it as a type D mode. However, this
     is a protocol violation by the server.


     Example: CHANMODES=b,k,l,imnpst

     The CHANMODES token requires a non-null value specifier; if no
     value is given, or it is null, the client should ignore the entire
     token.  There is no default value for the CHANMODES token.
*/

#define CHANMODES "be,k,l,cimnpqstAKMNORS"

#define FEATURES "SAFELIST"\
                " SILENCE" \
                " KNOCK" \
                " FNC" \
                " WATCH=%i" \
                " CHANLIMIT=#&:%i" \
                " MAXLIST=be:%i" \
                " NICKLEN=%i" \
                " TOPICLEN=%i" \
                " KICKLEN=%i" \
                " CHANNELLEN=%i"

#define FEATURESVALUES MAXWATCH,MAXCHANNELSPERUSER,MAXBANS,NICKLEN, \
        TOPICLEN,TOPICLEN,CHANNELLEN

#define FEATURES2 "EXCEPTS=%s" \
                  " CHANTYPES=%s" \
                  " PREFIX=%s" \
                  " CHANMODES=%s" \
                  " STATUSMSG=%s" \
                  " NETWORK=%s" \
                  " CASEMAPPING=%s" \
                  " STD=%s"

#define FEATURES2VALUES "e", \
                        "#&", \
                        "(aohv)!@%+", \
                        CHANMODES, \
                        "!@%+", \
                        IRCNETWORK, \
                        CASEMAP, \
                        STD




#endif /* INCLUDED_supported_h */

