/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_help.c
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
 *  $Id: s_err.c,v 1.8 2002/02/08 09:06:39 shadowmaster Exp $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "dynconf.h"
#include "numeric.h"
#include "msg.h"

#define SND(string)	sendto_one(source_p, ":%s 291 %s :%s", me.name, source_p->name, string)
#define HLP(string)	sendto_one(source_p, ":%s 293 %s :%s", me.name, source_p->name, string)
#define SND_FOOTER sendto_one(source_p, ":%s 292 %s :If you require any further assistance, please join %s", me.name, source_p->name, HELPCHAN)

static int help_ircd_main (aClient *);
static int help_ircd_usercmds (aClient *);
static int help_ircd_opercmds (aClient *);
static int help_ircd_usermodes (aClient *);
static int help_ircd_nmodelist (aClient *);
static int help_ircd_chanmodes (aClient *);
static int help_ircd_oflags (aClient *);
static int help_ircd_nmode (aClient *);
static int help_ircd_admin (aClient *);
static int help_ircd_away (aClient *);
static int help_ircd_nick (aClient *);
static int help_ircd_ircops (aClient *);
static int help_ircd_whois (aClient *);
static int help_ircd_who (aClient *);
static int help_ircd_rules (aClient *);
static int help_ircd_identify (aClient *);
static int help_ircd_ison (aClient *);
static int help_ircd_join (aClient *);
static int help_ircd_part (aClient *);
static int help_ircd_names (aClient *);
static int help_ircd_userhost (aClient *);
static int help_ircd_lusers (aClient *);
static int help_ircd_map (aClient *);
static int help_ircd_quit (aClient *);
static int help_ircd_stats (aClient *);
static int help_ircd_links (aClient *);
static int help_ircd_time (aClient *);
static int help_ircd_invite (aClient *);
static int help_ircd_kick (aClient *);
static int help_ircd_watch (aClient *);
static int help_ircd_list (aClient *);
static int help_ircd_privnot (aClient *);
static int help_ircd_vhost (aClient *);
static int help_ircd_topic (aClient *);
static int help_ircd_mode (aClient *);
static int help_ircd_info (aClient *);
static int help_ircd_version (aClient *);
static int help_ircd_motd (aClient *);
static int help_ircd_whowas (aClient *);
static int help_ircd_mkpasswd (aClient *);
static int help_ircd_ping (aClient *);
static int help_ircd_pong (aClient *);
static int help_ircd_sethost (aClient *);
static int help_ircd_fjoin (aClient *);
static int help_ircd_netglobal (aClient *);
static int help_ircd_fmode (aClient *);
static int help_ircd_oper (aClient *);
static int help_ircd_rehash (aClient *);
static int help_ircd_wallops (aClient *);
static int help_ircd_restart (aClient *);
static int help_ircd_adchat (aClient *);
static int help_ircd_die (aClient *);
static int help_ircd_kill (aClient *);
static int help_ircd_chanfix (aClient *);
static int help_ircd_connect (aClient *);
static int help_ircd_globops (aClient *);
static int help_ircd_kline (aClient *);
static int help_ircd_locops (aClient *);
static int help_ircd_unkline (aClient *);
static int help_ircd_trace (aClient *);
static int help_ircd_about (aClient *);
static int help_ircd_squit (aClient *);
static int help_ircd_channel (aClient *);
static int help_ircd_credits (aClient *);
static int help_ircd_copyright (aClient *);
static int help_ircd_dccallow (aClient *);
static int help_ircd_help (aClient *);
static int help_ircd_settings (aClient *);
static int help_ircd_knock (aClient *);
static int help_ircd_silence (aClient *);
static int help_ircd_users (aClient *);

int
m_help (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  char *help;


  if (!MyClient (source_p))
  {
    return 0;
  }

  if (parc < 2)
  {
    help_ircd_main (source_p);
    HLP (" ");
    sendto_one (source_p,
                ":%s 292 %s :If you require any further assistance, please join %s.",
                me.name, parv[0], HELPCHAN);
    return 0;
  }

  help = parv[1];

  if (ircncmp (help, "USERCMDS", 9) == 0)
  {
    help_ircd_usercmds (source_p);
  }
  else if (ircncmp (help, "OPERCMDS", 9) == 0)
  {
    help_ircd_opercmds (source_p);
  }
  else if (ircncmp (help, "USERMODES", 9) == 0)
  {
    help_ircd_usermodes (source_p);
  }
  else if (ircncmp (help, "NMODES", 9) == 0)
  {
    help_ircd_nmodelist (source_p);
  }
  else if (ircncmp (help, "NMODE", 9) == 0)
  {
    help_ircd_nmode (source_p);
  }
  else if (ircncmp (help, "CHANMODES", 9) == 0)
  {
    help_ircd_chanmodes (source_p);
  }
  else if (ircncmp (help, "OFLAGS", 9) == 0)
  {
    help_ircd_oflags (source_p);
  }
  else if (ircncmp (help, "ADMIN", 9) == 0)
  {
    help_ircd_admin (source_p);
  }
  else if (ircncmp (help, "AWAY", 9) == 0)
  {
    help_ircd_away (source_p);
  }
  else if (ircncmp (help, "NICK", 9) == 0)
  {
    help_ircd_nick (source_p);
  }
  else if (ircncmp (help, "IRCOPS", 9) == 0)
  {
    help_ircd_ircops (source_p);
  }
  else if (ircncmp (help, "WHOIS", 9) == 0)
  {
    help_ircd_whois (source_p);
  }
  else if (ircncmp (help, "WHO", 9) == 0)
  {
    help_ircd_who (source_p);
  }
  else if (ircncmp (help, "RULES", 9) == 0)
  {
    help_ircd_rules (source_p);
  }
  else if (ircncmp (help, "IDENTIFY", 9) == 0)
  {
    help_ircd_identify (source_p);
  }
  else if (ircncmp (help, "ISON", 9) == 0)
  {
    help_ircd_ison (source_p);
  }
  else if (ircncmp (help, "JOIN", 9) == 0)
  {
    help_ircd_join (source_p);
  }
  else if (ircncmp (help, "PART", 9) == 0)
  {
    help_ircd_part (source_p);
  }
  else if (ircncmp (help, "NAMES", 9) == 0)
  {
    help_ircd_names (source_p);
  }
  else if (ircncmp (help, "USERHOST", 9) == 0)
  {
    help_ircd_userhost (source_p);
  }
  else if (ircncmp (help, "LUSERS", 9) == 0)
  {
    help_ircd_lusers (source_p);
  }
  else if (ircncmp (help, "MAP", 9) == 0)
  {
    help_ircd_map (source_p);
  }
  else if (ircncmp (help, "QUIT", 9) == 0)
  {
    help_ircd_quit (source_p);
  }
  else if (ircncmp (help, "STATS", 9) == 0)
  {
    help_ircd_stats (source_p);
  }
  else if (ircncmp (help, "LINKS", 9) == 0)
  {
    help_ircd_links (source_p);
  }
  else if (ircncmp (help, "TIME", 9) == 0)
  {
    help_ircd_time (source_p);
  }
  else if (ircncmp (help, "INVITE", 9) == 0)
  {
    help_ircd_invite (source_p);
  }
  else if (ircncmp (help, "KICK", 9) == 0)
  {
    help_ircd_kick (source_p);
  }
  else if (ircncmp (help, "WATCH", 9) == 0)
  {
    help_ircd_watch (source_p);
  }
  else if (ircncmp (help, "LIST", 9) == 0)
  {
    help_ircd_list (source_p);
  }
  else if (ircncmp (help, "PRIVMSG", 9) == 0)
  {
    help_ircd_privnot (source_p);
  }
  else if (ircncmp (help, "NOTICE", 9) == 0)
  {
    help_ircd_privnot (source_p);
  }
  else if (ircncmp (help, "VHOST", 9) == 0)
  {
    help_ircd_vhost (source_p);
  }
  else if (ircncmp (help, "TOPIC", 9) == 0)
  {
    help_ircd_topic (source_p);
  }
  else if (ircncmp (help, "MODE", 9) == 0)
  {
    help_ircd_mode (source_p);
  }
  else if (ircncmp (help, "INFO", 9) == 0)
  {
    help_ircd_info (source_p);
  }
  else if (ircncmp (help, "VERSION", 9) == 0)
  {
    help_ircd_version (source_p);
  }
  else if (ircncmp (help, "MOTD", 9) == 0)
  {
    help_ircd_motd (source_p);
  }
  else if (ircncmp (help, "WHOWAS", 9) == 0)
  {
    help_ircd_whowas (source_p);
  }
  else if (ircncmp (help, "MKPASSWD", 9) == 0)
  {
    help_ircd_mkpasswd (source_p);
  }
  else if (ircncmp (help, "MAKEPASS", 9) == 0)
  {
    help_ircd_mkpasswd (source_p);
  }
  else if (ircncmp (help, "PING", 9) == 0)
  {
    help_ircd_ping (source_p);
  }
  else if (ircncmp (help, "PONG", 9) == 0)
  {
    help_ircd_pong (source_p);
  }
  else if (ircncmp (help, "SETHOST", 9) == 0)
  {
    help_ircd_sethost (source_p);
  }
  else if (ircncmp (help, "FJOIN", 9) == 0)
  {
    help_ircd_fjoin (source_p);
  }
  else if (ircncmp (help, "NETGLOBAL", 9) == 0)
  {
    help_ircd_netglobal (source_p);
  }
  else if (ircncmp (help, "FMODE", 9) == 0)
  {
    help_ircd_fmode (source_p);
  }
  else if (ircncmp (help, "OPER", 9) == 0)
  {
    help_ircd_oper (source_p);
  }
  else if (ircncmp (help, "REHASH", 9) == 0)
  {
    help_ircd_rehash (source_p);
  }
  else if (ircncmp (help, "WALLOPS", 9) == 0)
  {
    help_ircd_wallops (source_p);
  }
  else if (ircncmp (help, "RESTART", 9) == 0)
  {
    help_ircd_restart (source_p);
  }
  else if (ircncmp (help, "ADCHAT", 9) == 0)
  {
    help_ircd_adchat (source_p);
  }
  else if (ircncmp (help, "DIE", 9) == 0)
  {
    help_ircd_die (source_p);
  }
  else if (ircncmp (help, "KILL", 9) == 0)
  {
    help_ircd_kill (source_p);
  }
  else if (ircncmp (help, "SQUIT", 9) == 0)
  {
    help_ircd_squit (source_p);
  }
  else if (ircncmp (help, "CHANFIX", 9) == 0)
  {
    help_ircd_chanfix (source_p);
  }
  else if (ircncmp (help, "CONNECT", 9) == 0)
  {
    help_ircd_connect (source_p);
  }
  else if (ircncmp (help, "GLOBOPS", 9) == 0)
  {
    help_ircd_globops (source_p);
  }
  else if (ircncmp (help, "KLINE", 9) == 0)
  {
    help_ircd_kline (source_p);
  }
  else if (ircncmp (help, "LOCOPS", 9) == 0)
  {
    help_ircd_locops (source_p);
  }
  else if (ircncmp (help, "UNKLINE", 9) == 0)
  {
    help_ircd_unkline (source_p);
  }
  else if (ircncmp (help, "TRACE", 9) == 0)
  {
    help_ircd_trace (source_p);
  }
  else if (ircncmp (help, "ABOUT", 9) == 0)
  {
    help_ircd_about (source_p);
  }
  else if (ircncmp (help, "CHANNEL", 9) == 0)
  {
    help_ircd_channel (source_p);
  }
  else if (ircncmp (help, "CREDITS", 9) == 0)
  {
    help_ircd_credits (source_p);
  }
  else if (ircncmp (help, "COPYRIGHT", 9) == 0)
  {
    help_ircd_copyright (source_p);
  }
  else if (ircncmp (help, "DCCALLOW", 9) == 0)
  {
    help_ircd_dccallow (source_p);
  }
  else if (ircncmp (help, "HELP", 9) == 0)
  {
    help_ircd_help (source_p);
  }
  else if (ircncmp (help, "IRCDHELP", 9) == 0)
  {
    help_ircd_help (source_p);
  }
  else if (ircncmp (help, "SETTINGS", 9) == 0)
  {
    help_ircd_settings (source_p);
  }
  else if (ircncmp (help, "KNOCK", 9) == 0)
  {
    help_ircd_knock (source_p);
  }
  else if (ircncmp (help, "SILENCE", 9) == 0)
  {
    help_ircd_silence (source_p);
  }
  else if (ircncmp (help, "USERS", 9) == 0)
  {
    help_ircd_users (source_p);
  }
  else
  {
    sendto_one (source_p, ":%s 293 %s :No help availible for %s", me.name,
                source_p->name, help);
  }

  HLP (" ");
  sendto_one (source_p,
              ":%s 292 %s :If you require any further assistance, please join %s.",
              me.name, parv[0], HELPCHAN);
  return 1;
}

int
help_ircd_main (aClient * source_p)
{
  SND ("Please specify a help topic from below after /IRCDHELP");
  SND ("Syntax: /IRCDHELP <topic>");
  SND (" ");
  SND (" ");
  SND ("USERCMDS      - A list of commands available to users");
  SND ("USERMODES     - Modes available to users");
  SND ("NMODES        - Noticemodes available to users");
  SND ("CHANMODES     - Modes available which can be used on channels");
  if (!IsAnOper (source_p))
  {
    return 0;
  }
  SND ("OPERCMDS      - A list of commands which are oper specific");
  SND ("OFLAGS        - A list of valid O:Flags");
  return 0;
}

int
help_ircd_usercmds (aClient * source_p)
{
  SND
    ("Below is a list of User Commands currently available. Type \'/IRCDHELP <command>\'");
  SND ("for more information on the command.");
  SND (" ");
  SND ("ADMIN    AWAY     CHANNEL  CREDITS  COPYRIGHT");
  SND ("DCCALLOW HELP     INFO     INVITE   IRCDHELP");
  SND ("IRCOPS   ISON     JOIN     KICK     KNOCK");
  SND ("LINKS    LIST     LUSERS   MAKEPASS MAP");
  SND ("MODE     MOTD     NICK     NMODE    NOTICE");
  SND ("PART     PING     PONG     RULES    SETTINGS");
  SND ("SILENCE  STATS    TIME     TRACE    TOPIC");
  SND ("USERHOST USERS    VERSION  WATCH    WHO");
  SND ("WHOIS    QUIT");
  return 0;
}

int
help_ircd_opercmds (aClient * source_p)
{
  if (!IsAnOper (source_p))
  {
    return 0;
  }
  SND ("Below is a list of commands availble to IRC operators (opers)");
  SND ("Syntax: /IRCDHELP <topic>");
  SND (" ");
  SND (" ");
  SND ("ADCHAT     - Sends a message to all admins online");
  SND ("CHANFIX    - Used to resync a channels timestamp");
  SND ("CONNECT    - Connect a given server");
  SND ("DIE        - Shut down the IRC server");
  SND ("FJOIN      - Joins a channel by force, bypasses bans, etc");
  SND ("FMODE      - Forces a mode upon a channel");
  SND ("GLOBOPS    - Sends a message to all online perators");
  SND ("KILL       - Causes a server to terminate a user connection");
  SND ("KLINE      - Adds a K:line");
  SND ("LOCOPS     - Send a message to all online local operators");
  SND ("NETGLOBAL  - Sends global IRCop messages");
  SND ("OPER       - Used to 'Oper Up'");
  SND ("REHASH     - Force the IRCd to reload the config file");
  SND ("RESTART    - Restarts the IRC server");
  SND ("SETHOST    - Change the hostmask of a given user");
  SND ("SQUIT      - Force a server to de-link");
  SND ("TRACE      - Trace a server or user");
  SND ("UNKLINE    - Removes a K:line");
  SND ("WALLOPs    - Sends a wallop");
  return 0;
}

int
help_ircd_usermodes (aClient * source_p)
{
  SND
    ("Below is a list of the User Modes currently available, along with their function.");
  SND (" ");
  SND ("Modes availible to all users:");
  SND (" ");
  SND ("d - is deaf (does not receive channel messages)");
  SND ("D - Has seen /DCCALLOW warning message");
  SND ("i - Invisible (Hidden in /WHO)");
  SND ("k - Can see server originating kill notices");
  SND ("R - Only receives private messages from registered users");
  SND ("s - Can see server notices");
  SND ("w - Can see /WALLOPS notices");
  SND (" ");
  SND ("Modes which are server controlled:");
  SND (" ");
  SND ("r - Registered nickname");
  SND ("S - Services client");
  SND ("x - Real host is hidden");
  SND (" ");
  SND
    ("Modes restricted to operators, most additionally restricted by OLine flags:");
  SND (" ");
  SND ("c - Can see connect notices");
  SND ("g - Can see /GLOBOPS notices");
  SND ("h - Help operator");
  SND ("p - Protected irc operator");
  SND ("W - Can see /WHOIS notices");
  SND (" ");
  SND ("Modes designating IRC operator levels:");
  SND (" ");
  SND ("O - Local irc operator");
  SND ("o - Global irc operator");
  SND (" ");
  SND ("Modes designating services access levels:");
  SND (" ");
  SND ("a - Services operator");
  SND ("P - Services administrator");
  SND ("Z - Services root administrator");
  SND (" ");
  SND ("Syntax: \002/mode nick +/-modes\002");
  SND ("Example: /mode Acidic32 +s");
  SND ("         /mode Acidic32 -s");
  return 0;
}

int
help_ircd_nmode (aClient * source_p)
{
  SND
    ("Allows the user to control what notices the server will send to them.");
  SND ("For a list of accepted notice mode flags see:");
  SND ("/IRCDHELP NMODES");
  SND (" ");
  SND ("Syntax: \002/NMODE +/-flags\002");
  SND ("Example: /NMODE +c");
  return 0;
}

int
help_ircd_oflags (aClient * source_p)
{
  SND
    ("Below is a list of the O:Line Flags currently available, along with their function.");
  SND (" ");
  SND (" o = Local operator");
  SND (" O = Global operator");
  SND (" ");
  SND (" a = Services operator");
  SND (" P = Services administrator");
  SND (" Z = Services root administrator");
  SND (" ");
  SND (" j = Guest administrator");
  SND (" J = Server co administrator");
  SND (" A = Server administrator");
  SND (" t = Technical Co administrator");
  SND (" T = Technical administrator");
  SND (" n = Network Co administrator");
  SND (" N = Network administrator");
  SND (" ");
  SND (" b = Access to do /kline and /unkline");
  SND (" c = Access to do local /connects and /squits");
  SND (" C = Access to do remote /connects and /squits");
  SND (" k = Access to do local /kills");
  SND (" K = Access to do global /kills");
  SND (" r = Access to do /rehash");
  SND (" R = Access to do /restart");
  SND (" D = Access to do /die");
  SND (" ");
  SND (" p = Oper can set +p (Protected IRC Operator)");
  SND (" h = Oper can set +h (Help Operator)");
  SND (" ");
  SND (" w = oper can send /wallops");
  SND
    (" l = Oper can send local server notices (/notice $servername message)");
  SND (" g = Oper can send global notices (/notice $*.network.net message)");
  SND (" ");
  SND
    (" E = Oper can use /add and /del commands from IRC if enabled by the admin.");
  return 0;
}

int
help_ircd_chanmodes (aClient * source_p)
{
  SND
    ("Below is a list of the Channel Modes currently available, along with their function.");
  SND (" ");
  SND ("- These modes dont need any parameters! -");
  SND ("A - Only Administrators can join the channel");
  SND ("c - No Colors (color codes are blocked in messages)");
  SND ("i - Invite Only (cannot join unless /INVITE'd)");
  SND ("K - No Knocks (disables /KNOCK for specific channel)");
  SND ("m - Moderated (only op/halfop/voice users may speak)");
  SND ("M - Only registered users may talk in the channel");
  SND ("n - No External Messages");
  SND ("N - Stops /INVITE's to your channel by non channel staff");
  SND ("O - Only IRC Operators can join the channel");
  SND ("p - Private (will not show on /LIST; cannot join unless invited)");
  SND ("q - Makes a quit appear to be /PART and hides the message");
  SND ("r - Registered Channel");
  SND ("R - Registered Nicks Only (only registered nicknames may join)");
  SND ("s - Secret (will not show on /LIST or in /WHOIS)");
  SND ("S - Only clients using SSL may join the channel");
  SND ("t - Topic Lock (only channel staff may change topic)");
  SND (" ");
  SND ("- These modes need a nickname as parameter! -");
  SND
    ("a <nickname> - Channel Admin (can toggle channel settings and cannot be de opped/halfopped/voiced or kicked");
  SND ("               except by other channel admins or services)");
  SND
    ("o <nickname> - Channel Operator (can toggle channel settings and cannot be kicked by halfops)");
  SND ("h <nickname> - Half Operator (can toggle various channel settings)");
  SND
    ("v <nickname> - Voiced User (can speak even when banned or channel is +m)");
  SND (" ");
  SND ("- These modes need a parameter! -");
  SND
    ("l <number> - User Limit (only specified amount of users may join channel)");
  SND ("k <key> - 'Lock' Channel (must specify correct key to join channel)");
  SND
    ("b <nick!user@host> - Ban (no users matching nick!user@host may join channel)");
  SND
    ("e <nick!user@host> - Exception Ban (users matching nick!user@host may join channel");
  SND ("                     even when banned)");
  SND (" ");
  SND ("Syntax: \002/mode #channel <+/-modes> [parameters]\002");
  SND ("Example: /mode #shadowrealm +nto Acidic32");
  SND ("         /mode #acidroom +b *shadow*!*@*.shadow-realm.org");
  return 0;
}

int
help_ircd_nmodelist (aClient * source_p)
{
  SND ("The following notice mode flags are availible to all users");
  SND (" ");
  SND ("s - Server notices");
  SND ("w - Wallop notices");
  if (!IsAnOper (source_p))
  {
    return 0;
  }
  SND (" ");
  SND ("The following notice mode flags are availible to irc operators");
  SND (" ");
  SND ("b - Blocked DCC Send notices");
  SND ("B - Spam bot notices");
  SND ("c - Local client connect/exit notices");
  SND ("C - Global client connect/exit notices");
  SND ("d - Debug notices");
  SND ("f - Flood notices");
  SND ("g - Various operator chat notices");
  SND ("G - Globops notices");
  SND ("k - Server originated kill notices");
  SND ("n - Network info notices");
  SND ("N - Network global notices");
  SND ("r - Routing notices");
  SND ("R - Reject notices");
  SND ("S - Spy notices");
  return 0;
}

int
help_ircd_admin (aClient * source_p)
{
  SND
    ("Displays information about the Server Administrator. The information includes the");
  SND
    ("administrators nickname, real name and e-mail address on most servers.");
  SND (" ");
  SND ("Syntax: \002/ADMIN [server]\002");
  SND ("Example: /ADMIN whiterose.shadow-realm.org");
  SND ("         /ADMIN whiterose.*");
  return 0;
}

int
help_ircd_away (aClient * source_p)
{
  SND ("Marks you as away on the server.");
  SND ("A line in your whois shows why you are away.");
  SND ("If no reason is specified the away-status is removed.");
  SND (" ");
  SND ("Syntax: \002/AWAY [reason]\002");
  SND ("Example: /AWAY Currently writeing the help");
  return 0;
}

int
help_ircd_nick (aClient * source_p)
{
  SND
    ("Changes your \"online identify\" (nickname) on the server. All channels that you are");
  SND ("a member of will be alerted of your nickname change.");
  SND (" ");
  SND ("Syntax: \002/NICK <newnickname>\002");
  SND ("Example: /NICK Acidic32");
  return 0;
}

int
help_ircd_ircops (aClient * source_p)
{
  SND
    ("Shows you all online IRCops on the network that are availible for help");
  SND ("and their operator level.");
  SND (" ");
  SND ("Syntax: \002/IRCOPS\002");
  return 0;
}

int
help_ircd_whois (aClient * source_p)
{
  SND
    ("Shows you information about the user you specify. The information includes nickname,");
  SND ("username, hostname, real name, etc.");
  SND (" ");
  SND ("Syntax: \002/WHOIS <nickname>\002");
  SND ("Example: /WHOIS ShadowMaster");
  return 0;
}

int
help_ircd_who (aClient * source_p)
{
  SND
    ("Searches visible (-i) user information for the information provided.");
  SND
    ("When used on a channel, it will give back a list of all the visible users on");
  SND ("the specified channel.");
  SND ("To see a list of all supported /WHO parameters see /WHO ?");
  SND (" ");
  SND ("Syntax: \002/WHO <parameter>\002");
  SND ("Example: /WHO Acidic32");
  SND ("         /WHO #ShadowRealm");
  return 0;
}

int
help_ircd_rules (aClient * source_p)
{
  SND
    ("Shows the rules of the server. (This information is saved in ircd.rules).");
  SND
    ("If you don´t specify a parameter, the information of the server you are");
  SND ("connected to, is shown");
  SND (" ");
  SND ("Syntax: \002/RULES [server]\002");
  SND ("Example: /RULES ShadowRealm.*");
  return 0;
}

int
help_ircd_identify (aClient * source_p)
{
  SND
    ("A services alias which allows you to identify your nickname with the nickname");
  SND ("or with the channel service without anyone being able to \'snoop\'.");
  SND (" ");
  SND
    ("Syntax: \002/IDENTIFY <password>\002 - For identifying with the nickname service");
  SND
    ("        \002/IDENTIFY <channel> <password>\002 - For identifying with the channel service");
  return 0;
}

int
help_ircd_ison (aClient * source_p)
{
  SND
    ("Used to determine whether the specified user is currently connected to the network.");
  SND ("You can specify more than one nick. Separate them by a space.");
  SND (" ");
  SND ("Syntax: \002/ISON <nickname> [nickname2 nickname3]\002");
  SND ("Example: /ISON ShadowMaster AgY");
  return 0;
}

int
help_ircd_join (aClient * source_p)
{
  SND
    ("Used to enter the specified channel(s). Users on the specified channel(s)");
  SND ("will be notified of your arrival.");
  SND
    ("You can specify more than one channel by separating the names with a comma.");
  SND
    ("If the channel has a password set (+k) you have to specify it after the");
  SND ("channel name separated by a comma.");
  SND (" ");
  SND ("Syntax: \002/JOIN #channel[,#channel2,#channel3] [password]\002");
  SND ("Example: /JOIN #ShadowRealm");
  SND ("         /JOIN #ShadowRealm,#WorldVillage,#IRCd");
  return 0;
}

int
help_ircd_part (aClient * source_p)
{
  SND
    ("Used to part the specified channel(s). Occupants of the specified channel(s) will be");
  SND ("notified or your departure.");
  SND
    ("You can specify more than one channel by separating the names with a comma.");
  SND (" ");
  SND ("Syntax: \002/PART #channel[,#channel2,#channel3] [message]\002");
  SND ("Example: /PART #ShadowRealm good bye people");
  SND ("         /PART #ShadowRealm,#WorldVillage,#IRCd");
  return 0;
}

int
help_ircd_names (aClient * source_p)
{
  SND
    ("Provides you with a list of all clients which currently occupy the specified channel.");
  SND (" ");
  SND ("Syntax: \002/NAMES <#channel>\002");
  SND ("Example: /NAMES #ShadowRealm");
  return 0;
}

int
help_ircd_userhost (aClient * source_p)
{
  SND ("Returns the hostname of the speicifed user.");
  SND ("You may specify more than one separated by comma.");
  SND (" ");
  SND ("Syntax: \002/USERHOST <nick1> [<nick2> <nick3>]\002");
  SND ("Example: /USERHOST Acidic32 ShadowMaster");
  return 0;
}

int
help_ircd_lusers (aClient * source_p)
{
  SND
    ("Displays the local and global user statistics. The information includes maximum local");
  SND ("user count, maximum global user count, etc.");
  SND (" ");
  SND ("Syntax: \002/LUSERS [server]\002");
  SND ("Example: /LUSERS");
  SND ("         /LUSERS LadyHawke.*");
  return 0;
}

int
help_ircd_map (aClient * source_p)
{
  SND
    ("Displays a graphical map of the IRC Network, and the amount of users on each server.");
  SND (" ");
  SND ("Syntax: \002/MAP\002");
  return 0;
}

int
help_ircd_quit (aClient * source_p)
{
  SND
    ("Disconnects you from the IRC Network. Clients occupying the channels you are in will be");
  SND
    ("notified of your departure, aswell as IRC Operators currently online.");
  SND (" ");
  SND ("Syntax: \002/QUIT [message]\002");
  SND ("Example: /QUIT Goodbye everyone!");
  return 0;
}

int
help_ircd_stats (aClient * source_p)
{
  SND
    ("Reports statistical information about the specified subject. Some characters are limited");
  SND ("to IRC Operators.");
  SND (" ");
  SND ("Syntax: \002/STATS <character>\002");
  SND ("Example: /STATS ?");
  SND ("         /STATS w");
  SND ("         /STATS k");
  return 0;
}

int
help_ircd_links (aClient * source_p)
{
  SND ("Displays a list of IRC Servers linked to the network.");
  SND (" ");
  SND ("Syntax: \002/LINKS\002");
  return 0;
}

int
help_ircd_time (aClient * source_p)
{
  SND ("Displays the current Time and Date of the IRC Server.");
  SND (" ");
  SND ("Syntax: \002/TIME\002");
  return 0;
}

int
help_ircd_invite (aClient * source_p)
{
  SND
    ("Sends the specified user an invitation to join the channel. You must be a channel");
  SND ("operator or half-operator to be able to use this function.");
  SND (" ");
  SND ("Syntax: \002/INVITE <nickname> <channel>\002");
  SND ("Example: /INVITE Acidic32 #ShadowRealm");
  return 0;
}

int
help_ircd_kick (aClient * source_p)
{
  SND
    ("Kicks (removes) the specified user from the channel. You must be a half-operator or higher");
  SND
    ("to use this function. If no reason is specified, reason becomes \'no reason");
  SND ("specified\'.");
  SND (" ");
  SND
    ("Syntax: \002/KICK <channel> <nickname>[,<nick2>,<nick3>] <reason>\002");
  SND ("Example: /KICK #ShadowRealm ShadowMaster");
  SND ("         /KICK #ShadowRealm Acidic32 bad language");
  return 0;
}

int
help_ircd_watch (aClient * source_p)
{
  SND
    ("WATCH is a new type of \'notify\' system. The server will notify you whenever a client");
  SND
    ("in your WATCH list connects or disconnects from IRC. Please note, your WATCH list _DOES_NOT_");
  SND
    ("remain between IRC sessions. You will need to re-build it whenever you reconnect to IRC.");
  SND (" ");
  SND ("Syntax: \002/WATCH [+/-nick] [+/-nick]\002");
  SND ("Example: /WATCH +ShadowMaster +AgY +cg");
  SND ("         /WATCH -ShadowMaster -cg");
  return 0;
}

int
help_ircd_list (aClient * source_p)
{
  SND
    ("Displays a list of channels currently open. The information includes the current number of");
  SND ("channel users, and the channel topic.");
  SND (" ");
  SND
    ("UltimateIRCd also supports \'search options\'. To use a search option, you must prefix the LIST");
  SND
    ("command with \'/QUOTE\' to avoid your IRC Client interpreting the command. If you do not specify");
  SND
    ("a search option, the server will display the full, unfiltered channel list.");
  SND (" ");
  SND ("Search Options:");
  SND (" ");
  SND (" > number      - List channels with more than <number> of users");
  SND (" < number      - List channels with less than <nubmer> of users");
  SND
    (" C > number    - List channels created between now and <number> minutes ago");
  SND
    (" C < number    - List channels created earlier than <number> minutes ago");
  SND
    (" T > number    - List channels whose topics are older than <number> minutes");
  SND
    (" T < number    - List channels whose topics are newer than <number> minutes");
  SND (" *mask*        - List all channels that match *mask*");
  SND (" !*mask*       - List all channels that do not match *mask*");
  SND (" ");
  SND ("Syntax: \002/LIST [parameters]\002");
  SND ("Example: /LIST");
  SND ("         /QUOTE LIST *IRCd*");
  SND ("         /QUOTE LIST < 30");
  return 0;
}

int
help_ircd_privnot (aClient * source_p)
{
  SND
    ("Used to sent text to clients and channels. Most IRC Clients support the alias /MSG");
  SND ("for /PRIVMSG.");
  SND (" ");
  SND ("Example: /PRIVMSG Acidic32 :Hello. Im bored");
  SND ("         /NOTICE Acidic32 hey there! what are you doing?");
  SND
    ("         /NOTICE @#channel :text    - Sends to all channel operators in #channel");
  SND
    ("         /NOTICE %#channel :text    - Sends to all half operators in #channel");
  SND
    ("         /NOTICE +#channel :text    - Sneds to all voiced users in #channel");
  SND ("Note: The syntax of /PRIVMSG and /NOTICE is the same.");
  return 0;
}

int
help_ircd_vhost (aClient * source_p)
{
  SND
    ("Provided you log in using the correct username and password, your hostname");
  SND
    ("will be masked using the virtual host (vhost) provided by the IRC Server.");
  SND
    ("The admin of the server has to set them in vhost.conf before it can be used.");
  SND (" ");
  SND ("Syntax: \002/VHOST <login> <password>\002");
  SND ("Example: /VHOST billgates ilovelinux");
  return 0;
}

int
help_ircd_topic (aClient * source_p)
{
  SND
    ("Used to either display the current topic of the specified channel, or to change the");
  SND ("of the specified channel to the specified topic.");
  SND (" ");
  SND ("Syntax: \002/TOPIC <channel> [topic message]\002");
  SND ("Example: /TOPIC #ShadowRealm");
  SND ("         /TOPIC #ShadowRealm UltimateIRCd 3.0.0 has been released!");
  return 0;
}

int
help_ircd_mode (aClient * source_p)
{
  SND
    ("Sets a mode upon a channel or user. For a full list of UserModes and their function,");
  SND
    ("type \'/IRCDHELP USERMODES\'. For a full list of ChannelModes and their function, type");
  SND ("\'/IRCDHELP CHANMODES\'.");
  SND (" ");
  SND ("Syntax: \002/MODE <nickname/channel> <+/-modes> [parameters]\002");
  SND ("Example: /MODE acidic32 +x-i");
  SND ("         /MODE #ShadowRealm +h-o Acidic32 Acidic32");
  return 0;
}

int
help_ircd_info (aClient * source_p)
{
  SND ("Displays information about the IRC Server.");
  SND (" ");
  SND ("Syntax: \002/INFO [server]\002");
  SND ("Example: /INFO");
  SND ("         /INFO LadyHawke.*");
  return 0;
}

int
help_ircd_version (aClient * source_p)
{
  SND
    ("Displays the version information about the current IRC Server software being used.");
  SND (" ");
  SND ("Syntax: \002/VERSION [server]\002");
  SND ("Example: /VERSION");
  SND ("         /VERSION LadyHawke.*");
  return 0;
}

int
help_ircd_motd (aClient * source_p)
{
  SND
    ("Displays the contents of the servers Message of the Day file (ircd.motd)");
  SND (" ");
  SND ("Syntax: \002/MOTD [server]\002");
  SND ("Example: /MOTD");
  SND ("         /MOTD LadyHawke.*");
  return 0;
}

int
help_ircd_whowas (aClient * source_p)
{
  SND
    ("Displays previous WHOIS information for specified users who are no longer connected");
  SND ("to IRC.");
  SND (" ");
  SND ("Syntax: /WHOWAS <nickname>[,<nick2>,<nick3>]\002");
  SND ("Example: /WHOWAS ShadowMaster");
  return 0;
}

int
help_ircd_mkpasswd (aClient * source_p)
{
  SND
    ("Encrypts the specified text so that it can be added directly to the ircd.conf file");
  SND ("where crypt is enabled.");
  SND (" ");
  SND ("Syntax: \002/MAKEPASS <password>");
  SND ("Example: /MAKEPASS rtfm");
  return 0;
}

int
help_ircd_ping (aClient * source_p)
{
  SND
    ("The PING command is used to test the presence of an active client or server at");
  SND
    ("the other end of the connection. Servers send a PING message at regular intervals");
  SND
    ("if no other activity is detected from the connection. If the connection fails to");
  SND
    ("reply to the PING message within a sertain amount of time, the connection is");
  SND
    ("closed. A PING message may be sent even if the connection is active.");
  SND (" ");
  SND ("Note: This function is not the same as the CTCP PING command.");
  SND (" ");
  SND ("Syntax: \002 PING <server1> <server2>\002");
  SND ("Example: PING LadyHawke.*");
  SND ("         PING Acidic32");
  SND ("         PING Acidic32 LadyHawke.*");
  return 0;
}

int
help_ircd_pong (aClient * source_p)
{
  SND
    ("The PONG command is used to reply to a PING message. If the <server2> parameter");
  SND
    ("is given, this message will be forwarded to the given target. The <server1>");
  SND
    ("parameter is the name of the entity who has responded to the PING message using");
  SND ("the PONG message.");
  SND (" ");
  SND ("Syntax: \002 PONG <server1> <server2>\002");
  SND
    ("Example: PONG DarkFyre.* Hub.*     - PONG message from DarkFyre.* to Hub.*");
  return 0;
}

int
help_ircd_sethost (aClient * source_p)
{
  SND
    ("Allows you to change your host or another users host (restricted to IRCops)");
  SND (" ");
  SND ("Syntax: \002/SETHOST <nickname> <host>\002");
  SND ("Example: /SETHOST Acidic32 i.code.for.shadow-realm.org");
  return 0;
}

int
help_ircd_fjoin (aClient * source_p)
{
  SND ("Allows an Operator to force entry into the specified channel");
  SND
    ("This overrides any bans and other limits by modes in place on the channel.");
  SND (" ");
  SND ("Syntax: \002/FJOIN <channel>\002");
  SND ("Example: /FJOIN #ShadowRealm");
  return 0;
}

int
help_ircd_netglobal (aClient * source_p)
{
  SND ("This allows you to send Global IRCop messages.");
  SND ("Useful for network infomation broadcasting.");
  SND (" ");
  SND ("Syntax: \002/NETGLOBAL :<text>\002");
  SND ("Example: /NETGLOBAL :Hi ALL Network Opers!!");
  return 0;
}

int
help_ircd_fmode (aClient * source_p)
{
  SND
    ("This allows IRCops with the correct permissions to force modes on a channel.");
  SND ("Useful for setting Channel Modes incase of floods/takeovers.");
  SND (" ");
  SND ("Syntax: \002/FMODE <channel> <+/-modes> [parameters]\002");
  SND ("Example: /FMODE #ShadowRealm +R");
  SND ("         /FMODE #ShadowRealm +o Acidic32");
  SND ("         /FMODE #ShadowRealm +Ro Acidic32");
  return 0;
}

int
help_ircd_oper (aClient * source_p)
{
  SND
    ("Used to 'Oper up'. When it is done correctly, IRC operator privilege");
  SND ("is granted to the person who triggered it.");
  SND
    ("IRC operator level and ability is determined by the O:line in the server config");
  SND ("");
  SND ("Syntax: \002/oper <username> <password>\002");
  SND ("Example: /oper Coder abcdef");
  return 0;
}

int
help_ircd_rehash (aClient * source_p)
{
  SND
    ("This command forces the IRC Server to re-load its standard configuration files. (ircd.conf, kline.conf, vhost.conf)");
  SND
    ("You can specify flags to let the ircd rehash specific parts locally.");
  SND ("List of flags you can specify:");
  SND (" ");
  SND ("DNS -- Flushes the DNS cache and re-reads /etc/resolve.conf");
  SND ("TKLINES -- Clears temporary kLines");
  SND ("GC -- Garbage Collecting");
  SND ("MOTD -- Re-read the MOTD file and SHORT MOTD file");
  SND ("RULES -- Re-read the RULES file");
  SND ("OPERMOTD -- Re-read the OPER MOTD file");
  SND ("IP -- Rehash the iphash");
  SND ("AKILLS -- Rehash akills");
  SND ("DYNCONF -- Rehash the dynamic config files");
  SND ("CHANNELS -- Rehash the channel restrict config file");
  SND ("THROTTLES -- Rehash the throttles");
  SND (" ");
  SND ("Syntax: \002/REHASH <flag>\002");
  SND ("Example: /REHASH");
  SND ("         /REHASH DYNCONF");
  return 0;
}

int
help_ircd_wallops (aClient * source_p)
{
  SND
    ("Sends a message to all users who have usermode +w set. Only IRC Operators may send WALLOPS,");
  SND ("however any user can set themselves +w and view WALLOP messages.");
  SND (" ");
  SND ("Syntax: \002/WALLOPS <text>\002");
  SND ("Example: /WALLOPS Hello everyone who has mode +w set!");
  return 0;
}

int
help_ircd_restart (aClient * source_p)
{
  SND ("This command is used to restart an IRCd should the need ever arise");
  SND ("Requires a password if a X:Line is present in the configuration.");
  SND ("");
  SND ("Syntax: \002/restart [password]\002");
  SND ("Example: /restart shadowrealmdotorg");
  return 0;
}

int
help_ircd_adchat (aClient * source_p)
{
  SND ("Sends a message to all online Server and Co-Server admins.");
  SND ("Can be used by Co-Server admins and above.");
  SND ("");
  SND ("Syntax: \002/adchat :<text>\002");
  SND ("Example: /adchat :Hello admins");
  return 0;
}

int
help_ircd_die (aClient * source_p)
{
  SND
    ("Used to terminate (shut down) the IRC Server. Requires password if a X:Line");
  SND ("is present in the configuration file.");
  SND (" ");
  SND ("Syntax: \002/DIE [password]\002");
  SND ("Example: /DIE shadowrealmdotorg");
  return 0;
}

int
help_ircd_kill (aClient * source_p)
{
  SND ("Forces the disconnect of the specified user(s) from the server.");
  SND (" ");
  SND ("Syntax: \002/KILL <user1>[,<user2>,<user3>] <reason>\002");
  SND ("Example: /KILL Acidic32,ShadowMaster,B3N goodbye lamers!");
  return 0;
}

int
help_ircd_squit (aClient * source_p)
{
  SND ("Causes the IRCd to terminate it's connection to the network.");
  SND ("To squit local, do not specify a server.");
  SND ("");
  SND ("Syntax: \002/squit <server>\002");
  SND ("Example: /squit theserver.*");
  return 0;
}

int
help_ircd_chanfix (aClient * source_p)
{
  SND
    ("This command allows Administrators to resync the timestamp of a channel.");
  SND ("It can be used to fix channels with invalid ts time.");
  SND (" ");
  SND ("Syntax: \002/CHANFIX <channel>\002");
  SND ("Example: /CHANFIX #ShadowRealm");
  return 0;
}

int
help_ircd_connect (aClient * source_p)
{
  SND
    ("Links an IRC Server to the server you are currently on. UltimateIRCd also allows");
  SND ("\'remote\' connections.");
  SND (" ");
  SND ("Syntax: \002/CONNECT <server>\002");
  SND ("        \002/CONNECT <server1> <port> <server2>\002");
  SND ("Example: /CONNECT Hub.*");
  SND ("         /CONNECT MadChat.* 7000 Hub.*");
  return 0;
}

int
help_ircd_globops (aClient * source_p)
{
  SND
    ("Sends a message to all IRC Operators connected to the network. Unlike WALLOPS,");
  SND
    ("the messages are only viewable by IRC Operators. Normally used for important");
  SND ("network information.");
  SND (" ");
  SND ("Syntax: \002/GLOBOPS <text>\002");
  SND ("Example: /GLOBOPS Them damn flooders are back!");
  return 0;
}

int
help_ircd_kline (aClient * source_p)
{
  SND ("Adds a k:line to the server which is temporary and will disappear");
  SND
    ("once the server is restarted or crashes or such unless 0 is given as");
  SND ("the delay. At which point the k:line becomes a K:line.");
  SND ("");
  SND ("");
  SND ("Syntax: \002/kline <duration> <mask> :<reason>\002");
  SND ("");
  SND ("Example: /kline 60 *@127.0.0.6 :This is almost local host");
  SND ("");
  SND ("If you don't specify a duration the default of 30 minutes is used.");
  SND
    ("Once again if you specify 0 as a duration, the server will add the ban");
  SND ("to the kline config means it will be permanent.");
  return 0;
}

int
help_ircd_locops (aClient * source_p)
{
  SND
    ("Similar to GLOBOPS, accept only IRC Operators on your server receive");
  SND ("the message.");
  SND (" ");
  SND ("Syntax: \002/LOCOPS <text>\002");
  SND ("Example: /LOCOPS Hello to all the IRC Operators on my server!");
  return 0;
}

int
help_ircd_unkline (aClient * source_p)
{
  SND ("Removes a k:line or K:line from the server.");
  SND
    ("Note: If it is a K:line it has to be in the kline config or you will");
  SND ("not be able to remove it with /unkline");
  SND (" ");
  SND ("Syntax: \002/unkline <mask>\002");
  SND ("Example: /unkline *@127.0.0.6");
  return 0;
}

int
help_ircd_trace (aClient * source_p)
{
  SND ("Used to find out what servers are connected to what.");
  SND (" ");
  SND ("Syntax: \002/TRACE [server/nick]\002");
  SND ("Example: /TRACE Shadow.*");
  return 0;
}

int
help_ircd_about (aClient * source_p)
{
  SND ("The Ultimate Help System was coded by Acidic32 (aka Daniel Moss)");
  SND
    ("as a replacement for documentation files. For more information on the IRCd");
  SND ("itself, type \'/INFO\'");
  SND (" ");
  SND
    ("The UltimateIRCd Help System - Copyright (C) Daniel Moss, UltimateIRCd, Inc.");
  return 0;
}

int
help_ircd_channel (aClient * source_p)
{
  SND ("Used to join into a channel. Works the same way like JOIN.");
  SND ("To get help about the syntax type \002/IRCDHELP JOIN\002");
  return 0;
}

int
help_ircd_credits (aClient * source_p)
{
  SND ("Shows the credits given to some persons by the coders.");
  SND ("Syntax: \002/CREDITS\002");
  return 0;
}

int
help_ircd_copyright (aClient * source_p)
{
  SND ("Shows copyright information about the IRCd.");
  SND ("Syntax:\002 /COPYRIGHT\002");
  return 0;
}

int
help_ircd_dccallow (aClient * source_p)
{
  SND ("Allows users on the list to send you files");
  SND ("that are blocked by default for security reasons.");
  SND ("Syntax:\002 /DCCALLOW [+/-nick[,+/-nick, ...]] [list] [help]\002");
  SND ("You can get a better documented help with \002/DCCALLOW HELP\002");
  return 0;
}

int
help_ircd_help (aClient * source_p)
{
  SND
    ("Shows the UltimateIRCd helpsystem. Shows how to use commands and other features.");
  SND ("Syntax: \002/IRCDHELP [subtopic]\002");
  SND ("or");
  SND ("Syntax: \002/HELP [subtopic]\002");
  SND ("Example: /IRCDHELP NMODES");
  return 0;
}

int
help_ircd_settings (aClient * source_p)
{
  SND ("Shows some settings defined by configuration files.");
  SND ("It depends on your status what information you can see.");
  SND ("Type /SETTINGS without a paramter to see the subtopics you can see.");
  SND (" ");
  SND ("Syntax: \002/SETTINGS [subtopic]\002");
  SND ("Example: /SETTINGS CHANNELS");
  return 0;
}

int
help_ircd_knock (aClient * source_p)
{
  SND ("Knocks on a channel. Sends a message to the channel staff.");
  SND
    ("Use it to tell the staff of an invited only channel that you want to join.");
  SND (" ");
  SND ("Syntax: \002/KNOCK <channel> :<message>\002");
  SND ("Example: /KNOCK #ShadowRealm :I want to join in");
  return 0;
}

int
help_ircd_silence (aClient * source_p)
{
  SND ("Silence is a server-side ignore system used to ignore users");
  SND ("based on their nick!user@host mask. If no parameters are specified");
  SND ("a list of all masks on your silence list is shown");
  SND (" ");
  SND ("Syntax: \002/SILENCE [+/-nick!user@host]\002");
  SND ("Example: /SILENCE +ShadowMaster!*@Technical-Admin.net");
  return 0;
}

int
help_ircd_users (aClient * source_p)
{
  SND ("Shows the current local and global usercount.");
  SND
    ("You can also look up the remote local count by specifying a servername.");
  SND (" ");
  SND ("Syntax: \002/USERS [server]\002");
  SND ("Example: /USERS ShadowRealm.MI.*");
  return 0;
}
