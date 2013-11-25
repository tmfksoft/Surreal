/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_debug.c
 *
 *  Copyright (C) 1990 Jarkko Oikarinen and University of Oulu, Computing Center
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
 *  $Id: s_debug.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "version.h"
#include "blalloc.h"
#include "h.h"
#include "whowas.h"
/*
 * Option string.  Must be before #ifdef DEBUGMODE.
 */
/*
 * I took out a lot of options that really aren't optional anymore,
 * note also that at the end we denote what our release status is
 */
char serveropts[] = {
#ifdef	CMDLINE_CONFIG
  'C',
#endif
#ifdef        DO_ID
  'd',
#endif

/*
#ifdef	HUB
   'H',
#endif
*/
#ifdef	SHOW_INVISIBLE_LUSERS
  'i',
#endif
#ifndef	NO_DEFAULT_INVISIBLE
  'I',
#endif
#ifdef	CRYPT_OPER_PASSWORD
  'p',
#endif
#ifdef	IRCII_KLUDGE
  'u',
#endif
#ifdef	USE_SYSLOG
  'Y',
#endif
#ifdef INET6
  '6',
#endif
  '(',
  'T',
  'S',
#ifdef TS_CURRENT
  '0' + TS_CURRENT,
#endif
  /*
   * th+hybrid servers ONLY do TS
   */
  /*
   * th+hybrid servers ALWAYS do TS_WARNINGS
   */
  'o',
  'w',
  ')',
#ifdef USE_SSL
  '-',
  'S',
  'S',
  'L',
#endif
  '\0'
};

#include "numeric.h"
#include "common.h"
#include "sys.h"
/*
 * #include "whowas.h" 
 */
#include "hash.h"
#ifndef _WIN32
# include <sys/file.h>
# include <sys/resource.h>
#endif
#if !defined(ULTRIX) && !defined(SGI) && !defined(sequent) && \
    !defined(__convex__) && !defined(_WIN32)
#include <sys/param.h>
#endif
#if defined( HAVE_GETRUSAGE )
#ifdef SOL20
#include <sys/time.h>
/*
 * #  include <sys/rusage.h>
 */
#endif
#else
#if defined( HAVE_TIMES )
#include <sys/times.h>
#endif
#endif /*
        * HAVE_GETRUSAGE 
        */
#include "h.h"
#include "userban.h"

#ifndef ssize_t
#define ssize_t unsigned int
#endif

/*
 * extern char *sys_errlist[]; 
 */

/*
 * #ifdef DEBUGMODE 
 */
#if defined(DNS_DEBUG) || defined(DEBUGMODE)
static char debugbuf[1024];

void
debug (int level, char *pattern, ...)
{
  va_list vl;
  int err = errno;

  va_start (vl, pattern);
  (void) vsprintf (debugbuf, pattern, vl);
  va_end (vl);

#ifdef USE_SYSLOG
  if (level == DEBUG_ERROR)
    syslog (LOG_ERR, "%s", debugbuf);
#endif

  if ((debuglevel >= 0) && (level <= debuglevel))
  {

    if (local[2])
    {
      local[2]->sendM++;
      local[2]->sendB += strlen (debugbuf);
    }
    ilog (LOGF_DEBUG, "Level %d: %s", level, debugbuf);
  }
  errno = err;
}

/*
 * This is part of the STATS replies. There is no offical numeric for
 * this since this isnt an official command, in much the same way as
 * HASH isnt. It is also possible that some systems wont support this
 * call or have different field names for "struct rusage". -avalon
 */
void
send_usage (aClient * client_p, char *nick)
{
#ifndef _WIN32
#if defined( HAVE_GETRUSAGE )
  struct rusage rus;
  time_t secs, rup;

#ifdef	hz
#define hzz hz
#else
#ifdef HZ
#define hzz HZ
#else
  int hzz = 1;

#endif
#endif

  if (getrusage (RUSAGE_SELF, &rus) == -1)
  {
    sendto_one (client_p, ":%s NOTICE %s :Getruseage error: %s.",
                me.name, nick,
#ifndef OS_SOLARIS              /* sys_errlist has been removed in Solaris 7. strerror is the replacement - Againaway */
                sys_errlist[errno]);
#else
                strerror);
#endif
    return;
  }
  secs = rus.ru_utime.tv_sec + rus.ru_stime.tv_sec;
  rup = timeofday - me.since;
  if (secs == 0)
    secs = 1;

  sendto_one (client_p,
              ":%s %d %s :CPU Secs %d:%d User %d:%d System %d:%d",
              me.name, RPL_STATSDEBUG, nick, secs / 60, secs % 60,
              rus.ru_utime.tv_sec / 60, rus.ru_utime.tv_sec % 60,
              rus.ru_stime.tv_sec / 60, rus.ru_stime.tv_sec % 60);
  sendto_one (client_p, ":%s %d %s :RSS %d ShMem %d Data %d Stack %d",
              me.name, RPL_STATSDEBUG, nick, rus.ru_maxrss,
              rus.ru_ixrss / (rup * hzz), rus.ru_idrss / (rup * hzz),
              rus.ru_isrss / (rup * hzz));
  sendto_one (client_p, ":%s %d %s :Swaps %d Reclaims %d Faults %d",
              me.name, RPL_STATSDEBUG, nick, rus.ru_nswap,
              rus.ru_minflt, rus.ru_majflt);
  sendto_one (client_p, ":%s %d %s :Block in %d out %d",
              me.name, RPL_STATSDEBUG, nick, rus.ru_inblock, rus.ru_oublock);
  sendto_one (client_p, ":%s %d %s :Msg Rcv %d Send %d",
              me.name, RPL_STATSDEBUG, nick, rus.ru_msgrcv, rus.ru_msgsnd);
  sendto_one (client_p, ":%s %d %s :Signals %d Context Vol. %d Invol %d",
              me.name, RPL_STATSDEBUG, nick, rus.ru_nsignals,
              rus.ru_nvcsw, rus.ru_nivcsw);
#else
#if defined( HAVE_TIMES )
  struct tms tmsbuf;
  time_t secs, mins;
  int hzz = 1, ticpermin;
  int umin, smin, usec, ssec;

  ticpermin = hzz * 60;

  umin = tmsbuf.tms_utime / ticpermin;
  usec = (tmsbuf.tms_utime % ticpermin) / (float) hzz;
  smin = tmsbuf.tms_stime / ticpermin;
  ssec = (tmsbuf.tms_stime % ticpermin) / (float) hzz;
  secs = usec + ssec;
  mins = (secs / 60) + umin + smin;
  secs %= hzz;

  if (times (&tmsbuf) == -1)
  {
    sendto_one (client_p, ":%s %d %s :times(2) error: %s.",
                me.name, RPL_STATSDEBUG, nick, strerror (errno));
    return;
  }
  secs = tmsbuf.tms_utime + tmsbuf.tms_stime;

  sendto_one (client_p,
              ":%s %d %s :CPU Secs %d:%d User %d:%d System %d:%d",
              me.name, RPL_STATSDEBUG, nick, mins, secs, umin, usec,
              smin, ssec);
#endif /*
        * HAVE_TIMES 
        */
#endif /*
        * HAVE_GETRUSAGE 
        */
  sendto_one (client_p, ":%s %d %s :Reads %d Writes %d",
              me.name, RPL_STATSDEBUG, nick, readcalls, writecalls);
  sendto_one (client_p, ":%s %d %s :DBUF alloc %d used %d",
              me.name, RPL_STATSDEBUG, nick, DBufCount, DBufUsedCount);
  sendto_one (client_p,
              ":%s %d %s :Writes:  <0 %d 0 %d <16 %d <32 %d <64 %d",
              me.name, RPL_STATSDEBUG, nick,
              writeb[0], writeb[1], writeb[2], writeb[3], writeb[4]);
  sendto_one (client_p,
              ":%s %d %s :<128 %d <256 %d <512 %d <1024 %d >1024 %d",
              me.name, RPL_STATSDEBUG, nick,
              writeb[5], writeb[6], writeb[7], writeb[8], writeb[9]);
#else
  sendto_one (client_p,
              ":%s %d %s :Not supported on the win32 platform currently",
              me.name, RPL_STATSDEBUG, nick);
#endif
  return;
}
#endif

void
count_memory (aClient * client_p, char *nick)
{
  extern aChannel *channel;
  extern aClass *classes;
  extern aConfItem *conf;

  extern BlockHeap *free_local_aClients;
  extern BlockHeap *free_Links;
  extern BlockHeap *free_DLinks;
  extern BlockHeap *free_remote_aClients;
  extern BlockHeap *free_anUsers;
  extern BlockHeap *free_channels;
  extern BlockHeap *free_chanMembers;
#ifdef FLUD
  extern BlockHeap *free_fludbots;
#endif

  extern aMotd *motd;
  extern aMotd *shortmotd;
  extern aMotd *helpfile;

  extern int num_msg_trees;

  aClient *target_p;
  Link *mylink;
  chanMember *cm;
  aBan *bp;
  aChannel *channel_p;
  aConfItem *aconf;
  aClass *cltmp;
  aMotd *amo;

  int lc = 0;                   /*
                                 * local clients 
                                 */
  int ch = 0;                   /*

                                 * channels 
                                 */
  int lcc = 0;                  /*
                                 * local client conf links 
                                 */
  int rc = 0;                   /*
                                 * remote clients 
                                 */
  int us = 0;                   /*
                                 * user structs 
                                 */
  int chu = 0;                  /*
                                 * channel users 
                                 */
  int chi = 0;                  /*
                                 * channel invites 
                                 */
  int chb = 0;                  /*
                                 * channel bans 
                                 */
  int wwu = 0;                  /*
                                 * whowas users 
                                 */
  int cl = 0;                   /*
                                 * classes 
                                 */
  int co = 0;                   /*
                                 * conf lines 
                                 */
  int usi = 0;                  /*
                                 * users invited 
                                 */
  int usc = 0;                  /*
                                 * users in channels 
                                 */
  int usdm = 0;                 /* dccallow local */
  int usdr = 0;                 /* dccallow remote */
  int uss = 0;                  /* silenced users */
  int aw = 0;                   /*
                                 * aways set 
                                 */

  int number_ips_stored;        /*
                                 * number of ip addresses hashed 
                                 */
  int number_servers_cached;    /*
                                 * number of servers cached by
                                 * * scache 
                                 */

  u_long chbm = 0;              /*
                                 * memory used by channel bans 
                                 */
  u_long lcm = 0;               /*
                                 * memory used by local clients 
                                 */
  u_long rcm = 0;               /*
                                 * memory used by remote clients 
                                 */
  u_long awm = 0;               /*
                                 * memory used by aways 
                                 */
  u_long wwm = 0;               /*
                                 * whowas array memory used 
                                 */
  u_long com = 0;               /*
                                 * memory used by conf lines 
                                 */
  size_t db = 0, db2 = 0;       /*
                                 * memory used by dbufs 
                                 */
  u_long rm = 0;                /*
                                 * res memory used 
                                 */
  u_long mem_servers_cached;    /*
                                 * memory used by scache 
                                 */
  u_long mem_ips_stored;        /*
                                 * memory used by ip address hash 
                                 */

  u_long totcl = 0;
  u_long totch = 0;
  u_long totww = 0;
  u_long totmisc = 0;
  u_long tothash = 0;
  u_long totuban = 0;
  u_long tot = 0;

  int wlh = 0, wle = 0;         /* watch headers/entries */
  u_long wlhm = 0;              /* memory used by watch */

  int lcalloc = 0;              /* local clients allocated */
  int rcalloc = 0;              /* remote clients allocated */
  int useralloc = 0;            /* allocated users */
  int linkalloc = 0;            /* allocated links */
  int dlinkalloc = 0;           /* allocated dlinks */
  int totallinks = 0;           /* total links used */
  int chanalloc = 0;            /* total channels alloc'd */
  int cmemballoc = 0;
  u_long lcallocsz = 0, rcallocsz = 0;  /* size for stuff above */
  u_long userallocsz = 0, linkallocsz = 0, dlinkallocsz = 0, chanallocsz =
    0, cmemballocsz = 0;

  int fludalloc = 0;
  u_long fludallocsz = 0;
  int fludlink = 0;

  int motdlen = 0;

  int servn = 0;

  count_whowas_memory (&wwu, &wwm);     /*
                                         * no more away memory to count 
                                         */

  count_watch_memory (&wlh, &wlhm);
  for (target_p = client; target_p; target_p = target_p->next)
  {
    if (MyConnect (target_p))
    {
      lc++;
      wle += target_p->watches;
      for (mylink = target_p->confs; mylink; mylink = mylink->next)
        lcc++;
    }
    else
      rc++;

#ifdef FLUD
    for (mylink = target_p->fludees; mylink; mylink = mylink->next)
      fludlink++;
#endif
    if (target_p->serv)
    {
      servn++;
    }

    if (target_p->user)
    {
      us++;
      for (mylink = target_p->user->invited; mylink; mylink = mylink->next)
        usi++;
      for (mylink = target_p->user->channel; mylink; mylink = mylink->next)
        usc++;
      for (mylink = target_p->user->dccallow; mylink; mylink = mylink->next)
      {
        if (mylink->flags == DCC_LINK_ME)
          usdm++;
        else
          usdr++;
      }
      for (mylink = target_p->user->silence; mylink; mylink = mylink->next)
        uss++;
      if (target_p->user->away)
      {
        aw++;
        awm += (strlen (target_p->user->away) + 1);
      }
    }
  }

  lcm = lc * CLIENT_LOCAL_SIZE;
  rcm = rc * CLIENT_REMOTE_SIZE;

  for (channel_p = channel; channel_p; channel_p = channel_p->nextch)
  {
    ch++;

    for (cm = channel_p->members; cm; cm = cm->next)
      chu++;
    for (mylink = channel_p->invites; mylink; mylink = mylink->next)
      chi++;
    for (bp = channel_p->banlist; bp; bp = bp->next)
    {
      chb++;
      chbm += (strlen (bp->who) + strlen (bp->banstr) + 2 + sizeof (aBan));
    }
  }

  for (aconf = conf; aconf; aconf = aconf->next)
  {
    co++;
    com += aconf->host ? strlen (aconf->host) + 1 : 0;
    com += aconf->passwd ? strlen (aconf->passwd) + 1 : 0;
    com += aconf->name ? strlen (aconf->name) + 1 : 0;
    com += sizeof (aConfItem);
  }

  for (cltmp = classes; cltmp; cltmp = cltmp->next)
    cl++;

  for (amo = motd; amo; amo = amo->next)
    motdlen++;
  if (SHORT_MOTD == 1)
  {
    for (amo = shortmotd; amo; amo = amo->next)
      motdlen++;
  }

  for (amo = helpfile; amo; amo = amo->next)
    motdlen++;

  lcalloc =
    free_local_aClients->blocksAllocated * free_local_aClients->elemsPerBlock;
  lcallocsz = lcalloc * free_local_aClients->elemSize;

  rcalloc =
    free_remote_aClients->blocksAllocated *
    free_remote_aClients->elemsPerBlock;
  rcallocsz = rcalloc * free_remote_aClients->elemSize;

  useralloc = free_anUsers->blocksAllocated * free_anUsers->elemsPerBlock;
  userallocsz = useralloc * free_anUsers->elemSize;

  linkalloc = free_Links->blocksAllocated * free_Links->elemsPerBlock;
  linkallocsz = linkalloc * free_Links->elemSize;

  dlinkalloc = free_DLinks->blocksAllocated * free_DLinks->elemsPerBlock;
  dlinkallocsz = dlinkalloc * free_DLinks->elemSize;

  chanalloc = free_channels->blocksAllocated * free_channels->elemsPerBlock;
  chanallocsz = chanalloc * free_channels->elemSize;

  cmemballoc =
    free_chanMembers->blocksAllocated * free_chanMembers->elemsPerBlock;
  cmemballocsz = cmemballoc * free_chanMembers->elemSize;

#ifdef FLUD
  fludalloc = free_fludbots->blocksAllocated * free_fludbots->elemsPerBlock;
  fludallocsz = fludalloc * free_fludbots->elemSize;
#endif

  totallinks = lcc + usi + uss + usc + chi + wle + fludlink + usdm + usdr;

  sendto_one (client_p, ":%s %d %s :Memory Use Summary",
              me.name, RPL_STATSDEBUG, nick);
  sendto_one (client_p, ":%s %d %s :Client usage %d(%d) ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, lc + rc, lcm + rcm,
              lcalloc + rcalloc, lcallocsz + rcallocsz);
  sendto_one (client_p, ":%s %d %s :   Local %d(%d) ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, lc, lcm, lcalloc, lcallocsz);
  sendto_one (client_p, ":%s %d %s :   Remote %d(%d) ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, rc, rcm, rcalloc, rcallocsz);
  sendto_one (client_p, ":%s %d %s :Users %d(%d) ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, us, us * sizeof (anUser),
              useralloc, userallocsz);

  totcl = lcallocsz + rcallocsz + userallocsz;

  sendto_one (client_p, ":%s %d %s :Links %d(%d) ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, totallinks,
              totallinks * sizeof (Link), linkalloc, linkallocsz);
  sendto_one (client_p, ":%s %d %s :   UserInvites %d(%d) ChanInvites %d(%d)",
              me.name, RPL_STATSDEBUG, nick, usi, usi * sizeof (Link), chi,
              chi * sizeof (Link));
  sendto_one (client_p, ":%s %d %s :   UserChannels %d(%d)", me.name,
              RPL_STATSDEBUG, nick, usc, usc * sizeof (Link));
  sendto_one (client_p, ":%s %d %s :   DCCAllow Local %d(%d) Remote %d(%d)",
              me.name, RPL_STATSDEBUG, nick, usdm, usdm * sizeof (Link), usdr,
              usdr * sizeof (Link));
  sendto_one (client_p, ":%s %d %s :   WATCH entries %d(%d)", me.name,
              RPL_STATSDEBUG, nick, wle, wle * sizeof (Link));
  sendto_one (client_p, ":%s %d %s :   Attached confs %d(%d)", me.name,
              RPL_STATSDEBUG, nick, lcc, lcc * sizeof (Link));
  sendto_one (client_p, ":%s %d %s :   Fludees %d(%d)", me.name,
              RPL_STATSDEBUG, nick, fludlink, fludlink * sizeof (Link));

  sendto_one (client_p, ":%s %d %s :DLinks ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, dlinkalloc, dlinkallocsz);
  /* Print summary of DLINKs used in clientlist.c */
  print_list_memory (client_p);

  sendto_one (client_p, ":%s %d %s :WATCH headers %d(%d)",
              me.name, RPL_STATSDEBUG, nick, wlh, wlhm);
  sendto_one (client_p, ":%s %d %s :Conflines %d(%d)",
              me.name, RPL_STATSDEBUG, nick, co, com);
  sendto_one (client_p, ":%s %d %s :Classes %d(%d)",
              me.name, RPL_STATSDEBUG, nick, cl, cl * sizeof (aClass));
  sendto_one (client_p, ":%s %d %s :Away Messages %d(%d)",
              me.name, RPL_STATSDEBUG, nick, aw, awm);
  sendto_one (client_p, ":%s %d %s :MOTD structs %d(%d)",
              me.name, RPL_STATSDEBUG, nick, motdlen,
              motdlen * sizeof (aMotd));
  sendto_one (client_p, ":%s %d %s :Servers %d(%d)", me.name, RPL_STATSDEBUG,
              nick, servn, servn * sizeof (aServer));
  sendto_one (client_p, ":%s %d %s :Message Trees %d(%d)", me.name,
              RPL_STATSDEBUG, nick, num_msg_trees,
              num_msg_trees * sizeof (MESSAGE_TREE));

  totmisc =
    wlhm + com + (cl * sizeof (aClass)) + awm + (motdlen * sizeof (aMotd)) +
    (servn * sizeof (aServer)) + (num_msg_trees * sizeof (MESSAGE_TREE));

  sendto_one (client_p, ":%s %d %s :Fludbots ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, fludalloc, fludallocsz);

  sendto_one (client_p,
              ":%s %d %s :Channels %d(%d) ALLOC %d(%d) Bans %d(%d) Members %d(%d) ALLOC %d(%d)",
              me.name, RPL_STATSDEBUG, nick, ch, ch * sizeof (aChannel),
              chanalloc, chanallocsz, chb, chbm, chu,
              chu * sizeof (chanMember), cmemballoc, cmemballocsz);

  totch = chanallocsz + cmemballocsz + chbm;

  /* print userban summary, get userban total usage */
  totuban = count_userbans (client_p);

  sendto_one (client_p, ":%s %d %s :Whowas users %d(%d)",
              me.name, RPL_STATSDEBUG, nick, wwu, wwu * sizeof (anUser));
  sendto_one (client_p, ":%s %d %s :Whowas array %d(%d)",
              me.name, RPL_STATSDEBUG, nick, NICKNAMEHISTORYLENGTH, wwm);

  totww = wwu * sizeof (anUser) + wwm;

  sendto_one (client_p,
              ":%s %d %s :Hash: client %d(%d) chan %d(%d) whowas %d(%d) watch %d(%d)",
              me.name, RPL_STATSDEBUG, nick, U_MAX,
              sizeof (aHashEntry) * U_MAX, CH_MAX,
              sizeof (aHashEntry) * CH_MAX, WW_MAX,
              sizeof (aWhowas *) * WW_MAX, WATCHHASHSIZE,
              sizeof (aWatch *) * WATCHHASHSIZE);

  count_dbuf_memory (&db, &db2);
  sendto_one (client_p, ":%s %d %s :Dbuf blocks %d(%d) MAX %d(%d)",
              me.name, RPL_STATSDEBUG, nick, DBufUsedCount, db2,
              DBufCount, db);

  rm = cres_mem (client_p);

  count_scache (&number_servers_cached, &mem_servers_cached);

  sendto_one (client_p, ":%s %d %s :scache %d(%d)",
              me.name, RPL_STATSDEBUG, nick,
              number_servers_cached, mem_servers_cached);

  count_ip_hash (&number_ips_stored, &mem_ips_stored);
  sendto_one (client_p, ":%s %d %s :iphash %d(%d)",
              me.name, RPL_STATSDEBUG, nick,
              number_ips_stored, mem_ips_stored);

  totmisc += (mem_ips_stored + mem_servers_cached);

  tothash = (sizeof (aHashEntry) * U_MAX) + (sizeof (aHashEntry) * CH_MAX) +
    (sizeof (aWatch *) * WATCHHASHSIZE) + (sizeof (aWhowas *) * WW_MAX);

  tot =
    totww + totch + totcl + totmisc + db + rm + tothash + linkallocsz +
    dlinkallocsz + fludallocsz + totuban;

  sendto_one (client_p,
              ":%s %d %s :whowas %d chan %d client/user %d misc %d dbuf %d hash %d res %d link %d flud %d userban %d",
              me.name, RPL_STATSDEBUG, nick, totww, totch, totcl, totmisc, db,
              tothash, rm, linkallocsz, fludallocsz, totuban);

  sendto_one (client_p, ":%s %d %s :TOTAL: %d"
#ifndef _WIN32
              " sbrk(0)-etext: %d"
#endif
              , me.name, RPL_STATSDEBUG, nick, tot
#ifndef _WIN32
              , (void *) sbrk ((size_t) 0) - (void *) sbrk0
#endif
    );

  return;
}
