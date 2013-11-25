/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_services.c
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
 *  $Id: s_services.c 985 2007-02-04 20:51:36Z shadowmaster $
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

/* Externally defined stuffs */
extern int user_mode_table[];
extern unsigned long my_rand ();

/* internally defined stuffs */

static int channel_svsmode (aClient *, aClient *, int, char *[]);



/*
 * the services aliases.
 *
 * NICKSERV - /ns (/NickServ)
 * CHANSERV - /cs (/ChanServ)
 * OPERSERV - /os (/OperServ)
 * MEMOSERV - /ms (/MemoServ)
 * BOTSERV  - /bs (/BotServ)
 * STATSERV - /ss (/StatServ)
 * ROOTSERV - /rs (/RootServ)
 * HOSTSERV - /hs (/HostServ) 
 * SERVICES - /services
 * IDENTIFY - /identify
 */

int
m_cs (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_CHANSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0]);
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (CHANSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  CHANSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], CHANSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (CHANSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                CHANSERV_NAME, SERVICES_SERVER, parv[1]);
  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], CHANSERV_NAME);
  return 0;
}

/*
 * m_ns
 */
int
m_ns (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_NICKSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "NickServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (NICKSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  NICKSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], NICKSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (NICKSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                NICKSERV_NAME, SERVICES_SERVER, parv[1]);
  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], NICKSERV_NAME);
  return 0;
}

/*
 * m_ms 
 */
int
m_ms (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_MEMOSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "MemoServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (MEMOSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  MEMOSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], MEMOSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (MEMOSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                MEMOSERV_NAME, SERVICES_SERVER, parv[1]);
  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], MEMOSERV_NAME);
  return 0;
}

/*
 * m_os
 */
int
m_os (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_OPERSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "OperServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (OPERSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  OPERSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], OPERSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (OPERSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                OPERSERV_NAME, SERVICES_SERVER, parv[1]);

  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], OPERSERV_NAME);
  return 0;
}

/*
 * m_ss
 */
int
m_ss (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_STATSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "StatServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (STATSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  STATSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], STATSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (STATSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                STATSERV_NAME, STATSERV_SERVER, parv[1]);

  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], STATSERV_NAME);
  return 0;
}

/*
 * m_bs
 */
int
m_bs (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_BOTSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "BotServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (BOTSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  BOTSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], BOTSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (BOTSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                BOTSERV_NAME, SERVICES_SERVER, parv[1]);
  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], BOTSERV_NAME);
  return 0;
}

/*
 * m_rs
 */
int
m_rs (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_ROOTSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "RootServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (ROOTSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  ROOTSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], ROOTSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (ROOTSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                ROOTSERV_NAME, SERVICES_SERVER, parv[1]);
  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], ROOTSERV_NAME);
  return 0;
}

/*
 * m_rs
 */
int
m_hs (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (!ENABLE_HOSTSERV_ALIAS)
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0],
                "HostServ");
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;
  if (parc < 2 || *parv[1] == '\0')
  {
    if ((target_p = find_person (HOSTSERV_NAME, NULL)))
      sendto_one (target_p, ":%s PRIVMSG %s@%s :help", parv[0],
                  HOSTSERV_NAME, SERVICES_SERVER);
    else
      sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                  parv[0], HOSTSERV_NAME);
    return -1;
  }
  if ((target_p = find_person (HOSTSERV_NAME, NULL)))
    sendto_one (target_p, ":%s PRIVMSG %s@%s :%s", parv[0],
                HOSTSERV_NAME, SERVICES_SERVER, parv[1]);
  else
    sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                parv[0], HOSTSERV_NAME);
  return 0;
}


/*
 * m_services -- see df465+taz
 */
int
m_services (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  char *tmps;

  if ((!ENABLE_NICKSERV_ALIAS)
      && (!ENABLE_CHANSERV_ALIAS)
      && (!ENABLE_MEMOSERV_ALIAS) && (!ENABLE_BOTSERV_ALIAS))
  {
    sendto_one (source_p, err_str (ERR_UNKNOWNCOMMAND), me.name, parv[0]);
    return -1;
  }
  if (check_registered_user (source_p))
    return 0;

  if (parc < 2 || *parv[1] == '\0')
  {
    if (ENABLE_NICKSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For NickServ "
                  "help use: /nickserv help", NICKSERV_NAME,
                  SERVICES_SERVER, source_p->name);
    if (ENABLE_CHANSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For ChanServ "
                  "help use: /chanserv help", CHANSERV_NAME,
                  SERVICES_SERVER, source_p->name);
    if (ENABLE_MEMOSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For MemoServ "
                  "help use: /memoserv help", MEMOSERV_NAME,
                  SERVICES_SERVER, source_p->name);
    if (ENABLE_BOTSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For BotServ "
                  "help use: /botserv help", BOTSERV_NAME, SERVICES_SERVER,
                  source_p->name);
    return -1;
  }
  if ((strlen (parv[1]) >= 4) && (ircncmp (parv[1], "help", 4) == 0))
  {
    if (ENABLE_NICKSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For NickServ "
                  "help use: /nickserv help", NICKSERV_NAME,
                  SERVICES_SERVER, source_p->name);
    if (ENABLE_CHANSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For ChanServ "
                  "help use: /chanserv help", CHANSERV_NAME,
                  SERVICES_SERVER, source_p->name);
    if (ENABLE_MEMOSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For MemoServ "
                  "help use: /memoserv help", MEMOSERV_NAME,
                  SERVICES_SERVER, source_p->name);
    if (ENABLE_BOTSERV_ALIAS)
      sendto_one (source_p, ":%s!services@%s NOTICE %s :For BotServ "
                  "help use: /botserv help", BOTSERV_NAME, SERVICES_SERVER,
                  source_p->name);
    return 0;
  }
  if ((tmps = (char *) strchr (parv[1], ' ')))
  {
    for (; *tmps == ' '; tmps++);       /* er.. before this for loop, the next
                                         * comparison would always compare '#' with ' '.. oops. - lucas
                                         */
    if (*tmps == '#')
      return m_cs (client_p, source_p, parc, parv);
    else
      return m_ns (client_p, source_p, parc, parv);
  }
  return m_ns (client_p, source_p, parc, parv);
}

/*
 * m_identify  df465+taz
 */
int
m_identify (client_p, source_p, parc, parv)
     aClient *client_p, *source_p;
     int parc;
     char *parv[];
{
  aClient *target_p;

  if (check_registered_user (source_p))
    return 0;

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NOTEXTTOSEND), me.name, parv[0]);
    return -1;
  }
  if (*parv[1])
  {
    if ((*parv[1] == '#') && ((char *) strchr (parv[1], ' ')))
    {
      if ((target_p = find_person (CHANSERV_NAME, NULL)))
        sendto_one (target_p, ":%s PRIVMSG %s@%s :IDENTIFY %s ", parv[0],
                    CHANSERV_NAME, SERVICES_SERVER, parv[1]);
      else
        sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                    parv[0], CHANSERV_NAME);
    }
    else
    {
      if ((target_p = find_person (NICKSERV_NAME, NULL)))
        sendto_one (target_p, ":%s PRIVMSG %s@%s :IDENTIFY %s", parv[0],
                    NICKSERV_NAME, SERVICES_SERVER, parv[1]);
      else
        sendto_one (source_p, err_str (ERR_SERVICESDOWN), me.name,
                    parv[0], NICKSERV_NAME);
    }
  }
  return 0;
}

/* s_svsnick - Pretty straight forward.  Mostly straight outta df
 *  - Raistlin
 * parv[0] = sender
 * parv[1] = old nickname
 * parv[2] = new nickname
 * parv[3] = timestamp
 */
int
m_svsnick (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p, *oclient_p;
  char newnick[NICKLEN + 1];

  if (!IsULine (source_p) || parc < 4 || (strlen (parv[2]) > NICKLEN))
    return 0;

  if (hunt_server (client_p, source_p, ":%s SVSNICK %s %s :%s", 1, parc, parv)
      != HUNTED_ISME)
    return 0;

  /* can't find them? oh well. */
  if ((target_p = find_person (parv[1], NULL)) == NULL)
    return 0;

  strncpyzt (newnick, parv[2], NICKLEN + 1);

  /* does the nick we're changing them to already exist? */
  /* Try to make a unique nickname */
  if ((oclient_p = find_client (newnick, NULL)) != NULL)
  {
    char servprefix[HOSTLEN + 1], *pptr;
    int tries = 0, nprefix;

    strncpyzt (servprefix, me.name, NICKLEN + 1);
    pptr = strchr (servprefix, '.');
    if (pptr)
      *pptr = '\0';

    do
    {
      nprefix = my_rand () % 999;
      ircsnprintf (newnick, NICKLEN, "%s-%d[%s]", parv[2], nprefix,
                   servprefix);
      tries++;
    }
    while (((oclient_p = find_client (newnick, NULL)) != NULL)
           && (tries < 10));

    /* well, we tried.. */
    if (oclient_p)
    {
      if (IsUnknown (oclient_p))
        return exit_client (oclient_p, oclient_p, &me, "SVSNICK Override");
      else
        return exit_client (target_p, target_p, &me, "SVSNICK Collide");
    }
  }

  target_p->umode &= ~UMODE_r;
  target_p->tsinfo = atoi (parv[3]);
#ifdef ANTI_NICK_FLOOD
  target_p->last_nick_change = atoi (parv[3]);
#endif
  sendto_common_channels (target_p, ":%s NICK :%s", parv[1], newnick);
  add_history (target_p, 1);
  sendto_serv_butone (NULL, ":%s NICK %s :%d", parv[1], newnick,
                      target_p->tsinfo);
  if (target_p->name[0])
    del_from_client_hash_table (target_p->name, target_p);
  strcpy (target_p->name, newnick);
  add_to_client_hash_table (target_p->name, target_p);

  return 0;
}

/* channel_svsmode:
 * parv[0] sender
 * parv[1] channel
 * parv[2] modes
 * parv[3] nick
 * parv[4] nickts
 * currently, only a mode of -b is supported.
 * services should use MODE for regular channel modes.
 * 2/5/00 lucas
 * preconditions: parc >= 3, source_p is ulined
 */
int
channel_svsmode (aClient * client_p, aClient * source_p, int parc,
                 char *parv[])
{
  aChannel *channel_p;
  aClient *target_p = NULL;
  char *m, *nick = NULL;
  char change = '+';
  ts_val nickts = 0;
  int mysendmsg = 1;

  if (!(channel_p = find_channel (parv[1], NULL)))
  {
    return 0;
  }

  if (parc >= 4)
  {
    nick = parv[3];
    if (parc > 4)
      nickts = atol (parv[4]);
  }

  if (nick)
  {
    target_p = find_person (nick, NULL);
    if (!target_p)
    {
      return 0;
    }

    if (nickts && target_p->tsinfo != nickts)
    {
      sendto_serv_butone (&me,
                          ":%s GLOBOPS :Server %s trying to SVSMODE user %s on channel %s, but TimeStamps mismatch (local: %d remote: %d)",
                          me.name, source_p->name, nick, parv[1],
                          target_p->tsinfo, nickts);
      send_globops
        ("From %s: Server %s trying to SVSMODE user %s on channel %s, but TimeStamps mismatch (local: %d remote: %d)",
         me.name, source_p->name, nick, parv[1], target_p->tsinfo, nickts);
      return 0;
    }
  }

  for (m = parv[2]; *m; m++)
    switch (*m)
    {
       case '+':
       case '-':
         change = *m;
         break;

       case 'b':
         if (nick && MyClient (target_p) && change == '-')
         {
           remove_matching_bans (channel_p, target_p, &me);
           mysendmsg--;
         }
         break;


       case 'e':
         if (nick && MyClient (target_p) && change == '-')
         {
           remove_matching_exempts (channel_p, target_p, &me);
           mysendmsg--;
         }

       default:
         mysendmsg++;
         break;
    }

  if (!mysendmsg)
    return 0;

  if (nick)
    sendto_serv_butone (client_p, ":%s SVSMODE %s %s %s %ld", parv[0],
                        parv[1], parv[2], nick, target_p->tsinfo);
  else
    sendto_serv_butone (client_p, ":%s SVSMODE %s %s", parv[0], parv[1],
                        parv[2]);

  return 0;
}

/* m_svsmode - df function integrated
 *  - Raistlin
 * -- Behaviour changed - Epi (11/30/99)
 * parv[0] - sender
 * parv[1] - nick
 * parv[2] - TS (or mode, depending on svs version)
 * parv[3] - mode (or services id if old svs version)
 * parv[4] - optional arguement (services id)
 */
int
m_svsmode (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int flag, what, oldumode;
  char *m, *modes, *myoptarg;
  aClient *target_p;
  ts_val ts = 0;

  if (!IsServer (source_p))
  {
    if (MyClient (source_p))
    {
      sendto_one (source_p, err_str (ERR_SERVERONLY), me.name, parv[0]);
    }
    return 0;
  }

  if (!IsULine (source_p))
  {
    sendto_serv_butone (&me,
                        ":%s GLOBOPS :Non-ULined server %s trying to SVSMODE!",
                        me.name, source_p->name);
    send_globops ("From %s: Non-ULined server %s trying to SVSMODE!",
                  me.name, source_p->name);
    return 0;
  }

  if (parc < 3)
  {
    sendto_serv_butone (&me,
                        ":%s GLOBOPS :Broken server %s trying to SVSMODE with too few parameters!",
                        me.name, source_p->name);
    send_globops
      ("From %s: Broken server %s trying to SVSMODE with too few parameters!",
       me.name, source_p->name);
    return 0;
  }

  if (parv[1][0] == '#')
  {
    return channel_svsmode (client_p, source_p, parc, parv);
  }

  if ((parc >= 4) && ((parv[3][0] == '+') || (parv[3][0] == '-')))
  {
    ts = atol (parv[2]);
    modes = parv[3];
    myoptarg = (parc > 4) ? parv[4] : NULL;
  }
  else
  {
    modes = parv[2];
    myoptarg = (parc > 3) ? parv[3] : NULL;
  }

  if (!(target_p = find_person (parv[1], NULL)))
  {
    return 0;
  }

  if (ts && (ts != target_p->tsinfo))
  {
    /*
       sendto_serv_butone (&me,
       ":%s GLOBOPS :Server %s trying to SVSMODE user %s, but TimeStamps mismatch (local: %d remote: %d)",
       me.name, source_p->name, parv[1], target_p->tsinfo,
       ts);
       send_globops
       ("From %s: Server %s trying to SVSMODE user %s, but TimeStamps mismatch (local: %d remote: %d)",
       me.name, source_p->name, parv[1], target_p->tsinfo, ts);
     */
    return 0;
  }

  what = MODE_ADD;
  oldumode = target_p->umode;
  for (m = modes; *m; m++)
    switch (*m)
    {
       case '+':
         what = MODE_ADD;
         break;
       case '-':
         what = MODE_DEL;
         break;
       case ' ':
       case '\n':
       case '\r':
       case '\t':
         break;
       case 'd':
         if (myoptarg && MyIsDigit (*myoptarg))
           target_p->user->servicestamp = strtoul (myoptarg, NULL, 0);
         break;
       default:
         if ((flag = user_mode_table[(unsigned char) *m]))
         {
           if (what == MODE_ADD)
           {
             target_p->umode |= flag;
           }
           else
           {
             target_p->umode &= ~flag;
           }
           /* If this SVSMODE removed their oper status,
            * remove them from the oper fd list */
           if (MyConnect (target_p) && what == MODE_DEL &&
               (flag == UMODE_o || flag == UMODE_O) && !IsAnOper (target_p))
           {
             remove_from_list (&oper_list, target_p, NULL);
           }
         }
         break;
    }

  if (myoptarg)
    sendto_serv_butone (client_p, ":%s SVSMODE %s %ld %s %s",
                        parv[0], parv[1], target_p->tsinfo, modes, myoptarg);
  else
    sendto_serv_butone (client_p, ":%s SVSMODE %s %ld %s",
                        parv[0], parv[1], target_p->tsinfo, modes);

  if (MyClient (target_p))
  {
    char buf[BUFSIZE];
    send_umode (target_p, target_p, oldumode, ALL_UMODES, buf);
  }

  return 0;
}

/*
 * m_svsjoin()
 *
 * Originally by Lamego - Wed Jul 21 20:04:48 1999
 *
 * parv[0] - sender
 * parv[1] - nick to make join
 * parv[2] - channel(s) to join
 *
 * Allows ULined clients to forcibly join a user to a channel.
 */
int
m_svsjoin (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  if (!IsULine (source_p))
  {
    return 0;
  }

  if (parc != 3 || !(target_p = find_person (parv[1], NULL)))
  {
    return 0;
  }

  if (MyClient (target_p))
  {
    parv[0] = parv[1];
    parv[1] = parv[2];
    m_join (target_p, target_p, 2, parv);
  }
  else
  {
    sendto_serv_butone (client_p, ":%s SVSJOIN %s %s", parv[0], parv[1],
                        parv[2]);
  }

  return 0;
}

/*
 * m_svspart() - Lamego - Wed Jul 21 20:04:48 1999
 *
 *	parv[0] - sender
 *	parv[1] - nick to make part
 *	parv[2] - channel(s) to part
 */
int
m_svspart (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  if (!IsULine (source_p))
  {
    return 0;
  }

  if (parc != 3 || !(target_p = find_person (parv[1], NULL)))
  {
    return 0;
  }

  if (MyClient (target_p))
  {
    parv[0] = parv[1];
    parv[1] = parv[2];
    m_part (target_p, target_p, 2, parv);
  }
  else
  {
    sendto_serv_butone (client_p, ":%s SVSPART %s %s", parv[0], parv[1],
                        parv[2]);
  }
  return 0;
}
