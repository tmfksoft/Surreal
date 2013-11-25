/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/hash.c
 *
 *  Copyright (C) 1991 Darren Reed
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
 *  $Id: hash.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "hash.h"
#include "h.h"

static aHashEntry clientTable[U_MAX];
static aHashEntry channelTable[CH_MAX];

static unsigned hash_nick_name (char *);
static int hash_channel_name (char *);


/*
 * look in whowas.c for the missing ...[WW_MAX]; entry - Dianora */
/*
 * Hashing.
 * 
 * The server uses a chained hash table to provide quick and efficient
 * hash table mantainence (providing the hash function works evenly
 * over the input range).  The hash table is thus not susceptible to
 * problems of filling all the buckets or the need to rehash. It is
 * expected that the hash table would look somehting like this during
 * use: +-----+    +-----+    +-----+   +-----+ 
 *   ---| 224 |----| 225 |----| 226 |---| 227 |--- 
 *      +-----+    +-----+    +-----+   +-----+ 
 *         |          |          | 
 *      +-----+    +-----+    +-----+ 
 *      |  A  |    |  C  |    |  D  | 
 *      +-----+    +-----+    +-----+ 
 *         | 
 *      +-----+ 
 *      |  B  | 
 *      +-----+
 * 
 * A - GOPbot, B - chang, C - hanuaway, D - *.mu.OZ.AU
 * 
 * The order shown above is just one instant of the server.  Each time a
 * lookup is made on an entry in the hash table and it is found, the
 * entry is moved to the top of the chain.
 * 
 * ^^^^^^^^^^^^^^^^ **** Not anymore - Dianora
 * 
 */

unsigned
hash_nick_name (char *nname)
{
  unsigned hash = 0;
  int hash2 = 0;
  int ret;
  char lower;

  while (*nname)
  {
    lower = MyToLower (*nname);
    hash = (hash << 1) + lower;
    hash2 = (hash2 >> 1) + lower;
    nname++;
  }
  ret = ((hash & U_MAX_INITIAL_MASK) << BITS_PER_COL) +
    (hash2 & BITS_PER_COL_MASK);
  return ret;
}

/*
 * hash_channel_name
 * 
 * calculate a hash value on at most the first 30 characters of the
 * channel name. Most names are short than this or dissimilar in this
 * range. There is little or no point hashing on a full channel name
 * which maybe 255 chars long.
 */
int
hash_channel_name (char *name)
{
  unsigned char *hname = (unsigned char *) name;
  unsigned int hash = 0;
  int hash2 = 0;
  char lower;
  int i = 30;

  while (*hname && --i)
  {
    lower = MyToLower (*hname);
    hash = (hash << 1) + lower;
    hash2 = (hash2 >> 1) + lower;
    hname++;
  }
  return ((hash & CH_MAX_INITIAL_MASK) << BITS_PER_COL) +
    (hash2 & BITS_PER_COL_MASK);
}

unsigned int
hash_whowas_name (char *name)
{
  unsigned char *nname = (unsigned char *) name;
  unsigned int hash = 0;
  int hash2 = 0;
  int ret;
  char lower;

  while (*nname)
  {
    lower = MyToLower (*nname);
    hash = (hash << 1) + lower;
    hash2 = (hash2 >> 1) + lower;
    nname++;
  }
  ret = ((hash & WW_MAX_INITIAL_MASK) << BITS_PER_COL) +
    (hash2 & BITS_PER_COL_MASK);
  return ret;
}

/*
 * clear_*_hash_table
 * 
 * Nullify the hashtable and its contents so it is completely empty.
 */
void
clear_client_hash_table ()
{
  memset ((char *) clientTable, '\0', sizeof (aHashEntry) * U_MAX);
}

void
clear_channel_hash_table ()
{
  memset ((char *) channelTable, '\0', sizeof (aHashEntry) * CH_MAX);
}

/*
 * add_to_client_hash_table
 */
int
add_to_client_hash_table (char *name, aClient * client_p)
{
  int hashv;

  hashv = hash_nick_name (name);
  client_p->hnext = (aClient *) clientTable[hashv].list;
  clientTable[hashv].list = (void *) client_p;
  clientTable[hashv].links++;
  clientTable[hashv].hits++;
  return 0;
}

/*
 * add_to_channel_hash_table
 */
int
add_to_channel_hash_table (char *name, aChannel * channel_p)
{
  int hashv;

  hashv = hash_channel_name (name);
  channel_p->hnextch = (aChannel *) channelTable[hashv].list;
  channelTable[hashv].list = (void *) channel_p;
  channelTable[hashv].links++;
  channelTable[hashv].hits++;
  return 0;
}

/*
 * del_from_client_hash_table
 */
int
del_from_client_hash_table (char *name, aClient * client_p)
{
  aClient *tmp, *prev = NULL;
  int hashv;

  hashv = hash_nick_name (name);
  for (tmp = (aClient *) clientTable[hashv].list; tmp; tmp = tmp->hnext)
  {
    if (tmp == client_p)
    {
      if (prev)
        prev->hnext = tmp->hnext;
      else
        clientTable[hashv].list = (void *) tmp->hnext;
      tmp->hnext = NULL;
      if (clientTable[hashv].links > 0)
      {
        clientTable[hashv].links--;
        return 1;
      }
      else
        /*
         * Should never actually return from here and if we do it
         * is an error/inconsistency in the hash table.
         */
        return -1;
    }
    prev = tmp;
  }
  return 0;
}

/*
 * del_from_channel_hash_table
 */
int
del_from_channel_hash_table (char *name, aChannel * channel_p)
{
  aChannel *tmp, *prev = NULL;
  int hashv;

  hashv = hash_channel_name (name);
  for (tmp = (aChannel *) channelTable[hashv].list; tmp; tmp = tmp->hnextch)
  {
    if (tmp == channel_p)
    {
      if (prev)
        prev->hnextch = tmp->hnextch;
      else
        channelTable[hashv].list = (void *) tmp->hnextch;
      tmp->hnextch = NULL;
      if (channelTable[hashv].links > 0)
      {
        channelTable[hashv].links--;
        return 1;
      }
      else
        return -1;
    }
    prev = tmp;
  }
  return 0;
}

/*
 * hash_find_client
 */
aClient *
hash_find_client (char *name, aClient * client_p)
{
  aClient *tmp;
  aHashEntry *tmp3;
  int hashv;

  hashv = hash_nick_name (name);
  tmp3 = &clientTable[hashv];
  /*
   * Got the bucket, now search the chain.
   */
  for (tmp = (aClient *) tmp3->list; tmp; tmp = tmp->hnext)
    if (irccmp (name, tmp->name) == 0)
    {
      return (tmp);
    }
  return (client_p);
  /*
   * If the member of the hashtable we found isnt at the top of its
   * chain, put it there.  This builds a most-frequently used order
   * into the chains of the hash table, giving speedier lookups on
   * those nicks which are being used currently.  This same block of
   * code is also used for channels and servers for the same
   * performance reasons.
   * 
   * I don't believe it does.. it only wastes CPU, lets try it and
   * see....
   * 
   * - Dianora
   */
}

/*
 * hash_find_nickserver
 */
aClient *
hash_find_nickserver (char *name, aClient * client_p)
{
  aClient *tmp;
  aHashEntry *tmp3;
  int hashv;
  char *serv;

  serv = strchr (name, '@');
  *serv++ = '\0';
  hashv = hash_nick_name (name);
  tmp3 = &clientTable[hashv];
  /*
   * Got the bucket, now search the chain.
   */
  for (tmp = (aClient *) tmp3->list; tmp; tmp = tmp->hnext)
    if (irccmp (name, tmp->name) == 0 && tmp->user &&
        irccmp (serv, tmp->user->server) == 0)
    {
      *--serv = '\0';
      return (tmp);
    }

  *--serv = '\0';
  return (client_p);
}

/*
 * hash_find_server
 */
aClient *
hash_find_server (char *server, aClient * client_p)
{
  aClient *tmp;
  aHashEntry *tmp3;

  int hashv;

  hashv = hash_nick_name (server);
  tmp3 = &clientTable[hashv];

  for (tmp = (aClient *) tmp3->list; tmp; tmp = tmp->hnext)
  {
    if (!IsServer (tmp) && !IsMe (tmp))
      continue;
    if (irccmp (server, tmp->name) == 0)
    {
      return (tmp);
    }
  }
  return (client_p);
}

/*
 * hash_find_channel
 */
aChannel *
hash_find_channel (char *name, aChannel * channel_p)
{
  int hashv;
  aChannel *tmp;
  aHashEntry *tmp3;

  hashv = hash_channel_name (name);
  tmp3 = &channelTable[hashv];

  for (tmp = (aChannel *) tmp3->list; tmp; tmp = tmp->hnextch)
    if (irccmp (name, tmp->chname) == 0)
    {
      return (tmp);
    }
  return channel_p;
}

#ifdef UNUSED
/* This function is useless! 
** Fish (23/08/2003)
*/
/*
 * NOTE: this command is not supposed to be an offical part of the ircd
 * protocol.  It is simply here to help debug and to monitor the
 * performance of the hash functions and table, enabling a better
 * algorithm to be sought if this one becomes troublesome. -avalon
 * 
 * Needs rewriting for DOUGH_HASH, consider this a place holder until
 * thats done. Hopefully for hybrid-5, if not. tough. - Dianora
 * 
 */

int
m_hash (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  return 0;
}
#endif
/*
 * Rough figure of the datastructures for notify:
 *
 * NOTIFY HASH      client_p1
 *   |                |- nick1
 * nick1-|- client_p1     |- nick2
 *   |   |- client_p2                client_p3
 *   |   |- client_p3   client_p2          |- nick1
 *   |                |- nick1
 * nick2-|- client_p2     |- nick2
 *       |- client_p1
 *
 * add-to-notify-hash-table:
 * del-from-notify-hash-table:
 * hash-del-notify-list:
 * hash-check-notify:
 * hash-get-notify:
 */

static aWatch *watchTable[WATCHHASHSIZE];

void
count_watch_memory (count, memory)
     int *count;
     u_long *memory;
{
  int i = WATCHHASHSIZE;
  aWatch *anptr;


  while (i--)
  {
    anptr = watchTable[i];
    while (anptr)
    {
      (*count)++;
      (*memory) += sizeof (aWatch) + strlen (anptr->nick);
      anptr = anptr->hnext;
    }
  }
}

void
clear_watch_hash_table (void)
{
  memset ((char *) watchTable, '\0', sizeof (watchTable));
}


/*
 * add_to_watch_hash_table
 */
int
add_to_watch_hash_table (nick, client_p)
     char *nick;
     aClient *client_p;
{
  int hashv;
  aWatch *anptr;
  Link *lp;


  /* Get the right bucket... */
  hashv = hash_nick_name (nick) % WATCHHASHSIZE;

  /* Find the right nick (header) in the bucket, or NULL... */
  if ((anptr = (aWatch *) watchTable[hashv]))
    while (anptr && (irccmp (anptr->nick, nick) != 0))
      anptr = anptr->hnext;

  /* If found NULL (no header for this nick), make one... */
  if (!anptr)
  {
    anptr = (aWatch *) MyMalloc (sizeof (aWatch) + strlen (nick));
    anptr->lasttime = timeofday;
    strcpy (anptr->nick, nick);

    anptr->watch = NULL;

    anptr->hnext = watchTable[hashv];
    watchTable[hashv] = anptr;
  }
  /* Is this client already on the watch-list? */
  if ((lp = anptr->watch))
    while (lp && (lp->value.client_p != client_p))
      lp = lp->next;

  /* No it isn't, so add it in the bucket and client addint it */
  if (!lp)
  {
    lp = anptr->watch;
    anptr->watch = make_link ();
    anptr->watch->value.client_p = client_p;
    anptr->watch->next = lp;

    lp = make_link ();
    lp->next = client_p->watch;
    lp->value.wptr = anptr;
    client_p->watch = lp;
    client_p->watches++;
  }

  return 0;
}

/*
 *  hash_check_watch
 */
int
hash_check_watch (client_p, reply)
     aClient *client_p;
     int reply;
{
  int hashv;
  aWatch *anptr;
  Link *lp;


  /* Get us the right bucket */
  hashv = hash_nick_name (client_p->name) % WATCHHASHSIZE;

  /* Find the right header in this bucket */
  if ((anptr = (aWatch *) watchTable[hashv]))
    while (anptr && (irccmp (anptr->nick, client_p->name) != 0))
      anptr = anptr->hnext;
  if (!anptr)
    return 0;                   /* This nick isn't on watch */

  /* Update the time of last change to item */
  anptr->lasttime = NOW;

  /* Send notifies out to everybody on the list in header */
  for (lp = anptr->watch; lp; lp = lp->next)
    sendto_one (lp->value.client_p, rpl_str (reply), me.name,
                lp->value.client_p->name, client_p->name,
                (IsPerson (client_p) ? client_p->user->username : "<N/A>"),
                (IsPerson (client_p)
                 ? (IsHidden (client_p) ? client_p->user->
                    virthost : client_p->user->host) : "<N/A>"),
                anptr->lasttime, client_p->info);

  return 0;
}

/*
 * hash_get_watch
 */
aWatch *
hash_get_watch (name)
     char *name;
{
  int hashv;
  aWatch *anptr;


  hashv = hash_nick_name (name) % WATCHHASHSIZE;

  if ((anptr = (aWatch *) watchTable[hashv]))
    while (anptr && (irccmp (anptr->nick, name) != 0))
      anptr = anptr->hnext;

  return anptr;
}

/*
 * del_from_watch_hash_table
 */
int
del_from_watch_hash_table (nick, client_p)
     char *nick;
     aClient *client_p;
{
  int hashv;
  aWatch *anptr, *nlast = NULL;
  Link *lp, *last = NULL;


  /* Get the bucket for this nick... */
  hashv = hash_nick_name (nick) % WATCHHASHSIZE;

  /* Find the right header, maintaining last-link pointer... */
  if ((anptr = (aWatch *) watchTable[hashv]))
    while (anptr && (irccmp (anptr->nick, nick) != 0))
    {
      nlast = anptr;
      anptr = anptr->hnext;
    }
  if (!anptr)
    return 0;                   /* No such watch */

  /* Find this client from the list of notifies... with last-ptr. */
  if ((lp = anptr->watch))
    while (lp && (lp->value.client_p != client_p))
    {
      last = lp;
      lp = lp->next;
    }
  if (!lp)
    return 0;                   /* No such client to watch */

  /* Fix the linked list under header, then remove the watch entry */
  if (!last)
    anptr->watch = lp->next;
  else
    last->next = lp->next;
  free_link (lp);

  /* Do the same regarding the links in client-record... */
  last = NULL;
  if ((lp = client_p->watch))
    while (lp && (lp->value.wptr != anptr))
    {
      last = lp;
      lp = lp->next;
    }

  /*
   * Give error on the odd case... probobly not even neccessary
   * No error checking in ircd is unneccessary ;) -Cabal95
   */
  if (!lp)
    sendto_ops ("WATCH debug error: del_from_watch_hash_table "
                "found a watch entry with no client "
                "counterpoint processing nick %s on client %s!",
                nick, client_p->user);
  else
  {
    if (!last)                  /* First one matched */
      client_p->watch = lp->next;
    else
      last->next = lp->next;
    free_link (lp);
  }
  /* In case this header is now empty of notices, remove it */
  if (!anptr->watch)
  {
    if (!nlast)
      watchTable[hashv] = anptr->hnext;
    else
      nlast->hnext = anptr->hnext;
    MyFree (anptr);
  }

  /* Update count of notifies on nick */
  client_p->watches--;

  return 0;
}

/*
 * hash_del_watch_list
 */
int
hash_del_watch_list (client_p)
     aClient *client_p;
{
  int hashv;
  aWatch *anptr;
  Link *np, *lp, *last;


  if (!(np = client_p->watch))
    return 0;                   /* Nothing to do */

  client_p->watch = NULL;       /* Break the watch-list for client */
  while (np)
  {
    /* Find the watch-record from hash-table... */
    anptr = np->value.wptr;
    last = NULL;
    for (lp = anptr->watch; lp && (lp->value.client_p != client_p);
         lp = lp->next)
      last = lp;

    /* Not found, another "worst case" debug error */
    if (!lp)
      sendto_ops ("WATCH Debug error: hash_del_watch_list "
                  "found a WATCH entry with no table "
                  "counterpoint processing client %s!", client_p->name);
    else
    {
      /* Fix the watch-list and remove entry */
      if (!last)
        anptr->watch = lp->next;
      else
        last->next = lp->next;
      free_link (lp);

      /*
       * If this leaves a header without notifies,
       * remove it. Need to find the last-pointer!
       */
      if (!anptr->watch)
      {
        aWatch *np2, *nl;

        hashv = hash_nick_name (anptr->nick) % WATCHHASHSIZE;

        nl = NULL;
        np2 = watchTable[hashv];
        while (np2 != anptr)
        {
          nl = np2;
          np2 = np2->hnext;
        }

        if (nl)
          nl->hnext = anptr->hnext;
        else
          watchTable[hashv] = anptr->hnext;
        MyFree (anptr);
      }
    }

    lp = np;                    /* Save last pointer processed */
    np = np->next;              /* Jump to the next pointer */
    free_link (lp);             /* Free the previous */
  }

  client_p->watches = 0;

  return 0;
}

aChannel *
hash_get_chan_bucket (hashv)
     int hashv;
{
  if (hashv > CH_MAX)
    return NULL;
  return (aChannel *) channelTable[hashv].list;
}
