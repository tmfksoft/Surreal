/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/list.c
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
 *  $Id: list.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "numeric.h"
#include "blalloc.h"
#include "dh.h"
#include "zlink.h"


/*
 * locally defined functions 
 */

/*
 * re-written to use Wohali (joant@cadence.com) block allocator
 * routines. very nicely done Wohali
 * 
 * -Dianora
 * 
 */
/*
 * Number of Link's to pre-allocate at a time for Efnet 1000 seems
 * reasonable, for smaller nets who knows? -Dianora
 */

#define LINK_PREALLOCATE 512
#define DLINK_PREALLOCATE 128
#define CHANMEMBER_PREALLOCATE 512
/*
 * Number of aClient structures to preallocate at a time for Efnet 1024
 * is reasonable for smaller nets who knows? -Dianora
 */
/*
 * This means you call MyMalloc 30 some odd times, rather than 30k
 * times -Dianora
 */


#define CLIENTS_PREALLOCATE 512
#if CLIENTS_PREALLOCATE > MAXCONNECTIONS
#undef CLIENTS_PREALLOCATE
#define CLIENTS_PREALLOCATE MAXCONNECTIONS
#endif

/*
 * Number of channels to allocate per block, 1024 sounds nice.
 */

#define CHANNELS_PREALLOCATE 512

void outofmemory ();

int numclients = 0;

/*
 * for jolo's block allocator 
 */
BlockHeap *free_local_aClients;
BlockHeap *free_Links;
BlockHeap *free_DLinks;
BlockHeap *free_chanMembers;
BlockHeap *free_remote_aClients;
BlockHeap *free_anUsers;
BlockHeap *free_channels;

#ifdef FLUD
BlockHeap *free_fludbots;

#endif /*
        * FLUD 
        */

void
initlists ()
{
  /*
   * Might want to bump up LINK_PREALLOCATE if FLUD is defined 
   */
  free_Links = BlockHeapCreate ((size_t) sizeof (Link), LINK_PREALLOCATE);
  free_DLinks = BlockHeapCreate ((size_t) sizeof (DLink), DLINK_PREALLOCATE);
  free_chanMembers =
    BlockHeapCreate ((size_t) sizeof (chanMember), CHANMEMBER_PREALLOCATE);

  /*
   * start off with CLIENTS_PREALLOCATE for now... on typical efnet
   * these days, it can get up to 35k allocated
   */

  free_remote_aClients =
    BlockHeapCreate ((size_t) CLIENT_REMOTE_SIZE, CLIENTS_PREALLOCATE);

  /*
   * Can't EVER have more than MAXCONNECTIONS number of local aClients 
   * And that was stupid, because an idle server built for 32k would use
   * 50 megs of ram.
   */

  free_local_aClients = BlockHeapCreate ((size_t) CLIENT_LOCAL_SIZE,
                                         CLIENTS_PREALLOCATE);
  /*
   * anUser structs are used by both local aClients, and remote
   * aClients
   */

  free_anUsers = BlockHeapCreate ((size_t) sizeof (anUser),
                                  CLIENTS_PREALLOCATE * 2);

  /* channels are a very frequent thing in ircd. :) */

  free_channels = BlockHeapCreate ((size_t) sizeof (aChannel),
                                   CHANNELS_PREALLOCATE);


#ifdef FLUD
  /*
   * fludbot structs are used to track CTCP Flooders 
   */
  free_fludbots = BlockHeapCreate ((size_t) sizeof (struct fludbot),
                                   CLIENTS_PREALLOCATE);

#endif /*
        * FLUD 
        */
}

/*
 * outofmemory()
 * 
 * input            - NONE
 * output           - NONE
 * side effects     - simply try to report there is a problem I free
 * all the memory in the kline lists hoping to free enough memory so
 * that a proper report can be made.
 * If I was already here (was_here) then I got called twice, and more
 * drastic measures are in order. I'll try to just abort() at least.
 * -Dianora
 */

void
outofmemory ()
{
  static int was_here = 0;

  if (was_here)
    abort ();

  was_here = YES;

  Debug ((DEBUG_FATAL, "Out of memory: restarting server..."));
  restart ("Out of Memory");
}

/*
 * * Create a new aClient structure and set it to initial state. *
 * 
 *      from == NULL,   create local client (a client connected *
 * o a socket). *
 * 
 *      from,   create remote client (behind a socket *
 * ssociated with the client defined by *
 *  ('from' is a local client!!).
 *
 * uplink is this client's uplink connection to the network. - lucas
 */
aClient *
make_client (aClient * from, aClient * uplink)
{
  aClient *client_p = NULL;

  if (!from)
  {                             /*
                                 * from is NULL 
                                 */
    client_p = BlockHeapALLOC (free_local_aClients, aClient);

    if (client_p == (aClient *) NULL)
      outofmemory ();

    memset ((char *) client_p, '\0', CLIENT_LOCAL_SIZE);

    /* Note:  structure is zero (calloc) */
    client_p->from = client_p;  /* from' of local client is self! */
    client_p->status = STAT_UNKNOWN;
    client_p->fd = -1;
    client_p->uplink = uplink;
    (void) strcpy (client_p->username, "unknown");
    client_p->since = client_p->lasttime = client_p->firsttime = timeofday;
    client_p->sockerr = -1;
    client_p->authfd = -1;
    client_p->sendqlen = MAXSENDQLENGTH;
#ifdef USE_ADNS
    client_p->dnslookup = NULL;
#endif
    return (client_p);
  }
  else
  {                             /*
                                 * from is not NULL 
                                 */
    client_p = BlockHeapALLOC (free_remote_aClients, aClient);

    if (client_p == (aClient *) NULL)
      outofmemory ();

    memset ((char *) client_p, '\0', CLIENT_REMOTE_SIZE);

    /*
     * Note:  structure is zero (calloc) 
     */
    client_p->from = from;      /* 'from' of local client is self! */
    client_p->status = STAT_UNKNOWN;
    client_p->fd = -1;
    client_p->uplink = uplink;
#ifdef USE_ADNS
    client_p->dnslookup = NULL;
#endif
    /* (void) strcpy(client_p->username, "unknown"); only in local structs now */
    return (client_p);
  }
}

void
free_client (aClient * client_p)
{
  int retval = 0;

  if (client_p->fd == -2)
  {
    retval = BlockHeapFree (free_local_aClients, client_p);
  }
  else
  {
    retval = BlockHeapFree (free_remote_aClients, client_p);
  }
  if (retval)
  {
    /*
     * Looks "unprofessional" maybe, but I am going to leave this
     * sendto_ops in it should never happen, and if it does, the
     * hybrid team wants to hear about it
     */
    sendto_ops
      ("list.c couldn't BlockHeapFree(free_remote_aClients,client_p) client_p = %lX",
       client_p);
    sendto_ops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");

#if defined(USE_SYSLOG) && defined(SYSLOG_BLOCK_ALLOCATOR)
    syslog (LOG_DEBUG,
            "list.c couldn't BlockHeapFree(free_remote_aClients,client_p) client_p = %lX",
            client_p);
#endif
  }
}

/*
 * make_channel() free_channel()
 * functions to maintain blockheap of channels.
 */

aChannel *
make_channel ()
{
  aChannel *chan;

  chan = BlockHeapALLOC (free_channels, aChannel);

  if (chan == NULL)
    outofmemory ();

  memset ((char *) chan, '\0', sizeof (aChannel));

  return chan;
}

void
free_channel (aChannel * chan)
{
  if (BlockHeapFree (free_channels, chan))
  {
    sendto_ops
      ("list.c couldn't BlockHeapFree(free_channels,chan) chan = %lX", chan);
    sendto_ops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
  }
}

/*
 * * 'make_user' add's an User information block to a client * if it
 * was not previously allocated.
 */
anUser *
make_user (aClient * client_p)
{
  anUser *user;

  user = client_p->user;
  if (!user)
  {
    user = BlockHeapALLOC (free_anUsers, anUser);

    if (user == (anUser *) NULL)
      outofmemory ();

    memset (user, 0, sizeof (anUser));
    client_p->user = user;
  }
  return user;
}

aServer *
make_server (aClient * client_p)
{
  aServer *serv = client_p->serv;

  if (!serv)
  {
    serv = (aServer *) MyMalloc (sizeof (aServer));

    memset (serv, 0, sizeof (aServer));
    /*
     *serv->bynick = '\0';
     *serv->byuser = '\0';
     *serv->byhost = '\0';
     serv->up = (char *) NULL;
     */
    client_p->serv = serv;
  }
  return client_p->serv;
}

/*
 * * free_user *      Decrease user reference count by one and
 * release block, *     if count reaches 0
 */
void
free_user (anUser * user, aClient * client_p)
{
  if (user->away)
    MyFree ((char *) user->away);
  /*
   * sanity check
   */
  if (user->joined || user->invited || user->channel)
    sendto_ops ("* %#x user (%s!%s@%s) %#x %#x %#x %d *",
                client_p, client_p ? client_p->name : "<noname>",
                user->username, user->host, user,
                user->invited, user->channel, user->joined);

  if (BlockHeapFree (free_anUsers, user))
  {
    sendto_ops
      ("list.c couldn't BlockHeapFree(free_anUsers,user) user = %lX", user);
    sendto_ops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
#if defined(USE_SYSLOG) && defined(SYSLOG_BLOCK_ALLOCATOR)
    syslog (LOG_DEBUG,
            "list.c couldn't BlockHeapFree(free_anUsers,user) user = %lX",
            user);
#endif
  }
}

/*
 * taken the code from ExitOneClient() for this and placed it here. -
 * avalon
 */
void
remove_client_from_list (aClient * client_p)
{
  if (IsServer (client_p))
    Count.server--;
  else if (IsClient (client_p) && !IsULine (client_p))
  {
    Count.total--;
    if (IsAnOper (client_p))
      Count.oper--;
    if (IsInvisible (client_p))
      Count.invisi--;
  }
  checklist ();
  if (client_p->prev)
    client_p->prev->next = client_p->next;
  else
  {
    client = client_p->next;
    client->prev = NULL;
  }
  if (client_p->next)
    client_p->next->prev = client_p->prev;
  if (IsPerson (client_p) && client_p->user)
  {
    add_history (client_p, 0);
    off_history (client_p);
  }

#ifdef FLUD
  if (MyFludConnect (client_p))
    free_fluders (client_p, NULL);
  free_fludees (client_p);
#endif

  if (client_p->user)
    (void) free_user (client_p->user, client_p);        /*
                                                         * try this here 
                                                         */
  if (client_p->serv)
  {
#ifdef HAVE_ENCRYPTION_ON
    if (client_p->serv->sessioninfo_in)
      dh_end_session (client_p->serv->sessioninfo_in);
    if (client_p->serv->sessioninfo_out)
      dh_end_session (client_p->serv->sessioninfo_out);
    if (client_p->serv->rc4_in)
      rc4_destroystate (client_p->serv->rc4_in);
    if (client_p->serv->rc4_out)
      rc4_destroystate (client_p->serv->rc4_out);
#endif
    if (client_p->serv->zip_in)
      zip_destroy_input_session (client_p->serv->zip_in);
    if (client_p->serv->zip_out)
      zip_destroy_output_session (client_p->serv->zip_out);
    MyFree ((char *) client_p->serv);
  }
  /*
   * YEUCK... This is telling me the only way of knowing that a client_p
   * was pointing to a remote aClient or not is by checking to see if
   * its fd is not -2, that is, unless you look at FLAGS_LOCAL
   * 
   * -Dianora
   * 
   */

  (void) free_client (client_p);
  return;
}

/*
 * although only a small routine, it appears in a number of places as a
 * collection of a few lines...functions like this *should* be in this
 * file, shouldnt they ?  after all, this is list.c, isnt it ? -avalon
 */
void
add_client_to_list (aClient * client_p)
{
  /*
   * since we always insert new clients to the top of the list, this
   * should mean the "me" is the bottom most item in the list.
   */
  client_p->next = client;
  client = client_p;
  if (client_p->next)
    client_p->next->prev = client_p;
  return;
}

/*
 * Look for ptr in the linked listed pointed to by link.
 */
chanMember *
find_user_member (chanMember * cm, aClient * ptr)
{
  if (ptr)
    while (cm)
    {
      if (cm->client_p == ptr)
        return (cm);
      cm = cm->next;
    }
  return ((chanMember *) NULL);
}

Link *
find_channel_link (Link * lp, aChannel * channel_p)
{
  if (channel_p)
    for (; lp; lp = lp->next)
      if (lp->value.channel_p == channel_p)
        return lp;
  return ((Link *) NULL);
}

/*
 * Look for a match in a list of strings. Go through the list, and run
 * match() on it. Side effect: if found, this link is moved to the top of
 * the list.
 */
Link *
find_str_link (Link * lp, char *charptr)
{
  if (!charptr)
    return ((Link *) NULL);
  while (lp != NULL)
  {
    if (!match (lp->value.cp, charptr))
      return lp;
    lp = lp->next;
  }
  return ((Link *) NULL);
}

Link *
make_link ()
{
  Link *lp;
  lp = BlockHeapALLOC (free_Links, Link);

  if (lp == (Link *) NULL)
    outofmemory ();

  lp->next = (Link *) NULL;     /*
                                 * just to be paranoid... 
                                 */

  return lp;
}

void
free_link (Link * lp)
{
  if (BlockHeapFree (free_Links, lp))
  {
    sendto_ops ("list.c couldn't BlockHeapFree(free_Links,lp) lp = %lX", lp);
    sendto_ops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
  }
}

DLink *
make_dlink ()
{
  DLink *lp;
  lp = BlockHeapALLOC (free_DLinks, DLink);

  if (lp == (DLink *) NULL)
    outofmemory ();

  lp->next = (DLink *) NULL;    /* just to be paranoid... */
  lp->prev = (DLink *) NULL;    /* just to be paranoid... */

  return lp;
}

void
free_dlink (DLink * lp)
{
  if (BlockHeapFree (free_DLinks, lp))
  {
    sendto_ops ("list.c couldn't BlockHeapFree(free_DLinks,lp) lp = %lX", lp);
    sendto_ops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
  }
}

chanMember *
make_chanmember ()
{
  chanMember *mp;
  mp = BlockHeapALLOC (free_chanMembers, chanMember);

  if (mp == (chanMember *) NULL)
    outofmemory ();

  return mp;
}

void
free_chanmember (chanMember * mp)
{
  if (BlockHeapFree (free_chanMembers, mp))
  {
    sendto_ops
      ("list.c couldn't BlockHeapFree(free_chanMembers,mp) mp = %lX", mp);
    sendto_ops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
  }
}

aClass *
make_class ()
{
  aClass *tmp;

  tmp = (aClass *) MyMalloc (sizeof (aClass));
  return tmp;
}

void
free_class (tmp)
     aClass *tmp;
{
  MyFree ((char *) tmp);
}

aConfItem *
make_conf ()
{
  aConfItem *aconf;
  aconf = (struct ConfItem *) MyMalloc (sizeof (aConfItem));
  memset ((char *) aconf, '\0', sizeof (aConfItem));

  aconf->status = CONF_ILLEGAL;
  Class (aconf) = 0;

  return (aconf);
}

void
delist_conf (aConfItem * aconf)
{
  if (aconf == conf)
    conf = conf->next;
  else
  {
    aConfItem *bconf;

    for (bconf = conf; aconf != bconf->next; bconf = bconf->next);
    bconf->next = aconf->next;
  }
  aconf->next = NULL;
}

void
free_conf (aConfItem * aconf)
{
#ifdef USE_ADNS
  delete_adns_queries (aconf->dnslookup);
#else
  del_queries ((char *) aconf);
#endif
  MyFree (aconf->host);
  if (aconf->passwd)
    memset (aconf->passwd, '\0', strlen (aconf->passwd));
  MyFree (aconf->passwd);
  MyFree (aconf->name);
  MyFree ((char *) aconf);
  return;
}

/*
 * Attempt to free up some block memory
 * 
 * list_garbage_collect
 * 
 * inputs               - NONE output           - NONE side effects     -
 * memory is possibly freed up
 */

void
block_garbage_collect ()
{
  BlockHeapGarbageCollect (free_Links);
  BlockHeapGarbageCollect (free_DLinks);
  BlockHeapGarbageCollect (free_chanMembers);
  BlockHeapGarbageCollect (free_local_aClients);
  BlockHeapGarbageCollect (free_remote_aClients);
  BlockHeapGarbageCollect (free_anUsers);
  BlockHeapGarbageCollect (free_channels);
#ifdef FLUD
  BlockHeapGarbageCollect (free_fludbots);
#endif /*
        * FLUD 
        */
}
