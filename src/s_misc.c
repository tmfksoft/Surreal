/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_misc.c
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
 *  $Id: s_misc.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#ifndef _WIN32
# include <sys/time.h>
#endif
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "zlink.h"
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
# include <sys/socket.h>
#else
# include <winsock2.h>
#endif
#if !defined(ULTRIX) && !defined(SGI) && !defined(sequent) && !defined(__convex__) && !defined(_WIN32)
#include <sys/param.h>
#endif
#if defined(_WIN32) || defined(AIX) || defined(SVR3) || ((__GNU_LIBRARY__ == 6) && (__GLIBC__ >=2) && (__GLIBC_MINOR__ >= 2))
#include <time.h>
#endif
#include "h.h"
#include "inet.h"               /*INET6 */
#include "fdlist.h"

extern float curSendK, curRecvK;

#ifdef NO_CHANOPS_WHEN_SPLIT
extern int server_was_split;
extern time_t server_split_time;
#endif

#ifdef ALWAYS_SEND_DURING_SPLIT
int currently_processing_netsplit = NO;
#endif

static void exit_one_client (aClient *, aClient *, aClient *, char *, int *);
static int remove_dcc_references (aClient *);
static void exit_one_client_in_split (aClient *, aClient *, char *);
static void exit_one_server (aClient *, aClient *, aClient *, aClient *,
                             char *, char *);
static void exit_server (aClient *, aClient *, aClient *, char *);


static char *months[] = {
  "January", "February", "March", "April",
  "May", "June", "July", "August",
  "September", "October", "November", "December"
};

static char *weekdays[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

/*
 * stats stuff
 */
struct stats ircst, *ircstp = &ircst;

char *
date (time_t myclock)
{
  static char buf[80], plus;
  struct tm *lt, *gm;
  struct tm gmbuf;
  int minswest;

  if (!myclock)
    time (&myclock);
  gm = gmtime (&myclock);
  memcpy ((char *) &gmbuf, (char *) gm, sizeof (gmbuf));
  gm = &gmbuf;
  lt = localtime (&myclock);

  if (lt->tm_yday == gm->tm_yday)
    minswest = (gm->tm_hour - lt->tm_hour) * 60 + (gm->tm_min - lt->tm_min);
  else if (lt->tm_yday > gm->tm_yday)
    minswest = (gm->tm_hour - (lt->tm_hour + 24)) * 60;
  else
    minswest = ((gm->tm_hour + 24) - lt->tm_hour) * 60;

  plus = (minswest > 0) ? '-' : '+';
  if (minswest < 0)
    minswest = -minswest;

  (void) ircsprintf (buf, "%s %s %d %04d -- %02d:%02d %c%02d:%02d",
                     weekdays[lt->tm_wday], months[lt->tm_mon], lt->tm_mday,
                     lt->tm_year + 1900, lt->tm_hour, lt->tm_min,
                     plus, minswest / 60, minswest % 60);

  return buf;
}

/*
 */
char *
smalldate (time_t myclock)
{
  static char buf[MAX_DATE_STRING];
  struct tm *lt, *gm;
  struct tm gmbuf;

  if (!myclock)
    time (&myclock);
  gm = gmtime (&myclock);
  memcpy ((char *) &gmbuf, (char *) gm, sizeof (gmbuf));
  gm = &gmbuf;
  lt = localtime (&myclock);

  (void) ircsprintf (buf, "%04d/%02d/%02d %02d.%02d",
                     lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                     lt->tm_hour, lt->tm_min);

  return buf;
}

/**
 ** myctime()
 **   This is like standard ctime()-function, but it zaps away
 **   the newline from the end of that string. Also, it takes
 **   the time value as parameter, instead of pointer to it.
 **   Note that it is necessary to copy the string to alternate
 **   buffer (who knows how ctime() implements it, maybe it statically
 **   has newline there and never 'refreshes' it -- zapping that
 **   might break things in other places...)
 **
 **/

char *
myctime (time_t value)
{
  static char buf[28];
  char *p;

  (void) strcpy (buf, ctime (&value));
  if ((p = (char *) strchr (buf, '\n')) != NULL)
    *p = '\0';

  return buf;
}

/*
 * * check_registered_user is used to cancel message, if the *
 * originator is a server or not registered yet. In other * words,
 * passing this test, *MUST* guarantee that the * source_p->user exists
 * (not checked after this--let there * be coredumps to catch bugs...
 * this is intentional --msa ;) *
 * 
 * There is this nagging feeling... should this NOT_REGISTERED * error
 * really be sent to remote users? This happening means * that remote
 * servers have this user registered, although this * one has it not...
 * Not really users fault... Perhaps this * error message should be
 * restricted to local clients and some * other thing generated for
 * remotes...
 */
int
check_registered_user (aClient * source_p)
{
  if (!IsRegisteredUser (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOTREGISTERED), me.name, "*");
    return -1;
  }
  return 0;
}

/*
 * * check_registered user cancels message, if 'x' is not * registered
 * (e.g. we don't know yet whether a server * or user)
 */
int
check_registered (aClient * source_p)
{
  if (!IsRegistered (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOTREGISTERED), me.name, "*");
    return -1;
  }
  return 0;
}

/*
 * * get_client_name *      Return the name of the client for various
 * tracking and *      admin purposes. The main purpose of this
 * function is to *      return the "socket host" name of the client,
 * if that *    differs from the advertised name (other than case). *
 * But, this can be used to any client structure. *
 * 
 *      Returns: *        "name[user@ip#.port]" if 'showip' is true; *
 * "name[sockethost]", if name and sockhost are different and *
 * showip is false; else *        "name". *
 * 
 * NOTE 1: *    Watch out the allocation of "nbuf", if either
 * source_p->name * or source_p->sockhost gets changed into pointers instead of *
 * directly allocated within the structure... *
 * 
 * NOTE 2: *    Function return either a pointer to the structure
 * (source_p) or *  to internal buffer (nbuf). *NEVER* use the returned
 * pointer *    to modify what it points!!!
 */
char *
get_client_name (aClient * source_p, int showip)
{
  static char nbuf[HOSTLEN * 2 + USERLEN + 5];

  if (MyConnect (source_p))
  {
    switch (showip)
    {
       case TRUE:
#ifdef SHOW_UH
         (void) ircsprintf (nbuf, "%s[%s%s@%s]",
                            source_p->name,
                            (!(source_p->flags & FLAGS_GOTID)) ? "" : "(+)",
                            source_p->username,
                            inet_ntop (AFINET, (char *) &source_p->ip,
                                       mydummy, sizeof (mydummy)));
#else
         (void) sprintf (nbuf, "%s[%s@%s]",
                         source_p->name,
                         (!(source_p->flags & FLAGS_GOTID)) ? "" : source_p->
                         username, inet_ntop (AFINET, (char *) &source_p->ip,
                                              mydummy, sizeof (mydummy)));
#endif
         break;
       case HIDEME:
#ifdef SHOW_UH
         (void) ircsprintf (nbuf, "%s[%s%s@%s]",
                            source_p->name,
                            (!(source_p->flags & FLAGS_GOTID)) ? "" :
                            "(+)",
                            source_p->user ? source_p->user->username :
                            source_p->username, INADDRANY_STR);
#else
         (void) sprintf (nbuf, "%s[%s@%s]",
                         source_p->name,
                         (!(source_p->flags & FLAGS_GOTID)) ? "" :
                         source_p->username, INADDRANY_STR);
#endif
         break;
       default:
         if (irccmp (source_p->name, source_p->sockhost) != 0)
#ifdef USERNAMES_IN_TRACE
           (void) ircsprintf (nbuf, "%s[%s@%s]",
                              source_p->name,
                              source_p->user ? source_p->user->username :
                              source_p->username, source_p->sockhost);
#else
           (void) ircsprintf (nbuf, "%s[%s]", source_p->name,
                              source_p->sockhost);
#endif
         else
           return source_p->name;
    }
    return nbuf;
  }
  return source_p->name;
}

char *
get_client_host (aClient * client_p)
{
  static char nbuf[HOSTLEN * 2 + USERLEN + 5];

  if (!MyConnect (client_p))
    return client_p->name;
#ifdef USE_ADNS
  if (!client_p->sockhost)
    return get_client_name (client_p, FALSE);
  else
    (void) ircsprintf (nbuf, "%s[%-.*s@%-.*s]",
                       client_p->name, USERLEN,
                       (!(client_p->flags & FLAGS_GOTID)) ? "" : client_p->
                       username, HOSTLEN, client_p->sockhost);
#else
  if (!client_p->hostp)

    return get_client_name (client_p, FALSE);
  else
    (void) ircsprintf (nbuf, "%s[%-.*s@%-.*s]",
                       client_p->name, USERLEN,
                       (!(client_p->flags & FLAGS_GOTID)) ? "" : client_p->
                       username, HOSTLEN, client_p->hostp->h_name);
#endif
  return nbuf;
}

/*
 * Form sockhost such that if the host is of form user@host, only the
 * host portion is copied.
 */
void
get_sockhost (aClient * client_p, char *host)
{
  char *s;

  if ((s = (char *) strchr (host, '@')))
    s++;
  else
    s = host;
  strncpyzt (client_p->sockhost, s, sizeof (client_p->sockhost));
}

/*
 * Return wildcard name of my server name according to given config
 * entry --Jto
 */
char *
my_name_for_link (char *name, aConfItem * aconf)
{
  static char namebuf[HOSTLEN];
  int count = aconf->port;
  char *start = name;

  if (count <= 0 || count > 5)
    return start;

  while (count-- && name)
  {
    name++;
    name = (char *) strchr (name, '.');
  }
  if (!name)
    return start;

  namebuf[0] = '*';
  (void) strncpy (&namebuf[1], name, HOSTLEN - 1);
  namebuf[HOSTLEN - 1] = '\0';
  return namebuf;
}

int
remove_dcc_references (aClient * source_p)
{
  aClient *target_p;
  Link *lp, *nextlp;
  Link **lpp, *tmp;
  int found;

  lp = source_p->user->dccallow;

  while (lp)
  {
    nextlp = lp->next;
    target_p = lp->value.client_p;
    for (found = 0, lpp = &(target_p->user->dccallow); *lpp;
         lpp = &((*lpp)->next))
    {
      if (lp->flags == (*lpp)->flags)
        continue;               /* match only opposite types for sanity */
      if ((*lpp)->value.client_p == source_p)
      {
        if ((*lpp)->flags == DCC_LINK_ME)
        {
          sendto_one (target_p,
                      ":%s %d %s :%s has been removed from your DCC allow list for signing off",
                      me.name, RPL_DCCINFO, target_p->name, source_p->name);
        }
        tmp = *lpp;
        *lpp = tmp->next;
        free_link (tmp);
        found++;
        break;
      }
    }

    if (!found)
      sendto_realops_lev (DEBUG_LEV,
                          "rdr(): %s was in dccallowme list[%d] of %s but not in dccallowrem list!",
                          target_p->name, lp->flags, source_p->name);
    free_link (lp);
    lp = nextlp;
  }
  return 0;
}

/*
 * NOQUIT
 * a method of reducing the stress on the network during server splits
 * by sending only a simple "SQUIT" message for the server that is dropping,
 * instead of thousands upon thousands of QUIT messages for each user,
 * plus an SQUIT for each server behind the dead link.
 *
 * Original idea by Cabal95, implementation by lucas
 */

void
exit_one_client_in_split (aClient * client_p, aClient * dead, char *reason)
{
  Link *lp;

  /* send all the quit reasons to all the non-noquit servers we have */

  /* yikes. We only want to do this if dead was OUR server. */
  /* erm, no, that's not true. Doing that breaks things. 
   * If a non-noquit server is telling us a server has split,
   * we will have already recieved hundreds of QUIT messages
   * from it, which will be passed anyway, and this procedure
   * will never be called. - lucas
   */

  /*
   * We want to keep in mind that some U Lined servers and even services (Epona)
   * have clients that will be IN channels. In order not to give away where the U Lined
   * clients are linked we'll want to sendout a custom reason - ShadowMaster
   */

  if (!IsULine (client_p))
  {
    sendto_noquit_servs_butone (0, dead, ":%s QUIT :%s", client_p->name,
                                reason);

    sendto_common_channels (client_p, ":%s QUIT :%s", client_p->name, reason);
  }
  else
  {
    sendto_noquit_servs_butone (0, dead,
                                ":%s QUIT :Client Lost In Netsplit",
                                client_p->name);

    sendto_common_channels (client_p, ":%s QUIT :Client Lost In Netsplit",
                            client_p->name);
  }

  while ((lp = client_p->user->channel))
    remove_user_from_channel (client_p, lp->value.channel_p);
  while ((lp = client_p->user->invited))
    del_invite (client_p, lp->value.channel_p);
  while ((lp = client_p->user->silence))
    del_silence (client_p, lp->value.cp);

  remove_dcc_references (client_p);

  del_from_client_hash_table (client_p->name, client_p);

  hash_check_watch (client_p, RPL_LOGOFF);

  remove_client_from_list (client_p);
}

/* exit_one_server
 *
 * recursive function!
 * therefore, we pass dead and reason to ourselves.
 * in the beginning, dead == client_p, so it will be the one
 *  out of the loop last. therefore, dead should remain a good pointer.
 * client_p: the server being exited
 * dead: the actual server that split (if this belongs to us, we
 *       absolutely CANNOT send to it)
 * from: the client that caused this split
 * lclient_p: the local client that initiated this
 * spinfo: split reason, as generated in exit_server
 * comment: comment provided
 */

void
exit_one_server (aClient * client_p, aClient * dead, aClient * from,
                 aClient * lclient_p, char *spinfo, char *comment)
{
  aClient *target_p, *next;
  DLink *lp;

  /* okay, this is annoying.
   * first off, we need two loops.
   * one: to remove all the clients.
   * two: to remove all the servers.
   * HOWEVER! removing a server may cause removal of more servers and more clients.
   *  and this may make our pointer to next bad. therefore, we have to restart
   *  the server loop each time we find a server.
   * We _NEED_ two different loops: all clients must be removed before the server is
   *  removed. Otherwise, bad things (tm) can happen.
   */

  Debug ((DEBUG_NOTICE, "server noquit: %s", client_p->name));

  for (target_p = client; target_p; target_p = next)
  {
    next = target_p->next;      /* we might destroy this client record in the loop. */

    if (target_p->uplink != client_p || !IsPerson (target_p))
      continue;

    exit_one_client_in_split (target_p, dead, spinfo);
  }

  for (target_p = client; target_p; target_p = next)
  {
    next = target_p->next;      /* we might destroy this client record in the loop. */

    if (target_p->uplink != client_p || !IsServer (target_p))
      continue;

    exit_one_server (target_p, dead, from, lclient_p, spinfo, comment);
    next = client;              /* restart the loop */
  }

  Debug ((DEBUG_NOTICE, "done exiting server: %s", client_p->name));

  for (lp = server_list; lp; lp = lp->next)
  {
    target_p = lp->value.client_p;
    if (target_p == client_p || IsMe (target_p) ||
        target_p == dead || target_p == lclient_p)
      continue;

    /* if the server is noquit, we only want to send it information about 'dead' */
    /* if it's not, this server gets split information for ALL dead servers. */

    if (IsNoQuit (target_p) && client_p != dead)
      continue;

    if (client_p->from == target_p)     /* "upstream" squit */
      sendto_one (target_p, ":%s SQUIT %s :%s", from->name, client_p->name,
                  comment);
    else
      sendto_one (target_p, "SQUIT %s :%s", client_p->name, comment);
  }

  del_from_client_hash_table (client_p->name, client_p);
  hash_check_watch (client_p, RPL_LOGOFF);
  remove_client_from_list (client_p);
}

/* exit_server
 *
 * lclient_p: the local client that initiated this
 * client_p: the server that is being dropped.
 * from: the client/server that caused this to happen
 * comment: reason this is happening
 * we then call exit_one_server, the recursive function.
 */

void
exit_server (aClient * lclient_p, aClient * client_p, aClient * from,
             char *comment)
{
  char splitname[HOSTLEN + HOSTLEN + 2];

  ircsprintf (splitname, "%s %s", client_p->uplink->name, client_p->name);

  Debug ((DEBUG_NOTICE, "exit_server(%s, %s, %s)", client_p->name, from->name,
          comment));

  exit_one_server (client_p, client_p, from, lclient_p, splitname, comment);
}


/*
 *  exit_client
 * This is old "m_bye". Name  changed, because this is not a 
 * protocol function, but a general server utility function. 
 * 
 *      This function exits a client of *any* type (user, server, etc) 
 * from this server. Also, this generates all necessary prototol 
 * messages that this exit may cause. 
 * 
 *   1) If the client is a local client, then this implicitly exits
 * all other clients depending on this connection (e.g. remote
 * clients having 'from'-field that points to this. 
 * 
 *   2) If the client is a remote client, then only this is exited. 
 * 
 * For convenience, this function returns a suitable value for 
 * m_function return value: 
 * 
 * FLUSH_BUFFER    if (client_p == source_p) 0 if (client_p != source_p)
 *
 * client_p - The local client originating the exit or NULL, if this exit is
 *            generated by this server for internal reasons. This will not
 *            get any of the generated messages.
 * 
 * source_p - Client exiting
 *
 * from -  Client firing off this Exit, never NULL
 *
 * comment - Reason for the exit
*/
int
exit_client (aClient * client_p, aClient * source_p, aClient * from,
             char *comment)
{
  int remote = 1;
#ifdef	FNAME_USERLOG
  time_t on_for;

#endif

  if (MyConnect (source_p))
  {
    remote = 0;
#ifdef USE_ADNS
    if (source_p->dnslookup)
    {
      free (source_p->dnslookup);
    }
#endif
    if (source_p->flags & FLAGS_IPHASH)
    {
      remove_one_ip (source_p->ip.S_ADDR);
    }
    if (IsAnOper (source_p))
    {
      remove_from_list (&oper_list, source_p, NULL);
    }
    if (source_p->flags & FLAGS_HAVERECVQ)
    {
      remove_from_list (&recvq_clients, source_p, NULL);
    }
    if (IsClient (source_p))
    {
      Count.local--;
    }
    if (IsNegoServer (source_p))
    {
      sendto_realops ("Lost server %s during negotiation: %s",
                      source_p->name, comment);
    }
    if (IsServer (source_p))
    {
      Count.myserver--;
      if (IsULine (source_p))
      {
        Count.myulined--;
      }
      remove_from_list (&server_list, source_p, NULL);
#ifdef NO_CHANOPS_WHEN_SPLIT
      if (server_list == NULL)
      {
        server_was_split = YES;
        server_split_time = NOW;
      }
#endif
    }
    source_p->flags |= FLAGS_CLOSING;
    if (IsPerson (source_p))
    {
      Link *lp, *next;
      LOpts *lopt = source_p->user->lopt;
      /* poof goes their watchlist! */
      hash_del_watch_list (source_p);
      /* if they have listopts, axe those, too */
      if (lopt != NULL)
      {
        remove_from_list (&listing_clients, source_p, NULL);
        for (lp = lopt->yeslist; lp; lp = next)
        {
          next = lp->next;
          free_link (lp);
        }
        for (lp = lopt->nolist; lp; lp = next)
        {
          next = lp->next;
          free_link (lp);
        }

        MyFree (source_p->user->lopt);
        source_p->user->lopt = NULL;
      }

      sendto_connectnotice
        ("from %s: Client exiting: %s (%s@%s) [%s]%s [%s]", me.name,
         source_p->name, source_p->user->username, source_p->user->host,
         source_p->hostip, IsSSLClient (source_p) ? " (SSL)" : "", comment);

      ilog (LOGF_CLIENTS, "Client exiting: %s (%s@%s) [%s]%s [%s]",
            source_p->name, source_p->user->username, source_p->user->host,
            source_p->hostip, IsSSLClient (source_p) ? " (SSL)" : "",
            comment);

      if (GLOBAL_CONNECTS == 1)
      {
        sendto_serv_butone (client_p,
                            ":%s GCONNECT :Client exiting: %s (%s@%s) [%s]%s [%s]",
                            me.name, source_p->name,
                            source_p->user->username,
                            source_p->user->host, source_p->hostip,
                            IsSSLClient (source_p) ? " (SSL)" : "", comment);
      }

    }
#ifdef FNAME_USERLOG
    on_for = timeofday - source_p->firsttime;
#endif
#if defined(USE_SYSLOG) && defined(SYSLOG_USERS)
    if (IsPerson (source_p))
      syslog (LOG_NOTICE, "%s (%3d:%02d:%02d): %s!%s@%s %d/%d\n",
              myctime (source_p->firsttime),
              on_for / 3600, (on_for % 3600) / 60,
              on_for % 60, source_p->name,
              source_p->user->username, source_p->user->host,
              source_p->sendK, source_p->receiveK);
#endif
#if defined(FNAME_USERLOG)
    {
      char linebuf[300];
      static int logfile = -1;
      static long lasttime;

      /*
       * This conditional makes the logfile active only after it's
       * been created - thus logging can be turned off by removing
       * the file.
       * 
       * stop NFS hangs...most systems should be able to open a file in
       * 3 seconds. -avalon (curtesy of wumpus)
       * 
       * Keep the logfile open, syncing it every 10 seconds -Taner
       */
      if (IsPerson (source_p))
      {
        if (logfile == -1)
        {
#ifndef _WIN32
          (void) alarm (3);
#endif
          logfile = open (FNAME_USERLOG, O_WRONLY | O_APPEND);
#ifndef _WIN32
          (void) alarm (0);
#endif
        }
        (void) ircsprintf (linebuf,
                           "%s (%3d:%02d:%02d): %s!%s@%s %d/%d\n",
                           myctime (source_p->firsttime), on_for / 3600,
                           (on_for % 3600) / 60, on_for % 60,
                           source_p->name,
                           source_p->user->username,
                           source_p->user->host, source_p->sendK,
                           source_p->receiveK);
#ifndef _WIN32
        (void) alarm (3);
#endif
        (void) write (logfile, linebuf, strlen (linebuf));
#ifndef _WIN32
        (void) alarm (0);
#endif
        /*
         * Resync the file evey 10 seconds
         */
        if (timeofday - lasttime > 10)
        {
#ifndef _WIN32
          (void) alarm (3);
#endif
          (void) close (logfile);
#ifndef _WIN32
          (void) alarm (0);
#endif
          logfile = -1;
          lasttime = timeofday;
        }
      }
    }
#endif
    if (source_p->fd >= 0
#ifdef USE_SSL
        && !IsDead (source_p)
#endif
      )
    {
      if (client_p != NULL && source_p != client_p)
        sendto_one (source_p, "ERROR :Closing Link: %s %s (%s)",
                    IsPerson (source_p) ? source_p->
                    sockhost : INADDRANY_STR, source_p->name, comment);
      else
        sendto_one (source_p, "ERROR :Closing Link: %s (%s)",
                    IsPerson (source_p) ? source_p->
                    sockhost : INADDRANY_STR, comment);
    }
    /*
     * * Currently only server connections can have * depending
     * remote clients here, but it does no * harm to check for all
     * local clients. In * future some other clients than servers
     * might * have remotes too... *
     * 
     * Close the Client connection first and mark it * so that no
     * messages are attempted to send to it. *, The following *must*
     * make MyConnect(source_p) == FALSE!). * It also makes source_p->from ==
     * NULL, thus it's unnecessary * to test whether "source_p != target_p"
     * in the following loops.
     */
    if (IsServer (source_p))
    {
      sendto_ops
        ("%s was connected for %lu seconds.  %lu/%lu sendK/recvK.",
         source_p->name, timeofday - source_p->firsttime, source_p->sendK,
         source_p->receiveK);
#ifdef USE_SYSLOG
      syslog (LOG_NOTICE,
              "%s was connected for %lu seconds.  %lu/%lu sendK/recvK.",
              source_p->name, timeofday - source_p->firsttime,
              source_p->sendK, source_p->receiveK);
#endif
      close_connection (source_p);

      source_p->sockerr = 0;
      source_p->flags |= FLAGS_DEADSOCKET;

      /*
       * * First QUIT all NON-servers which are behind this link *
       * 
       * Note        There is no danger of 'client_p' being exited in *  the
       * following loops. 'client_p' is a *local* client, *      all
       * dependants are *remote* clients.
       */
      /*
       * This next bit is a a bit ugly but all it does is take the *
       * name of us.. me.name and tack it together with the name of *
       * the server source_p->name that just broke off and puts this *
       * together into exit_one_client() to provide some useful *
       * information about where the net is broken.      Ian
       */
    }
    else
    {
      close_connection (source_p);

      source_p->sockerr = 0;
      source_p->flags |= FLAGS_DEADSOCKET;
    }

  }
  exit_one_client (client_p, source_p, from, comment, (int *) remote);
  return client_p == source_p ? FLUSH_BUFFER : 0;
}

/*
 * * Exit one client, local or remote. Assuming all dependants have *
 * been already removed, and socket closed for local client.
 */
static void
exit_one_client (aClient * client_p,
                 aClient * source_p, aClient * from, char *comment,
                 int *remote)
{
  Link *lp;
  aClient *a2client_p;

  /*
   * *  For a server or user quitting, propogate the information to *
   * other servers (except to the one where is came from (client_p))
   */
  if (IsMe (source_p))
  {
    sendto_ops ("ERROR: tried to exit me! : %s", comment);
    return;                     /*
                                 * ...must *never* exit self!! 
                                 */
  }
  else if (IsServer (source_p))
  {

# ifdef ALWAYS_SEND_DURING_SPLIT
    currently_processing_netsplit = YES;
# endif

    exit_server (client_p, source_p, from, comment);

# ifdef ALWAYS_SEND_DURING_SPLIT
    currently_processing_netsplit = NO;
# endif
    return;
  }
  else if (!(IsPerson (source_p)))
    /*
     * ...this test is *dubious*, would need * some thought.. but for
     * now it plugs a * nasty hole in the server... --msa
     */
    ;                           /*
                                 * Nothing 
                                 */
  else if (source_p->name[0])
  {                             /*
                                 * ...just clean all others with
                                 * * QUIT... 
                                 */
    /*
     * * If this exit is generated from "m_kill", then there * is no
     * sense in sending the QUIT--KILL's have been * sent instead.
     */
    if ((source_p->flags & FLAGS_KILLED) == 0)
    {
      sendto_serv_butone (client_p, ":%s QUIT :%s", source_p->name, comment);
    }
    /*
     * * If a person is on a channel, send a QUIT notice * to every
     * client (person) on the same channel (so * that the client can
     * show the "**signoff" message). * (Note: The notice is to the
     * local clients *only*)
     */
    if (source_p->user)
    {
      a2client_p = find_server (source_p->user->server, NULL);
      send_part_to_common_channels (source_p, comment);
      send_quit_to_common_channels (source_p, comment);
      while ((lp = source_p->user->channel))
        remove_user_from_channel (source_p, lp->value.channel_p);

      /*
       * The EOBurst check is only needed as a temporary cludge
       * until we get rid of the GCONNECT legacy to avoid
       * double exit notices from being sent - ShadowMaster
       */
      if (remote && IsEOBurst (a2client_p))
      {
        sendto_globalconnectnotice
          ("from %s: Client exiting: %s (%s@%s) [%s]%s [%s]",
           source_p->user->server, source_p->name,
           source_p->user->username, source_p->user->host,
           source_p->hostip, IsSSL (source_p) ? " (SSL)" : "", comment);
      }

      /*
       * Clean up invitefield 
       */
      while ((lp = source_p->user->invited))
        del_invite (source_p, lp->value.channel_p);
      /* Clean up silences */
      while ((lp = source_p->user->silence))
        (void) del_silence (source_p, lp->value.cp);
      remove_dcc_references (source_p);
      /*
       * again, this is all that is needed
       */
    }
  }

  /*
   * Remove source_p from the client list 
   */
  if (del_from_client_hash_table (source_p->name, source_p) != 1)
  {
    /*
     * This is really odd - oh well, it just generates noise...
     * -Taner
     * 
     * sendto_realops("%#x !in tab %s[%s]", source_p, source_p->name, source_p->from
     * ? source_p->from->sockhost : "??host"); sendto_realops("from =
     * %#x", source_p->from); sendto_realops("next = %#x", source_p->next);
     * sendto_realops("prev = %#x", source_p->prev); sendto_realops("fd =
     * %d  status = %d", source_p->fd, source_p->status);
     * sendto_realops("user = %#x", source_p->user);
     */

    Debug ((DEBUG_ERROR, "%#x !in tab %s[%s] %#x %#x %#x %d %d %#x",
            source_p, source_p->name,
            source_p->from ? source_p->from->sockhost : "??host",
            source_p->from, source_p->next, source_p->prev, source_p->fd,
            source_p->status, source_p->user));
  }
  /* remove user from watchlists */
  if (IsRegistered (source_p))
    hash_check_watch (source_p, RPL_LOGOFF);
  remove_client_from_list (source_p);
  return;
}

void
checklist ()
{
  aClient *target_p;
  int i, j;

  if (!(bootopt & BOOT_AUTODIE))
    return;
  for (j = i = 0; i <= highest_fd; i++)
    if (!(target_p = local[i]))
      continue;
    else if (IsClient (target_p))
      j++;
  if (!j)
  {
#ifdef	USE_SYSLOG
    syslog (LOG_WARNING, "ircd exiting: checklist() s_misc.c autodie");
#endif
    exit (0);
  }
  return;
}

void
initstats ()
{
  memset ((char *) &ircst, '\0', sizeof (ircst));
}

void
tstats (aClient * client_p, char *name)
{
  aClient *target_p;
  int i;
  struct stats *sp;
  struct stats tmp;

  sp = &tmp;
  memcpy ((char *) sp, (char *) ircstp, sizeof (*sp));
  for (i = 0; i < highest_fd; i++)
  {
    if (!(target_p = local[i]))
      continue;
    if (IsServer (target_p))
    {
      sp->is_sbs += target_p->sendB;
      sp->is_sbr += target_p->receiveB;
      sp->is_sks += target_p->sendK;
      sp->is_skr += target_p->receiveK;
      sp->is_sti += timeofday - target_p->firsttime;
      sp->is_sv++;
      if (sp->is_sbs > 1023)
      {
        sp->is_sks += (sp->is_sbs >> 10);
        sp->is_sbs &= 0x3ff;
      }
      if (sp->is_sbr > 1023)
      {
        sp->is_skr += (sp->is_sbr >> 10);
        sp->is_sbr &= 0x3ff;
      }

    }
    else if (IsClient (target_p))
    {
      sp->is_cbs += target_p->sendB;
      sp->is_cbr += target_p->receiveB;
      sp->is_cks += target_p->sendK;
      sp->is_ckr += target_p->receiveK;
      sp->is_cti += timeofday - target_p->firsttime;
      sp->is_cl++;
      if (sp->is_cbs > 1023)
      {
        sp->is_cks += (sp->is_cbs >> 10);
        sp->is_cbs &= 0x3ff;
      }
      if (sp->is_cbr > 1023)
      {
        sp->is_ckr += (sp->is_cbr >> 10);
        sp->is_cbr &= 0x3ff;
      }

    }
    else if (IsUnknown (target_p))
      sp->is_ni++;
  }

  sendto_one (client_p, ":%s %d %s :accepts %u refused %u",
              me.name, RPL_STATSDEBUG, name, sp->is_ac, sp->is_ref);
  sendto_one (client_p, ":%s %d %s :unknown commands %u prefixes %u",
              me.name, RPL_STATSDEBUG, name, sp->is_unco, sp->is_unpf);
  sendto_one (client_p, ":%s %d %s :nick collisions %u unknown closes %u",
              me.name, RPL_STATSDEBUG, name, sp->is_kill, sp->is_ni);
  sendto_one (client_p, ":%s %d %s :wrong direction %u empty %u",
              me.name, RPL_STATSDEBUG, name, sp->is_wrdi, sp->is_empt);
  sendto_one (client_p, ":%s %d %s :numerics seen %u mode fakes %u",
              me.name, RPL_STATSDEBUG, name, sp->is_num, sp->is_fake);
  sendto_one (client_p, ":%s %d %s :auth successes %u fails %u",
              me.name, RPL_STATSDEBUG, name, sp->is_asuc, sp->is_abad);
  sendto_one (client_p, ":%s %d %s :proxy successes %u fails %u",
              me.name, RPL_STATSDEBUG, name, sp->is_psuc, sp->is_pbad);
  sendto_one (client_p, ":%s %d %s :local connections %u udp packets %u",
              me.name, RPL_STATSDEBUG, name, sp->is_loc, sp->is_udp);
  sendto_one (client_p,
              ":%s %d %s :drones refused %u throttled rejections %u", me.name,
              RPL_STATSDEBUG, name, sp->is_drone, sp->is_throt);
  sendto_one (client_p,
              ":%s %d %s :banned users refused before ident/dns %u after ident/dns %u",
              me.name, RPL_STATSDEBUG, name, sp->is_ref_1, sp->is_ref_2);
  sendto_one (client_p, ":%s %d %s :Client Server", me.name, RPL_STATSDEBUG,
              name);
  sendto_one (client_p, ":%s %d %s :connected %u %u", me.name, RPL_STATSDEBUG,
              name, sp->is_cl, sp->is_sv);
  sendto_one (client_p, ":%s %d %s :bytes sent %u.%uK %u.%uK", me.name,
              RPL_STATSDEBUG, name, sp->is_cks, sp->is_cbs, sp->is_sks,
              sp->is_sbs);
  sendto_one (client_p, ":%s %d %s :bytes recv %u.%uK %u.%uK", me.name,
              RPL_STATSDEBUG, name, sp->is_ckr, sp->is_cbr, sp->is_skr,
              sp->is_sbr);
  sendto_one (client_p, ":%s %d %s :time connected %u %u", me.name,
              RPL_STATSDEBUG, name, sp->is_cti, sp->is_sti);
#ifdef FLUD
  sendto_one (client_p, ":%s %d %s :CTCP Floods Blocked %u",
              me.name, RPL_STATSDEBUG, name, sp->is_flud);
#endif /*
        * FLUD 
        */
}

/*
 * Retarded - so sue me :-P
 */
#define	_1MEG	(1024.0)
#define	_1GIG	(1024.0*1024.0)
#define	_1TER	(1024.0*1024.0*1024.0)
#define	_GMKs(x)	( (x > _1TER) ? "Terabytes" : ((x > _1GIG) ? "Gigabytes" : \
			((x > _1MEG) ? "Megabytes" : "Kilobytes")))
#define	_GMKv(x)	( (x > _1TER) ? (float)(x/_1TER) : ((x > _1GIG) ? \
			(float)(x/_1GIG) : ((x > _1MEG) ? (float)(x/_1MEG) : (float)x)))

void
serv_info (aClient * client_p, char *name)
{
  static char Lformat[] = ":%s %d %s %s %u %u %u %u %u :%u %u %s";
  long sendK, receiveK, uptime;
  aClient *target_p;
  DLink *lp;
  int i = 0;

  sendK = receiveK = 0;

  for (lp = server_list; lp; lp = lp->next)
  {
    target_p = lp->value.client_p;

    if (HIDEULINEDSERVS == 1)
    {
      if (IsULine (target_p) && !IsAnOper (client_p))
        continue;
    }
    sendK += target_p->sendK;
    receiveK += target_p->receiveK;
    sendto_one (client_p, Lformat, me.name, RPL_STATSLINKINFO,
                name, get_client_name (target_p, HIDEME),
                (int) DBufLength (&target_p->sendQ),
                (int) target_p->sendM, (int) target_p->sendK,
                (int) target_p->receiveM, (int) target_p->receiveK,
                target_p->firsttime,
                timeofday - target_p->since,
                IsServer (target_p) ? (DoesTS (target_p) ? "TS" : "NoTS") :
                "-");

    if (RC4EncLink (target_p))
      sendto_one (client_p, ":%s %d %s : - RC4 encrypted",
                  me.name, RPL_STATSDEBUG, name);

    if (ZipOut (target_p))
    {
      unsigned long ib, ob;
      double rat;

      zip_out_get_stats (target_p->serv->zip_out, &ib, &ob, &rat);
      if (ib)
      {
        sendto_one (client_p,
                    ":%s %d %s : - [O] Zip inbytes %d, outbytes %d (%3.2f%%)",
                    name, RPL_STATSDEBUG, name, ib, ob, rat);
      }
    }
    if (ZipIn (target_p))
    {
      unsigned long ib, ob;
      double rat;
      zip_in_get_stats (target_p->serv->zip_in, &ib, &ob, &rat);
      if (ob)
      {
        sendto_one (client_p,
                    ":%s %d %s : - [I] Zip inbytes %d, outbytes %d (%3.2f%%)",
                    me.name, RPL_STATSDEBUG, name, ib, ob, rat);
      }
    }
    i++;
  }
  sendto_one (client_p, ":%s %d %s :%u total server%s",
              me.name, RPL_STATSDEBUG, name, i, (i == 1) ? "" : "s");

  sendto_one (client_p, ":%s %d %s :Sent total : %7.2f %s",
              me.name, RPL_STATSDEBUG, name, _GMKv (sendK), _GMKs (sendK));
  sendto_one (client_p, ":%s %d %s :Recv total : %7.2f %s",
              me.name, RPL_STATSDEBUG, name, _GMKv (receiveK),
              _GMKs (receiveK));

  uptime = (timeofday - me.since);
  sendto_one (client_p,
              ":%s %d %s :Server send: %7.2f %s (%4.1f Kb/s total, %4.1f Kb/s current)",
              me.name, RPL_STATSDEBUG, name, _GMKv (me.sendK),
              _GMKs (me.sendK), (float) ((float) me.sendK / (float) uptime),
              curSendK);
  sendto_one (client_p,
              ":%s %d %s :Server recv: %7.2f %s (%4.1f Kb/s total, %4.1f Kb/s current)",
              me.name, RPL_STATSDEBUG, name, _GMKv (me.receiveK),
              _GMKs (me.receiveK),
              (float) ((float) me.receiveK / (float) uptime), curRecvK);
}

void
show_opers (aClient * client_p, char *name)
{
  aClient *client_p2;
  DLink *lp;
  int j = 0;

  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p2 = lp->value.client_p;
    if (!IsAnOper (client_p))
    {
      if (client_p2->umode & UMODE_h)
      {
        sendto_one (client_p, ":%s %d %s :%s (%s@%s) Idle: %d",
                    me.name, RPL_STATSDEBUG, name, client_p2->name,
                    client_p2->user->username,
                    (IsHidden (client_p2) ? client_p2->user->
                     virthost : client_p2->user->host),
                    timeofday - client_p2->user->last);
        j++;
      }
    }
    else
    {
      sendto_one (client_p, ":%s %d %s :%s (%s@%s) Idle: %d",
                  me.name, RPL_STATSDEBUG, name, client_p2->name,
                  client_p2->user->username,
                  (IsHidden (client_p2) ? client_p2->user->
                   virthost : client_p2->user->host),
                  timeofday - client_p2->user->last);
      j++;
    }
  }
  sendto_one (client_p, ":%s %d %s :%d OPER%s", me.name, RPL_STATSDEBUG,
              name, j, (j == 1) ? "" : "s");
}

void
show_servers (aClient * client_p, char *name)
{
  aClient *client_p2;
  DLink *lp;
  int j = 0;

  for (lp = server_list; lp; lp = lp->next)
  {
    client_p2 = lp->value.client_p;
#ifdef HIDEULINEDSERVS
    if (IsULine (client_p2) && !IsAnOper (client_p))
      continue;
#endif
    j++;
    sendto_one (client_p, ":%s %d %s :%s (%s!%s@%s) Idle: %d",
                me.name, RPL_STATSDEBUG, name, client_p2->name,
                (client_p2->serv->bynick[0] ? client_p2->serv->
                 bynick : "Remote."),
                (client_p2->serv->byuser[0] ? client_p2->serv->
                 byuser : "*"),
                (client_p2->serv->byhost[0] ? client_p2->serv->
                 byhost : "*"), timeofday - client_p2->lasttime);
  }
  sendto_one (client_p, ":%s %d %s :%d Server%s", me.name, RPL_STATSDEBUG,
              name, j, (j == 1) ? "" : "s");
}


char *
make_parv_copy (char *pbuf, int parc, char *parv[])
{
  int pbpos = 0, i;

  for (i = 1; i < parc; i++)
  {
    char *tmp = parv[i];

    if (i != 1)
    {
      pbuf[pbpos++] = ' ';
    }

    if (i == (parc - 1))
    {
      pbuf[pbpos++] = ':';
    }

    while (*tmp)
    {
      pbuf[pbpos++] = *(tmp++);
    }
  }
  pbuf[pbpos] = '\0';

  return pbuf;
}
