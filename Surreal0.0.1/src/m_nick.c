/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/m_nick.c
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
 *  $Id: m_nick.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include <sys/stat.h>
#ifndef _WIN32
# include <utmp.h>
#endif
#include <fcntl.h>
#include "h.h"

extern int do_user (char *, aClient *, aClient *, char *, char *, char *,
                    unsigned long, unsigned char *, char *, char *);

#ifdef CHINESE_SUPPORT
int isvalidChinese (char c1, char c2);
#endif


extern int user_mode_table[];
extern int server_mode_table[];


#ifdef CHINESE_SUPPORT
int
isvalidChinese (char c1, char c2)
{
  int longCode;
  int longCodeStart;
  int longCodeEnd;
  char StartEndChar[5];

  if (c2 == (char) 0xff)
  {
    return 0;
  }

  strcpy (StartEndChar, "\xb0\xa1\xf5\xdd");
  longCodeStart = (StartEndChar[0] - 161) * 94 + StartEndChar[1] - 161;
  longCodeEnd = (StartEndChar[2] - 161) * 94 + StartEndChar[3] - 161;
  longCode = (c1 - 161) * 94 + c2 - 161;

  if (longCode < longCodeStart || longCode > longCodeEnd)
  {
    return 0;
  }

  return 1;
}

static int
do_nick_name (char *pnick)
{
  unsigned char *ch;
  unsigned char *nick = (unsigned char *) pnick;
  int firstChineseChar = 0;
  char lastChar;

  if (*nick == '-' || MyIsDigit (*nick))        /* first character in [0..9-] */
  {
    return 0;
  }

  for (ch = nick; *ch && (ch - nick) < NICKLEN; ch++)
  {
    if ((!isvalid (*ch) && !((*ch) & 0x80)) || MyIsSpace (*ch)
        || (*ch) == '@' || (*ch) == '!' || (*ch) == ':' || (*ch) == ' ')
    {
      break;
    }

    if (firstChineseChar)
    {
      if (!isvalidChinese (lastChar, *ch))
      {
        break;
      }

      firstChineseChar = 0;
    }

    else if ((*ch) & 0x80)
    {
      firstChineseChar = 1;
    }

    lastChar = *ch;
  }
  if (firstChineseChar)
  {
    ch--;
  }

  *ch = '\0';

  return (ch - nick);
}
#else

/*
 * * 'do_nick_name' ensures that the given parameter (nick) is * really
 * a proper string for a nickname (note, the 'nick' * may be modified
 * in the process...) *
 *
 *      RETURNS the length of the final NICKNAME (0, if *
 * nickname is illegal) *
 *
 *  Nickname characters are in range *  'A'..'}', '_', '-', '0'..'9' *
 * anything outside the above set will terminate nickname. *  In
 * addition, the first character cannot be '-' *  or a Digit. *
 *
 *  Note: *     '~'-character should be allowed, but *  a change should
 * be global, some confusion would *    result if only few servers
 * allowed it...
 */

static int
do_nick_name (char *nick)
{
  char *ch;

  if (*nick == '-' || MyIsDigit (*nick))        /* first character is [0..9-] */
    return 0;

  for (ch = nick; *ch && (ch - nick) < NICKLEN; ch++)
    if (!isvalid (*ch) || MyIsSpace (*ch))
      break;

  *ch = '\0';

  return (ch - nick);
}
#endif /* CHINESE_SUPPORT */

/*
 * m_nick
 * parv[0] = sender prefix 
 * parv[1] = nickname 
 * parv[2] = hopcount when new user; TS when nick change 
 * parv[3] = TS
 * ---- new user only below ---- 
 * parv[4] = umode
 * parv[5] = username 
 * parv[6] = hostname 
 * parv[7] = server 
 * parv[8] = serviceid
 * parv[9] = IP
 * parv[10] = ircname
 */
int
m_nick (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aConfItem *aconf;
  aClient *target_p, *uplink;
  Link *lp;
  char nick[NICKLEN + 2];
  ts_val newts = 0;
  int sameuser = 0, samenick = 0;
  char *mptr;

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NONICKNAMEGIVEN), me.name, parv[0]);
    return 0;
  }

  if (!IsServer (source_p) && IsServer (client_p) && parc > 2)
  {
    newts = atol (parv[2]);
  }
  else if (IsServer (source_p) && parc > 3)
  {
    newts = atol (parv[3]);
  }
  else
  {
    parc = 2;
  }

  /*
   * parc == 2 on a normal client sign on (local) and a normal client 
   * nick change
   * parc == 4 on a normal server-to-server client nick change
   * parc == 11 on a normal TS style server-to-server NICK introduction
   */
  if ((IsServer (source_p) || (parc > 4)) && (parc < 11))
  {
    /*
     * We got the wrong number of params. Someone is trying to trick
     * us. Kill it. -ThemBones As discussed with ThemBones, not much
     * point to this code now sending a whack of global kills would
     * also be more annoying then its worth, just note the problem,
     * and continue -Dianora
     */
    ircstp->is_kill++;
    sendto_realops_lev (DEBUG_LEV,
                        "Ignoring bad NICK with wrong number of parameters: %s[%s@%s] on %s (from %s)",
                        parv[1], (parc >= 6) ? parv[5] : "-",
                        (parc >= 7) ? parv[6] : "-",
                        (parc >= 8) ? parv[7] : "-", parv[0]);

    sendto_one (client_p,
                ":%s KILL %s :Bad NICK with wrong number of parameters",
                me.name, parv[1]);
    return 0;

  }

  /* blub an ipv6 host contains not dots ...
   * Lets see what we can do .. :) - Againaway
   */

  if ((parc == 11) && ((!strchr (parv[6], '.')) && (!strchr (parv[6], ':'))))
  {
    /*
     * Ok, we got the right number of params, but there isn't a
     * single dot in the hostname, which is suspicious. Don't fret
     * about it just kill it. - ThemBones
     */
    ircstp->is_kill++;
    sendto_realops_lev (DEBUG_LEV,
                        "Ignoring NICK with invalid hostname: %s[%s@%s] on %s (from %s)",
                        parv[0], parv[5], parv[6], parv[7], parv[0]);

    sendto_one (client_p,
                ":%s KILL %s :%s (Bad NICK with invalid hostname)", me.name,
                parv[1], me.name);
    return 0;
  }

  strncpyzt (nick, parv[1], NICKLEN + 1);
  /*
   * if do_nick_name() returns a null name OR if the server sent a
   * nick name and do_nick_name() changed it in some way (due to rules
   * of nick creation) then reject it. If from a server and we reject
   * it, and KILL it. -avalon 4/4/92
   */
  if (do_nick_name (nick) == 0
      || (IsServer (client_p) && strcmp (nick, parv[1])))
  {
    sendto_one (source_p, err_str (ERR_ERRONEUSNICKNAME),
                me.name, parv[0], parv[1], "Erroneous Nickname", "N/A");

    if (IsServer (client_p))
    {
      ircstp->is_kill++;
      sendto_realops_lev (DEBUG_LEV, "Bad Nick: %s From: %s %s",
                          parv[1], parv[0], get_client_name (client_p,
                                                             FALSE));
      sendto_one (client_p, ":%s KILL %s :%s (Bad Nick)", me.name,
                  parv[1], me.name);
      if (source_p != client_p)
      {                         /* bad nick change */
        sendto_serv_butone (client_p, ":%s KILL %s :%s (Bad Nick)",
                            me.name, parv[0], me.name);
        source_p->flags |= FLAGS_KILLED;
        return exit_client (client_p, source_p, &me, "BadNick");
      }
    }
    return 0;
  }
  /*
   * Check against nick name collisions.
   *
   * Put this 'if' here so that the nesting goes nicely on the screen
   * :) We check against server name list before determining if the
   * nickname is present in the nicklist (due to the way the below
   * for loop is constructed). -avalon
   */
  do
  {
    if ((target_p = find_server (nick, NULL)))
      if (MyConnect (source_p))
      {
        sendto_one (source_p, err_str (ERR_NICKNAMEINUSE), me.name,
                    BadPtr (parv[0]) ? "*" : parv[0], nick);
        return 0;
      }

    /*
     * target_p already has result from find_server
     * Well. unless we have a capricious server on the net, a nick can
     * never be the same as a server name - Dianora
     * That's not the only case; maybe someone broke do_nick_name
     * or changed it so they could use "." in nicks on their network 
     * - sedition
     */

    if (target_p)
    {
      /*
       * We have a nickname trying to use the same name as a
       * server. Send out a nick collision KILL to remove the
       * nickname. As long as only a KILL is sent out, there is no
       * danger of the server being disconnected.  Ultimate way to
       * jupiter a nick ? >;-). -avalon
       */
      sendto_realops_lev (SKILL_LEV, "Nick collision on %s", source_p->name);
      ircstp->is_kill++;
      sendto_one (client_p, ":%s KILL %s :%s (Nick Collision)", me.name,
                  source_p->name, me.name);
      source_p->flags |= FLAGS_KILLED;
      return exit_client (client_p, source_p, &me, "Nick/Server collision");
    }

    if (!(target_p = find_client (nick, NULL)))
      break;

    /*
     * If target_p == source_p, then we have a client doing a nick change
     * between *equivalent* nicknames as far as server is concerned
     * (user is changing the case of his/her nickname or somesuch)
     */
    if (target_p == source_p)
    {
      if (strcmp (target_p->name, nick) == 0)
        return 0;
      else
        break;
    }                           /* If user is changing nick to itself no point in propogating */

    /*
     * Note: From this point forward it can be assumed that target_p !=
     * source_p (point to different client structures).
     *
     * If the older one is "non-person", the new entry is just 
     * allowed to overwrite it. Just silently drop non-person, and
     * proceed with the nick. This should take care of the "dormant
     * nick" way of generating collisions...
     */
    if (IsUnknown (target_p))
    {
      if (MyConnect (target_p))
      {
        exit_client (NULL, target_p, &me, "Overridden");
        break;
      }
      else if (!(target_p->user))
      {
        sendto_realops_lev (SKILL_LEV,
                            "Nick Collision on %s - Got TS NICK before Non-TS USER",
                            parv[1]);
        sendto_serv_butone (NULL,
                            ":%s KILL %s :%s (Nick Collission - Got TS NICK before Non-TS USER)",
                            me.name, target_p->name, me.name);
        target_p->flags |= FLAGS_KILLED;
        /* Having no USER struct should be ok... */
        return exit_client (client_p, target_p, &me,
                            "Nick Collision - Got TS NICK before Non-TS USER");
      }
    }

    if (!IsServer (client_p))
    {
      /*
       * NICK is coming from local client connection. Just send
       * error reply and ignore the command.
       * parv[0] is empty on connecting clients
       */
      sendto_one (source_p, err_str (ERR_NICKNAMEINUSE),
                  me.name, BadPtr (parv[0]) ? "*" : parv[0], nick);
      return 0;
    }
    /*
     * NICK was coming from a server connection. Means that the same
     * nick is registered for different users by different server.
     * This is either a race condition (two users coming online about
     * same time, or net reconnecting) or just two net fragments
     * becoming joined and having same nicks in use. We cannot have
     * TWO users with same nick--purge this NICK from the system with
     * a KILL... >;)
     *
     * Changed to something reasonable like IsServer(source_p) (true if
     * "NICK new", false if ":old NICK new") -orabidoo
     */

    if (IsServer (source_p))
    {
      /*
       * A new NICK being introduced by a neighbouring server (e.g.
       * message type "NICK new" received)
       */
      if (!newts || !target_p->tsinfo || (newts == target_p->tsinfo))
      {
        sendto_realops_lev (SKILL_LEV, "Nick collision on %s", parv[1]);
        ircstp->is_kill++;
        sendto_one (target_p, err_str (ERR_NICKCOLLISION),
                    me.name, target_p->name, target_p->name);
        sendto_serv_butone (NULL, ":%s KILL %s :%s (Nick Collission)",
                            me.name, target_p->name, me.name);
        target_p->flags |= FLAGS_KILLED;
        return exit_client (client_p, target_p, &me, "Nick collision");
      }
      else
      {
        sameuser = ((target_p->user) &&
                    (irccmp (target_p->user->username, parv[5]) == 0) &&
                    (irccmp (target_p->user->host, parv[6]) == 0));
        if ((sameuser && newts < target_p->tsinfo) ||
            (!sameuser && newts > target_p->tsinfo))
        {
          return 0;
        }
        else
        {
          sendto_realops_lev (SKILL_LEV, "Nick collision on %s", parv[1]);
          ircstp->is_kill++;
          sendto_one (target_p, err_str (ERR_NICKCOLLISION),
                      me.name, target_p->name, target_p->name);
          sendto_serv_butone (source_p,
                              ":%s KILL %s :%s (Nick Collision)",
                              me.name, target_p->name);
          target_p->flags |= FLAGS_KILLED;
          (void) exit_client (client_p, target_p, &me, "Nick collision");
          break;
        }
      }
    }
    /*
     * * A NICK change has collided (e.g. message type * ":old NICK
     * new". This requires more complex cleanout. * Both clients must be
     * purged from this server, the "new" * must be killed from the
     * incoming connection, and "old" must * be purged from all outgoing
     * connections.
     */
    if (!newts || !target_p->tsinfo || (newts == target_p->tsinfo)
        || !source_p->user)
    {
      sendto_realops_lev (SKILL_LEV, "Nick change collision: %s", parv[1]);
      ircstp->is_kill++;
      sendto_one (target_p, err_str (ERR_NICKCOLLISION),
                  me.name, target_p->name, target_p->name);
      sendto_serv_butone (NULL, ":%s KILL %s :%s (Nick Collision)",
                          me.name, source_p->name, me.name);
      ircstp->is_kill++;
      sendto_serv_butone (NULL, ":%s KILL %s :%s (Nick Collision)",
                          me.name, target_p->name, me.name);
      target_p->flags |= FLAGS_KILLED;
      (void) exit_client (NULL, target_p, &me, "Nick collision(new)");
      source_p->flags |= FLAGS_KILLED;
      return exit_client (client_p, source_p, &me, "Nick collision(old)");
    }
    else
    {
      sameuser =
        ((irccmp (target_p->user->username, source_p->user->username) ==
          0) && (irccmp (target_p->user->host, source_p->user->host) == 0));
      if ((sameuser && newts < target_p->tsinfo)
          || (!sameuser && newts > target_p->tsinfo))
      {
        if (sameuser)
          sendto_realops_lev (SKILL_LEV,
                              "Nick change collision from %s to %s",
                              source_p->name, target_p->name);
        ircstp->is_kill++;
        sendto_serv_butone (client_p,
                            ":%s KILL %s :%s (Nick Collision)", me.name,
                            source_p->name, me.name);
        source_p->flags |= FLAGS_KILLED;
        if (sameuser)
          return exit_client (client_p, source_p, &me, "Nick collision(old)");
        else
          return exit_client (client_p, source_p, &me, "Nick collision(new)");
      }
      else
      {
        sendto_realops_lev (SKILL_LEV, "Nick collision on %s",
                            target_p->name);

        ircstp->is_kill++;
        sendto_one (target_p, err_str (ERR_NICKCOLLISION),
                    me.name, target_p->name, target_p->name);
        sendto_serv_butone (source_p,
                            ":%s KILL %s :%s (Nick Collision)", me.name,
                            target_p->name, me.name);
        target_p->flags |= FLAGS_KILLED;
        (void) exit_client (client_p, target_p, &me, "Nick collision");
      }
    }
  }
  while (0);

  if (IsServer (source_p))
  {
    uplink = find_server (parv[7], NULL);
    if (!uplink)
    {
      /* if we can't find the server this nick is on, 
       * complain loudly and ignore it. - lucas */
      sendto_realops ("Remote nick %s on UNKNOWN server %s", nick, parv[7]);
      return 0;
    }
    source_p = make_client (client_p, uplink);

    /* If this is on a U: lined server, it's a U: lined client. */
    if (IsULine (uplink))
      source_p->flags |= FLAGS_ULINE;

    add_client_to_list (source_p);
    if (parc > 2)
      source_p->hopcount = atoi (parv[2]);
    if (newts)
    {
      source_p->tsinfo = newts;
    }
    else
    {
      newts = source_p->tsinfo = (ts_val) timeofday;
      ts_warn ("Remote nick %s introduced without a TS", nick);
    }
    /*
     * copy the nick in place
     */
    (void) strcpy (source_p->name, nick);
    (void) add_to_client_hash_table (nick, source_p);
    if (parc >= 11)
    {
      int flag;

      /* parse the usermodes intelligently */
      mptr = &parv[4][1];
      while (*mptr)
      {
        if ((flag = user_mode_table[(unsigned char) *mptr]))
        {
          if (((flag == UMODE_o) || (flag == UMODE_O)) && !IsULine (source_p))
            Count.oper++;
          source_p->umode |= flag & SEND_UMODES;
        }
        mptr++;
      }

      /* FIXME ipv6 via NICK is broken - AgAiNaWaY */
      if ((strchr (parv[9], ':')) && (!strchr (parv[9], '.')))
      {
        SetIPV6Client (source_p);

        do_user (nick, client_p, source_p, parv[5], parv[6],
                 parv[7], strtoul (parv[8], NULL, 0),
                 (unsigned char *) parv[9], parv[10], NULL);
      }
      else
      {
        do_user (nick, client_p, source_p, parv[5], parv[6],
                 parv[7], strtoul (parv[8], NULL, 0),
                 (unsigned char *) strtoul (parv[9], NULL, 0), parv[10],
                 NULL);
      }
      /*
       * If the introduced nickname is not part of a server burst, generate
       * a remote connection notice for it.
       */
      if (IsEOBurst (uplink) && IsEOBurst (client_p))
      {

        sendto_globalconnectnotice
          ("from %s: Client connecting: %s (%s@%s) [%s]%s [%s]",
           uplink->name, nick, parv[6], parv[7], source_p->hostip,
           IsSSLClient (source_p) ? " (SSL)" : "", parv[12]);
      }
      return 0;
    }
  }
  else if (source_p->name[0])
  {


#ifdef DONT_CHECK_QLINE_REMOTE
    if (MyConnect (source_p))
    {
#endif
      if ((aconf = find_conf_name (nick, CONF_QUARANTINED_NICK)))
      {
#ifndef DONT_CHECK_QLINE_REMOTE
        if (!MyConnect (source_p))
          sendto_realops_lev (REJ_LEV, "Q:lined nick %s from %s on %s",
                              nick, (*source_p->name != 0
                                     && !IsServer (source_p)) ?
                              source_p->name : "<unregistered>",
                              (source_p->user ==
                               NULL) ? ((IsServer (source_p)) ? parv[6]
                                        : me.name) : source_p->user->server);
#endif

        if (MyConnect (source_p) && (!IsServer (client_p))
            && (!IsOper (client_p)) && (!IsULine (source_p)))
        {
          sendto_one (source_p, err_str (ERR_ERRONEUSNICKNAME),
                      me.name, BadPtr (parv[0]) ? "*" : parv[0], nick,
                      BadPtr (aconf->
                              passwd) ? "Erroneous Nickname" : aconf->
                      passwd, BadPtr (aconf->name) ? "N/A" : aconf->name);
          sendto_realops_lev (REJ_LEV,
                              "Forbidding Q:lined nick %s from %s.",
                              nick, get_client_name (client_p, FALSE));
          return 0;
        }
      }
#ifdef DONT_CHECK_QLINE_REMOTE
    }
#endif
    if (MyConnect (source_p))
    {
      if (IsRegisteredUser (source_p))
      {

        /* before we change their nick, make sure they're not banned
         * on any channels, and!! make sure they're not changing to
         * a banned nick -sed */
        /* a little cleaner - lucas */

        for (lp = source_p->user->channel; lp; lp = lp->next)
        {
          if (can_send (source_p, lp->value.channel_p, NULL))
          {
            sendto_one (source_p, err_str (ERR_BANNICKCHANGE),
                        me.name, source_p->name, lp->value.channel_p->chname);
            return 0;
          }
          if (nick_is_banned (lp->value.channel_p, nick, source_p) != NULL)
          {
            sendto_one (source_p, err_str (ERR_BANONCHAN), me.name,
                        source_p->name, nick, lp->value.channel_p->chname);
            return 0;
          }
        }
#ifdef ANTI_NICK_FLOOD
        if ((source_p->last_nick_change + MAX_NICK_TIME) < NOW)
          source_p->number_of_nick_changes = 0;
        source_p->last_nick_change = NOW;
        source_p->number_of_nick_changes++;

        if (source_p->number_of_nick_changes > MAX_NICK_CHANGES &&
            !IsAnOper (source_p))
        {
          sendto_one (source_p,
                      ":%s NOTICE %s :*** Notice -- Too many nick changes. Wait %d seconds before trying again.",
                      me.name, source_p->name, MAX_NICK_TIME);
          return 0;
        }
#endif
        /* If it changed nicks, -r it */
        if ((source_p->umode & UMODE_r) && (irccmp (parv[0], nick) != 0))
        {
          unsigned int oldumode;
          char mbuf[BUFSIZE];

          oldumode = source_p->umode;
          source_p->umode &= ~UMODE_r;
          send_umode (source_p, source_p, oldumode, ALL_UMODES, mbuf);
        }

        /*
         * Client just changing his/her nick. If he/she is on a
         * channel, send note of change to all clients on that channel.
         * Propagate notice to other servers.
         */

        /* if the nickname is different, set the TS */
        if (irccmp (parv[0], nick) != 0)
        {
          source_p->tsinfo = newts ? newts : (ts_val) timeofday;
        }

        sendto_common_channels (source_p, ":%s NICK :%s", parv[0], nick);
        if (source_p->user)
        {
          add_history (source_p, 1);

          sendto_serv_butone (client_p, ":%s NICK %s :%ld",
                              parv[0], nick, source_p->tsinfo);
        }
      }
    }
    else
    {
      /*
       * Client just changing his/her nick. If he/she is on a
       * channel, send note of change to all clients on that channel.
       * Propagate notice to other servers.
       */

      /* if the nickname is different, set the TS */
      if (irccmp (parv[0], nick) != 0)
      {
        source_p->tsinfo = newts ? newts : (ts_val) timeofday;
      }

      sendto_common_channels (source_p, ":%s NICK :%s", parv[0], nick);
      if (source_p->user)
      {
        add_history (source_p, 1);

        sendto_serv_butone (client_p, ":%s NICK %s :%ld",
                            parv[0], nick, source_p->tsinfo);

        /* If it changed nicks, -r it */
        if (irccmp (parv[0], nick))
        {
          source_p->umode &= ~UMODE_r;
        }
      }
    }
  }
  else
  {
    /* Client setting NICK the first time */
    if (MyConnect (source_p))
    {
      if ((aconf = find_conf_name (nick, CONF_QUARANTINED_NICK)))
      {
        if (MyConnect (source_p) && (!IsServer (client_p))
            && (!IsOper (client_p)) && (!IsULine (source_p)))
        {
          sendto_one (source_p, err_str (ERR_ERRONEUSNICKNAME),
                      me.name, BadPtr (parv[0]) ? "*" : parv[0], nick,
                      BadPtr (aconf->
                              passwd) ? "Erroneous Nickname" : aconf->
                      passwd, BadPtr (aconf->name) ? "N/A" : aconf->name);
          sendto_realops_lev (REJ_LEV,
                              "Forbidding Q:lined nick %s from <unregistered>%s.",
                              nick, get_client_name (client_p, FALSE));
          return 0;
        }
      }
    }

    (void) strcpy (source_p->name, nick);
    source_p->tsinfo = timeofday;
    if (source_p->user)
    {
      /* USER already received, now we have NICK */

      if (register_user
          (client_p, source_p, nick,
           source_p->user->username) == FLUSH_BUFFER)
        return FLUSH_BUFFER;
    }
  }
  /* Finally set new nick name. */

  if (source_p->name[0])
  {
    del_from_client_hash_table (source_p->name, source_p);
    samenick = ((irccmp (source_p->name, nick) != 0) ? 0 : 1);
    if (IsPerson (source_p) && !samenick)
    {
      hash_check_watch (source_p, RPL_LOGOFF);
    }
  }
  strcpy (source_p->name, nick);
  add_to_client_hash_table (nick, source_p);
  if (IsPerson (source_p) && !samenick)
  {
    hash_check_watch (source_p, RPL_LOGON);
  }
  return 0;
}

/*
 * This is pretty much just a cleaned up version of m_nick with hiddenhost and smode added.
 *
 * m_client
 * parv[0] = sender prefix
 * parv[1] = nickname
 * parv[2] = hopcount when new user
 * parv[3] = TS
 * parv[4] = umode
 * parv[5] = smode
 * parv[6] = username
 * parv[7] = hostname
 * parv[8] = hiddenhost (* if none)
 * parv[9] = server
 * parv[10] = serviceid
 * parv[11] = IP
 * parv[12] = ircname
 */

int
m_client (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p, *uplink;
  char nick[NICKLEN + 2];
  ts_val newts = 0;
  int sameuser = 0, samenick = 0;
  char *mptr;

  /*
   * Only Client Capable servers should ever send us CLIENT.
   */
  if (!IsServer (source_p) && !IsClientCapable (source_p))
  {
    sendto_one (source_p, err_str (ERR_NONICKNAMEGIVEN), me.name, parv[0]);
    return 0;
  }

  if (parc < 13)
  {
    /*
     * We got the wrong number of parameters from a server.
     * Tell staff and reject it.
     */
    ircstp->is_kill++;
    sendto_realops_lev
      (DEBUG_LEV,
       "Ignoring bad CLIENT with wrong number of parameters: %s(%s@%s) [%s] [%s] on %s (from %s)",
       parv[1], (parc >= 7) ? parv[6] : "-", (parc >= 8) ? parv[7] : "-",
       (parc >= 9) ? parv[8] : "-", (parc >= 12) ? parv[11] : "-",
       (parc >= 10) ? parv[9] : "-", parv[0]);

    sendto_one (client_p,
                ":%s KILL %s :%s (Bad CLIENT with wrong number of parameters)",
                me.name, parv[1], me.name);
    return 0;
  }

  if ((!strchr (parv[7], '.')) && (!strchr (parv[7], ':')))
  {
    /*
     * There isn't a single dot in the hostname
     *
     */
    ircstp->is_kill++;
    sendto_realops_lev (DEBUG_LEV,
                        "Ignoring bad CLIENT with invalid hostname: %s[%s@%s] on %s (from %s)",
                        parv[0], parv[6], parv[7], parv[9], parv[0]);

    sendto_one (client_p,
                ":%s KILL %s :%s (Bad CLIENT with invalid hostname)",
                me.name, parv[1], me.name);
    return 0;
  }
  newts = atol (parv[3]);
  strncpyzt (nick, parv[1], NICKLEN + 1);
  /*
   * if do_nick_name() returns a null name OR if the server sent a
   * nick name and do_nick_name() changed it in some way (due to rules
   * of nick creation) then reject it. If from a server and we reject
   * it, and KILL it. -avalon 4/4/92
   */
  if (do_nick_name (nick) == 0 || strcmp (nick, parv[1]))
  {
    sendto_one (source_p, err_str (ERR_ERRONEUSNICKNAME),
                me.name, parv[0], parv[1], "Erroneous Nickname", "N/A");
    ircstp->is_kill++;
    sendto_realops_lev (DEBUG_LEV, "Bad Nick: %s in CLIENT From: %s %s",
                        parv[1], parv[0], get_client_name (client_p, FALSE));
    sendto_one (client_p, ":%s KILL %s :%s (Bad Nick in CLIENT)", me.name,
                parv[1], me.name);
    if (source_p != client_p)
    {                           /* bad nick change */
      sendto_serv_butone (client_p,
                          ":%s KILL %s :%s (Bad Nick in CLIENT)", me.name,
                          parv[0], me.name);
      source_p->flags |= FLAGS_KILLED;
      return exit_client (client_p, source_p, &me, "BadNick");
    }
    return 0;
  }
  /*
   * Check against nick name collisions.
   *
   * Put this 'if' here so that the nesting goes nicely on the screen
   * :) We check against server name list before determining if the
   * nickname is present in the nicklist (due to the way the below
   * for loop is constructed). -avalon
   */
  do
  {
    if ((target_p = find_server (nick, NULL)))
      if (MyConnect (source_p))
      {
        sendto_one (source_p, err_str (ERR_NICKNAMEINUSE), me.name,
                    BadPtr (parv[0]) ? "*" : parv[0], nick);
        return 0;
      }

    /*
     * target_p already has result from find_server
     * Well. unless we have a capricious server on the net, a nick can
     * never be the same as a server name - Dianora
     * That's not the only case; maybe someone broke do_nick_name
     * or changed it so they could use "." in nicks on their network
     * - sedition
     */

    if (target_p)
    {
      /*
       * We have a nickname trying to use the same name as a
       * server. Send out a nick collision KILL to remove the
       * nickname. As long as only a KILL is sent out, there is no
       * danger of the server being disconnected.  Ultimate way to
       * jupiter a nick ? >;-). -avalon
       */
      sendto_realops_lev (SKILL_LEV, "Nick collision on %s", source_p->name);
      ircstp->is_kill++;
      sendto_one (client_p, ":%s KILL %s :%s (Nick Collision)", me.name,
                  source_p->name, me.name);
      source_p->flags |= FLAGS_KILLED;
      return exit_client (client_p, source_p, &me, "Nick/Server collision");
    }

    if (!(target_p = find_client (nick, NULL)))
    {
      break;
    }

    /*
     * If the older one is "non-person", the new entry is just
     * allowed to overwrite it. Just silently drop non-person, and
     * proceed with the nick. This should take care of the "dormant
     * nick" way of generating collisions...
     */
    if (IsUnknown (target_p))
    {
      if (MyConnect (target_p))
      {
        exit_client (NULL, target_p, &me, "Overridden");
        break;
      }
      else if (!(target_p->user))
      {
        sendto_realops_lev (SKILL_LEV, "Nick Collision on %s", parv[1]);
        sendto_serv_butone (NULL, ":%s KILL %s :%s (Nick Collission)",
                            me.name, target_p->name, me.name);
        target_p->flags |= FLAGS_KILLED;
        /* Having no USER struct should be ok... */
        return exit_client (client_p, target_p, &me,
                            "Got TS NICK before Non-TS USER");
      }
    }


    /*
     * The same nick is registered for different users by different server.
     * This is either a race condition (two users coming online about
     * same time, or net reconnecting) or just two net fragments
     * becoming joined and having same nicks in use. We cannot have
     * TWO users with same nick--purge this NICK from the system with
     * a KILL... >;)
     *
     * Changed to something reasonable like IsServer(source_p) (true if
     * "NICK new", false if ":old NICK new") -orabidoo
     */

    if (IsServer (source_p))
    {
      /*
       * A new NICK being introduced by a neighbouring server (e.g.
       * message type "NICK new" received)
       */
      if (!newts || !target_p->tsinfo || (newts == target_p->tsinfo))
      {
        sendto_realops_lev (SKILL_LEV, "Nick collision on %s", parv[1]);
        ircstp->is_kill++;
        sendto_one (target_p, err_str (ERR_NICKCOLLISION),
                    me.name, target_p->name, target_p->name);
        sendto_serv_butone (NULL, ":%s KILL %s :%s (Nick Collission)",
                            me.name, target_p->name, me.name);
        target_p->flags |= FLAGS_KILLED;
        return exit_client (client_p, target_p, &me, "Nick collision");
      }
      else
      {
        /* if the users are the same (loaded a client on a different server)
         * and the new users ts is older, or the users are different and the
         * new users ts is newer, collide the new client,
         */
        sameuser = ((target_p->user) &&
                    (irccmp (target_p->user->username, parv[5]) == 0) &&
                    (irccmp (target_p->user->host, parv[6]) == 0));
        if ((sameuser && newts < target_p->tsinfo) ||
            (!sameuser && newts > target_p->tsinfo))
        {
          return 0;
        }
        else
        {
          sendto_realops_lev (SKILL_LEV, "Nick collision on %s", parv[1]);
          ircstp->is_kill++;
          sendto_one (target_p, err_str (ERR_NICKCOLLISION),
                      me.name, target_p->name, target_p->name);
          sendto_serv_butone (source_p,
                              ":%s KILL %s :%s (Nick Collision)",
                              me.name, target_p->name, me.name);
          target_p->flags |= FLAGS_KILLED;
          (void) exit_client (client_p, target_p, &me, "Nick collision");
          break;
        }
      }
    }
  }
  while (0);
  if (IsServer (source_p))
  {
    int flag;

    uplink = find_server (parv[9], NULL);
    if (!uplink)
    {
      /* if we can't find the server this nick is on,
       * complain loudly and ignore it. - lucas */
      sendto_realops ("Rjecting CLIENT %s on UNKNOWN server %s", nick,
                      parv[9]);
      return 0;
    }
    source_p = make_client (client_p, uplink);
    /* If this is on a U: lined server, it's a U: lined client. */
    if (IsULine (uplink))
    {
      source_p->flags |= FLAGS_ULINE;
    }
    add_client_to_list (source_p);
    if (parc > 2)
      source_p->hopcount = atoi (parv[2]);
    {
      if (newts)
      {
        source_p->tsinfo = newts;
      }
      else
      {
        newts = source_p->tsinfo = (ts_val) timeofday;
        ts_warn ("Remote nick %s introduced without a TS", nick);
      }
    }
    /*
     * copy the nick in place
     */
    (void) strcpy (source_p->name, nick);
    (void) add_to_client_hash_table (nick, source_p);

    /* parse the usermodes intelligently */
    mptr = &parv[4][1];
    while (*mptr)
    {
      if ((flag = user_mode_table[(unsigned char) *mptr]))
      {
        if (((flag == UMODE_o) || (flag == UMODE_O)) && !IsULine (source_p))
          Count.oper++;
        source_p->umode |= flag & SEND_UMODES;
      }
      mptr++;
    }

    /* parse the servemodes intelligently */
    mptr = &parv[5][1];
    while (*mptr)
    {
      if ((flag = server_mode_table[(unsigned char) *mptr]))
      {
        source_p->smode |= flag & SEND_SMODES;
      }
      mptr++;
    }

    if ((strchr (parv[11], ':')) && (!strchr (parv[11], '.')))
    {
      SetIPV6Client (source_p);

      do_user (nick, client_p, source_p,
               parv[6], parv[7], parv[9],
               strtoul (parv[10], NULL, 0), (unsigned char *) parv[11],
               parv[12], parv[8]);
    }
    else
    {
      do_user (nick, client_p, source_p,
               parv[6], parv[7], parv[9],
               strtoul (parv[10], NULL, 0),
               (unsigned char *) strtoul (parv[11], NULL, 0), parv[12],
               parv[8]);
    }

    /*
     * If the introduced nickname is not part of a server burst, generate
     * a remote connection notice for it.
     */
    if (IsEOBurst (uplink) && IsEOBurst (client_p))
    {

      sendto_globalconnectnotice
        ("from %s: Client connecting: %s (%s@%s) [%s]%s [%s]",
         uplink->name, nick, parv[6], parv[7], source_p->hostip,
         IsSSLClient (source_p) ? " (SSL)" : "", parv[12]);
    }
    return 0;
  }

  /* Finally set new nick name. */

  (void) strcpy (source_p->name, nick);
  (void) add_to_client_hash_table (nick, source_p);
  if (IsPerson (source_p) && !samenick)
    hash_check_watch (source_p, RPL_LOGON);
  return 0;
}
