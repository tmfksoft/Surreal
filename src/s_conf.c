/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_conf.c
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
 *  $Id: s_conf.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "inet.h"
#ifndef _WIN32
# include <sys/socket.h>
# include <sys/wait.h>
#endif
#include <fcntl.h>
#ifdef __hpux
# include "inet.h"
#endif
#if defined(AIX) || defined(SVR3)
# include <time.h>
#endif

#include <signal.h>
#include "h.h"
#include "dich_conf.h"
#include "userban.h"
#include "msg.h"

#ifdef INET6
static unsigned char minus_one[] =
  { 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 0
};
#endif

extern int rehashed;

struct SOCKADDR_IN vserv;
char specific_virtual_host;


/* internally defined functions  */

static int lookup_confhost (aConfItem *);
static int attach_iline (aClient *, aConfItem *, char *);
static aConfItem *find_conf_entry (aConfItem *, int);


/* externally defined functions  */

extern void outofmemory (void); /* defined in list.c */

/*
 * usually, with hash tables, you use a prime number... but in this
 * case I am dealing with ip addresses, not ascii strings.
 */

#define IP_HASH_SIZE 0x1000

typedef struct ip_entry
{
#ifndef INET6
  unsigned long ip;
#else
  char ip[sizeof (struct IN_ADDR)];
#endif
  int count;
  struct ip_entry *next;
}
IP_ENTRY;

IP_ENTRY *ip_hash_table[IP_HASH_SIZE];

void init_ip_hash (void);
#ifndef INET6
static int hash_ip (unsigned long);
static IP_ENTRY *find_or_add_ip (unsigned long);
#else
static int hash_ip (char *);
static IP_ENTRY *find_or_add_ip (char *);
#endif

/* externally defined routines */

int find_conf_match (aClient *, aConfList *, aConfList *, aConfList *);
int find_fline (aClient *);


aConfItem *conf = ((aConfItem *) NULL);

#ifdef LOCKFILE
extern void do_pending_klines (void);
#endif

/*
 * remove all conf entries from the client except those which match the
 * status field mask.
 */

void
det_confs_butmask (aClient * client_p, int mask)
{
  Link *tmp, *tmp2;

  for (tmp = client_p->confs; tmp; tmp = tmp2)
  {
    tmp2 = tmp->next;
    if ((tmp->value.aconf->status & mask) == 0)
      (void) detach_conf (client_p, tmp->value.aconf);
  }
}

/* find the first (best) I line to attach. */

int
#ifdef USE_ADNS
attach_Iline (aClient * client_p, char *sockhost)
#else
attach_Iline (aClient * client_p, struct hostent *hp, char *sockhost)
#endif
{
  aConfItem *aconf;
#ifdef USE_ADNS
  struct DNSQuery *dnslookup = client_p->dnslookup;
#else
  char *hname;
  int i;
#endif
  static char uhost[HOSTLEN + USERLEN + 3];
  static char uhost2[HOSTLEN + USERLEN + 3];
  static char fullname[HOSTLEN + 1];


  for (aconf = conf; aconf; aconf = aconf->next)
  {
    if (aconf->status != CONF_CLIENT)
      continue;

    if (aconf->port && aconf->port != client_p->acpt->port)
      continue;

    if (!aconf->host || !aconf->name)
      return (attach_iline (client_p, aconf, uhost));

#ifdef USE_ADNS
    if ((dnslookup->answer) && (dnslookup->answer->status == adns_s_ok))
    {
      (void) strncpy (fullname, (char *) *dnslookup->answer->rrs.str,
                      sizeof (fullname) - 1);
#else
    if (hp)
      for (i = 0, hname = hp->h_name; hname; hname = hp->h_aliases[i++])
      {
        (void) strncpy (fullname, hname, sizeof (fullname) - 1);
#endif
        add_local_domain (fullname, HOSTLEN - strlen (fullname));
        Debug ((DEBUG_DNS, "a_il: %s->%s", sockhost, fullname));
        if (strchr (aconf->name, '@'))
        {
          (void) strcpy (uhost, client_p->username);
          (void) strcat (uhost, "@");
          (void) strcpy (uhost2, client_p->username);
          (void) strcat (uhost2, "@");
        }
        else
        {
          *uhost = '\0';
          *uhost2 = '\0';
        }
        (void) strncat (uhost, fullname, sizeof (uhost) - strlen (uhost));
        (void) strncat (uhost2, sockhost, sizeof (uhost2) - strlen (uhost2));
        if ((!match (aconf->name, uhost)) || (!match (aconf->name, uhost2)))
          return (attach_iline (client_p, aconf, uhost));
      }

    if (strchr (aconf->host, '@'))
    {
      strncpyzt (uhost, client_p->username, USERLEN + 1);
      (void) strcat (uhost, "@");
    }
    else
      *uhost = '\0';
    (void) strncat (uhost, sockhost, sizeof (uhost) - strlen (uhost));

    if (match (aconf->host, uhost) == 0)
      return (attach_iline (client_p, aconf, uhost));
  }

  return -1;                    /* no match */
}

/*
 * rewrote to remove the "ONE" lamity *BLEH* I agree with comstud on
 * this one. - Dianora
 */
static int
attach_iline (aClient * client_p, aConfItem * aconf, char *uhost)
{
  IP_ENTRY *ip_found;

  if (strchr (uhost, '@'))
    client_p->flags |= FLAGS_DOID;
  get_sockhost (client_p, uhost);

  /* every conf when created, has a class pointer set up. if it isn't,
   * well.  *BOOM* ! */

  ip_found = find_or_add_ip (client_p->ip.S_ADDR);

  client_p->flags |= FLAGS_IPHASH;
  ip_found->count++;

  /* only check it if its non zero  */

  if ((aconf->class->conFreq) && (ip_found->count > aconf->class->conFreq))
    return -4;                  /* Already at maximum allowed ip#'s  */

  return (attach_conf (client_p, aconf));
}

/* link list of free IP_ENTRY's */

static IP_ENTRY *free_ip_entries;

/*
 * init_ip_hash()
 * 
 * input 		- NONE output           - NONE side effects     - clear
 * the ip hash table
 * 
 * stole the link list pre-allocator from list.c
 */

void
clear_ip_hash_table ()
{
  void *block_IP_ENTRIES;       /* block of IP_ENTRY's  */
  IP_ENTRY *new_IP_ENTRY;       /* new IP_ENTRY being made */
  IP_ENTRY *last_IP_ENTRY;      /* last IP_ENTRY in chain */
  int size, n_left_to_allocate = MAXCONNECTIONS;

  /*
   * ok. if the sizeof the struct isn't aligned with that of the
   * smallest guaranteed valid pointer (void *), then align it ya. you
   * could just turn 'size' into a #define. do it. :-)
   * 
   * -Dianora
   */

  size = sizeof (IP_ENTRY) + (sizeof (IP_ENTRY) & (sizeof (void *) - 1));

  block_IP_ENTRIES = (void *) MyMalloc ((size * n_left_to_allocate));

  free_ip_entries = (IP_ENTRY *) block_IP_ENTRIES;
  last_IP_ENTRY = free_ip_entries;

  /*
   * *shudder* pointer arithmetic 
   */
  while (--n_left_to_allocate)
  {
    block_IP_ENTRIES = (void *) ((unsigned long) block_IP_ENTRIES +
                                 (unsigned long) size);
    new_IP_ENTRY = (IP_ENTRY *) block_IP_ENTRIES;
    last_IP_ENTRY->next = new_IP_ENTRY;
    new_IP_ENTRY->next = (IP_ENTRY *) NULL;
    last_IP_ENTRY = new_IP_ENTRY;
  }
  memset ((char *) ip_hash_table, '\0', sizeof (ip_hash_table));
}

/*
 * find_or_add_ip()
 * 
 * inputs               - unsigned long IP address value output         -
 * pointer to an IP_ENTRY element side effects  -
 * 
 * If the ip # was not found, a new IP_ENTRY is created, and the ip count
 * set to 0.
 */

#ifndef INET6
static IP_ENTRY *
find_or_add_ip (unsigned long ip_in)
#else
static IP_ENTRY *
find_or_add_ip (char *ip_in)
#endif
{
  int hash_index;
  IP_ENTRY *ptr, *newptr;

  newptr = (IP_ENTRY *) NULL;
  ptr = ip_hash_table[hash_index = hash_ip (ip_in)];
  while (ptr)
  {
#ifndef INET6
    if (ptr->ip == ip_in)
#else
    if (!memcmp (ptr->ip, ip_in, sizeof (struct IN_ADDR)))
#endif
      return (ptr);
    else
      ptr = ptr->next;
  }

  if ((ptr = ip_hash_table[hash_index]) != (IP_ENTRY *) NULL)
  {
    if (free_ip_entries == (IP_ENTRY *) NULL)   /* it might be */
    {                           /* recoverable */
      sendto_ops ("s_conf.c free_ip_entries was found NULL in find_or_add");
      sendto_ops ("rehash_ip was done, this is an error.");
      sendto_ops
        ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
      rehash_ip_hash ();
      if (free_ip_entries == (IP_ENTRY *) NULL)
        outofmemory ();
    }

    newptr = ip_hash_table[hash_index] = free_ip_entries;
    free_ip_entries = newptr->next;

#ifndef INET6
    newptr->ip = ip_in;
#else
    memcpy (newptr->ip, ip_in, sizeof (struct IN_ADDR));
#endif
    newptr->count = 0;
    newptr->next = ptr;
    return (newptr);
  }
  else
  {
    if (free_ip_entries == (IP_ENTRY *) NULL)   /* it might be */
    {                           /* recoverable */
      sendto_ops ("s_conf.c free_ip_entries was found NULL in find_or_add");
      sendto_ops ("rehash_ip was done, this is an error.");
      sendto_ops
        ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
      rehash_ip_hash ();
      if (free_ip_entries == (IP_ENTRY *) NULL)
        outofmemory ();
    }

    ptr = ip_hash_table[hash_index] = free_ip_entries;
    free_ip_entries = ptr->next;
#ifndef INET6
    ptr->ip = ip_in;
#else
    memcpy (ptr->ip, ip_in, sizeof (struct IN_ADDR));
#endif
    ptr->count = 0;
    ptr->next = (IP_ENTRY *) NULL;
    return (ptr);
  }
}

/*
 * remove_one_ip
 * 
 * inputs               - unsigned long IP address value output         - NONE
 * side effects - ip address listed, is looked up in ip hash table and
 * number of ip#'s for that ip decremented. if ip # count reaches 0,
 * the IP_ENTRY is returned to the free_ip_enties link list.
 */

#ifndef INET6
void
remove_one_ip (unsigned long ip_in)
#else
void
remove_one_ip (char *ip_in)
#endif
{
  int hash_index;
  IP_ENTRY *last_ptr, *ptr, *old_free_ip_entries;

  last_ptr = ptr = ip_hash_table[hash_index = hash_ip (ip_in)];
  while (ptr)
  {
#ifndef INET6
    if (ptr->ip == ip_in)
#else
    if (!memcmp (ptr->ip, ip_in, sizeof (struct IN_ADDR)))
#endif /* ! */
    {
      if (ptr->count != 0)
        ptr->count--;
      if (ptr->count == 0)
      {
        if (ip_hash_table[hash_index] == ptr)
          ip_hash_table[hash_index] = ptr->next;
        else
          last_ptr->next = ptr->next;

        if (free_ip_entries != (IP_ENTRY *) NULL)
        {
          old_free_ip_entries = free_ip_entries;
          free_ip_entries = ptr;
          ptr->next = old_free_ip_entries;
        }
        else
        {
          free_ip_entries = ptr;
          ptr->next = (IP_ENTRY *) NULL;
        }
      }
      return;
    }
    else
    {
      last_ptr = ptr;
      ptr = ptr->next;
    }
  }
  sendto_ops ("s_conf.c couldn't find ip# in hash table in remove_one_ip()");
  sendto_ops
    ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
  return;
}

/*
 * hash_ip()
 *
 * input                - unsigned long ip address output               -
 * integer value used as index into hash table side effects     -
 * hopefully, none
 */

#ifndef INET6
static int
hash_ip (unsigned long ip)
{
  int hash;

  ip = ntohl (ip);
  hash = ((ip >> 12) + ip) & (IP_HASH_SIZE - 1);
  return (hash);
}
#else
static int
hash_ip (char *ip)
{
  unsigned int hash = 0;
  uint16_t *ipp = (uint16_t *) ip;
  hash = ((((((((((((((((ipp[0]) << 1) -
                       ipp[1]) << 1) +
                     ipp[2]) << 1) -
                   ipp[3]) << 1) +
                 ipp[4]) << 1) - ipp[5]) << 1) + ipp[6]) << 1) - ipp[7]));

  return (hash & (IP_HASH_SIZE - 1));
}
#endif

/*
 * count_ip_hash
 *
 * inputs               - pointer to counter of number of ips hashed - pointer
 * to memory used for ip hash output            - returned via pointers
 * input side effects   - NONE
 * 
 * number of hashed ip #'s is counted up, plus the amount of memory used
 * in the hash.
 * Added so s_debug could check memory usage in here -Dianora 
 */

void
count_ip_hash (int *number_ips_stored, u_long * mem_ips_stored)
{
  IP_ENTRY *ip_hash_ptr;
  int i;

  *number_ips_stored = 0;
  *mem_ips_stored = 0;

  for (i = 0; i < IP_HASH_SIZE; i++)
  {
    ip_hash_ptr = ip_hash_table[i];
    while (ip_hash_ptr)
    {
      *number_ips_stored = *number_ips_stored + 1;
      *mem_ips_stored = *mem_ips_stored + sizeof (IP_ENTRY);

      ip_hash_ptr = ip_hash_ptr->next;
    }
  }
}

/*
 * rehash_ip_hash
 * 
 * inputs               - NONE output           - NONE side effects     -
 * 
 * This function clears the ip hash table, then re-enters all the ip's
 * found in local clients.
 * 
 * N.B. This should never have to be called, and hopefully, we can remove
 * this function in future versions of hybrid. i.e. if everything is
 * working right, there should never ever be a case where an IP is
 * still in the ip_hash table and the corresponding client isn't.
 * 
 */

void
rehash_ip_hash ()
{
  IP_ENTRY *ip_hash_ptr;
  IP_ENTRY *tmp_ip_hash_ptr;
  IP_ENTRY *old_free_ip_entries;
  int i;

  /* first, clear the ip hash */

  for (i = 0; i < IP_HASH_SIZE; i++)
  {
    ip_hash_ptr = ip_hash_table[i];
    while (ip_hash_ptr)
    {
      tmp_ip_hash_ptr = ip_hash_ptr->next;
      if (free_ip_entries)
      {
        old_free_ip_entries = free_ip_entries;
        free_ip_entries = ip_hash_ptr;
        ip_hash_ptr->next = old_free_ip_entries;
      }
      else
      {
        free_ip_entries = ip_hash_ptr;
        ip_hash_ptr->next = (IP_ENTRY *) NULL;
      }
      ip_hash_ptr = tmp_ip_hash_ptr;
    }
    ip_hash_table[i] = (IP_ENTRY *) NULL;
  }

  for (i = highest_fd; i >= 0; i--)
  {
    if (local[i] && MyClient (local[i]))
    {
      if ((local[i]->fd >= 0) && (local[i]->flags & FLAGS_IPHASH))
      {
        ip_hash_ptr = find_or_add_ip (local[i]->ip.S_ADDR);
        ip_hash_ptr->count++;
      }
    }
  }
}

/*
 * Find the single N line and return pointer to it (from list). If more
 * than one then return NULL pointer.
 */
aConfItem *
count_cnlines (Link * lp)
{
  aConfItem *aconf, *cline = NULL, *nline = NULL;

  for (; lp; lp = lp->next)
  {
    aconf = lp->value.aconf;
    if (!(aconf->status & CONF_SERVER_MASK))
      continue;
    if (aconf->status == CONF_CONNECT_SERVER && !cline)
      cline = aconf;
    else if (aconf->status == CONF_NOCONNECT_SERVER && !nline)
      nline = aconf;
  }
  return nline;
}

/*
 * * detach_conf 
 *    Disassociate configuration from the client. 
 *    Also removes a class from the list if marked for deleting.
 */
int
detach_conf (aClient * client_p, aConfItem * aconf)
{
  Link **lp, *tmp;

  lp = &(client_p->confs);

  while (*lp)
  {
    if ((*lp)->value.aconf == aconf)
    {
      if ((aconf) && (Class (aconf)))
      {
        if (aconf->status & CONF_CLIENT_MASK)
          if (ConfLinks (aconf) > 0)
            --ConfLinks (aconf);
        if (ConfMaxLinks (aconf) == -1 && ConfLinks (aconf) == 0)
        {
          free_class (Class (aconf));
          Class (aconf) = NULL;
        }
      }
      if (aconf && !--aconf->clients && IsIllegal (aconf))
        free_conf (aconf);
      tmp = *lp;
      *lp = tmp->next;
      free_link (tmp);
      return 0;
    }
    else
      lp = &((*lp)->next);
  }
  return -1;
}

static int
is_attached (aConfItem * aconf, aClient * client_p)
{
  Link *lp;

  for (lp = client_p->confs; lp; lp = lp->next)
    if (lp->value.aconf == aconf)
      break;

  return (lp) ? 1 : 0;
}

/*
 * * attach_conf 
 *    Associate a specific configuration entry to a *local* 
 *    client (this is the one which used in accepting the 
 *    connection). Note, that this automatically changes the
 *    attachment if there was an old one...
 */
int
attach_conf (aClient * client_p, aConfItem * aconf)
{
  Link *lp;

  if (is_attached (aconf, client_p))
  {
    return 1;
  }
  if (IsIllegal (aconf))
    return -1;

  /*
   * By using "ConfLinks(aconf) >= ConfMaxLinks(aconf).... the client
   * limit is set by the Y line, connection class, not by the
   * individual client count in each I line conf.
   *
   * If the requested change, is to turn them into an OPER, then they
   * are already attached to a fd there is no need to check for max in
   * a class now is there?
   *
   * If OLD_Y_LIMIT is defined the code goes back to the old way I
   * lines used to work, i.e. number of clients per I line not total
   * in Y
   * -Dianora
   */

#ifdef OLD_Y_LIMIT
  if ((aconf->status & (CONF_LOCOP | CONF_OPERATOR | CONF_CLIENT)) &&
      aconf->clients >= ConfMaxLinks (aconf) && ConfMaxLinks (aconf) > 0)
#else
  if ((aconf->status & (CONF_LOCOP | CONF_OPERATOR)) == 0)
  {
    if ((aconf->status & CONF_CLIENT) &&
        ConfLinks (aconf) >= ConfMaxLinks (aconf) && ConfMaxLinks (aconf) > 0)
#endif
    {
      if (!IsSuperExempt (client_p))
        return -3;              /* Use this for printing error message */
      else
      {
#ifdef SHOW_HEADERS
#ifdef USE_SSL
        if (!IsSSL (client_p))
        {
#endif
          send (client_p->fd,
                "NOTICE FLINE :*** I: line is full, but you have an F: line!\n",
                60, 0);
#ifdef USE_SSL
        }
#endif
#endif
      }
    }
#ifndef OLD_Y_LIMIT
  }
#endif

  lp = make_link ();
  lp->next = client_p->confs;
  lp->value.aconf = aconf;
  client_p->confs = lp;
  aconf->clients++;
  if (aconf->status & CONF_CLIENT_MASK)
    ConfLinks (aconf)++;
  return 0;
}

aConfItem *
find_admin ()
{
  aConfItem *aconf;

  for (aconf = conf; aconf; aconf = aconf->next)
    if (aconf->status & CONF_ADMIN)
      break;

  return (aconf);
}

/*
 * die/restart passes, like in df465 -mjs aconf->host =
 * X:<*PASSWORD*>:<password> aconf->passwd = X:<password>:<*PASSWORD*>
 */
char *
find_diepass ()
{
  aConfItem *aconf;

  for (aconf = conf; aconf; aconf = aconf->next)
    if (aconf->status & CONF_DRPASS)
      return (aconf->host);

  return NULL;
}

char *
find_restartpass ()
{
  aConfItem *aconf;

  for (aconf = conf; aconf; aconf = aconf->next)
    if (aconf->status & CONF_DRPASS)
      return (aconf->passwd);

  return NULL;
}


aConfItem *
find_me ()
{
  aConfItem *aconf;

  for (aconf = conf; aconf; aconf = aconf->next)
    if (aconf->status & CONF_ME)
      return (aconf);

  return ((aConfItem *) NULL);
}

/*
 * attach_confs Attach a CONF line to a client if the name passed
 * matches that for the conf file (for non-C/N lines) or is an exact
 * match (C/N lines only).  The difference in behaviour is to stop
 * C:*::* and N:*::*.
 */
aConfItem *
attach_confs (aClient * client_p, char *name, int statmask)
{
  aConfItem *tmp;
  aConfItem *first = NULL;
  int len = strlen (name);

  if (!name || len > HOSTLEN)
    return ((aConfItem *) NULL);

  for (tmp = conf; tmp; tmp = tmp->next)
  {
    if ((tmp->status & statmask) && !IsIllegal (tmp) &&
        ((tmp->status & (CONF_SERVER_MASK | CONF_HUB)) == 0) &&
        tmp->name && !match (tmp->name, name))
    {
      if (!attach_conf (client_p, tmp) && !first)
        first = tmp;
    }
    else if ((tmp->status & statmask) && !IsIllegal (tmp) &&
             (tmp->status & (CONF_SERVER_MASK | CONF_HUB)) &&
             (tmp->name && (irccmp (tmp->name, name) == 0)))
    {
      if (!attach_conf (client_p, tmp) && !first)
        first = tmp;
    }
  }
  return (first);
}

/*
 * Added for new access check    meLazy
 */
aConfItem *
attach_confs_host (aClient * client_p, char *host, int statmask)
{
  aConfItem *tmp;
  aConfItem *first = NULL;
  int len = strlen (host);

  if (!host || len > HOSTLEN)
    return ((aConfItem *) NULL);

  for (tmp = conf; tmp; tmp = tmp->next)
  {
    if ((tmp->status & statmask) && !IsIllegal (tmp) &&
        (tmp->status & CONF_SERVER_MASK) == 0 &&
        (!tmp->host || match (tmp->host, host) == 0))
    {
      if (!attach_conf (client_p, tmp) && !first)
        first = tmp;
    }
    else if ((tmp->status & statmask) && !IsIllegal (tmp) &&
             (tmp->status & CONF_SERVER_MASK) &&
             (tmp->host && (irccmp (tmp->host, host) == 0)))
    {
      if (!attach_conf (client_p, tmp) && !first)
        first = tmp;
    }
  }
  return (first);
}

/*
 * find a conf entry which matches the hostname and has the same name.
 */
aConfItem *
find_conf_exact (char *name, char *user, char *host, int statmask)
{
  aConfItem *tmp;
  char userhost[USERLEN + HOSTLEN + 3];

  (void) ircsprintf (userhost, "%s@%s", user, host);

  for (tmp = conf; tmp; tmp = tmp->next)
  {
    if (!(tmp->status & statmask) || !tmp->name || !tmp->host
        || (irccmp (tmp->name, name) != 0))
      continue;

    /* Accept if the *real* hostname (usually sockecthost) socket
     * host) matches *either* host or name field of the configuration.
     */

    if (match (tmp->host, userhost))
      continue;
    if (tmp->status & (CONF_OPERATOR | CONF_LOCOP))
    {
      if (tmp->clients < MaxLinks (Class (tmp)))
        return tmp;

      else
        continue;
    }
    else
      return tmp;
  }
  return ((aConfItem *) NULL);
}

/*
 * find_conf_name()
 * 
 * 
 * Accept if the *real* hostname (usually sockecthost) * matches *either*
 * host or name field of the configuration.
 */

aConfItem *
find_conf_name (char *name, int statmask)
{
  aConfItem *tmp;

  for (tmp = conf; tmp; tmp = tmp->next)
  {
    if ((tmp->status & statmask)
        && (!tmp->name || match (tmp->name, name) == 0))
      return tmp;
  }
  return ((aConfItem *) NULL);
}

aConfItem *
find_conf (Link * lp, char *name, int statmask)
{
  aConfItem *tmp;
  int namelen = name ? strlen (name) : 0;

  if (namelen > HOSTLEN)
    return ((aConfItem *) NULL);

  for (; lp; lp = lp->next)
  {
    tmp = lp->value.aconf;
    if ((tmp->status & statmask) &&
        (((tmp->status & (CONF_SERVER_MASK | CONF_HUB)) &&
          tmp->name && (irccmp (tmp->name, name) == 0)) ||
         ((tmp->status & (CONF_SERVER_MASK | CONF_HUB)) == 0 &&
          tmp->name && !match (tmp->name, name))))
      return tmp;
  }
  return ((aConfItem *) NULL);
}

/*
 * Added for new access check    meLazy
 */
aConfItem *
find_conf_host (Link * lp, char *host, int statmask)
{
  aConfItem *tmp;
  int hostlen = host ? strlen (host) : 0;

  if (hostlen > HOSTLEN || BadPtr (host))
    return ((aConfItem *) NULL);
  for (; lp; lp = lp->next)
  {
    tmp = lp->value.aconf;
    if (tmp->status & statmask &&
        (!(tmp->status & CONF_SERVER_MASK || tmp->host) ||
         (tmp->host && !match (tmp->host, host))))
      return tmp;
  }
  return ((aConfItem *) NULL);
}

/*
 * Added for CPU friendly U line checks - Raistlin
 */
aConfItem *
find_uline (Link * lp, char *host)
{
  aConfItem *tmp;
  int hostlen = host ? strlen (host) : 0;

  if (hostlen > HOSTLEN || BadPtr (host))
    return ((aConfItem *) NULL);

  for (; lp; lp = lp->next)
  {
    tmp = lp->value.aconf;
    if (tmp->status & CONF_ULINE
        && (tmp->host && (irccmp (tmp->host, host) == 0)))
    {
      return tmp;
    }
  }
  return ((aConfItem *) NULL);
}

aConfItem *
find_is_ulined (char *host)
{
  aConfItem *tmp;
  int hostlen = host ? strlen (host) : 0;

  if (hostlen > HOSTLEN || BadPtr (host))
    return ((aConfItem *) NULL);

  for (tmp = conf; tmp; tmp = tmp->next)
  {
    if (tmp->status & CONF_ULINE
        && (tmp->host && (irccmp (tmp->host, host) == 0)))
    {
      return tmp;
    }
  }
  return ((aConfItem *) NULL);
}

/*
 * find_conf_ip
 * 
 * Find a conf line using the IP# stored in it to search upon. Added
 * 1/8/92 by Avalon.
 */
aConfItem *
find_conf_ip (Link * lp, char *ip, char *user, int statmask)
{
  aConfItem *tmp;
  char *s;

  for (; lp; lp = lp->next)
  {
    tmp = lp->value.aconf;
    if (!(tmp->status & statmask))
      continue;
    s = strchr (tmp->host, '@');
    if (s == (char *) NULL)
      continue;
    *s = '\0';
    if (match (tmp->host, user))
    {
      *s = '@';
      continue;
    }
    *s = '@';
    if (!memcmp ((char *) &tmp->ipnum, ip, sizeof (struct IN_ADDR)))
      return tmp;
  }
  return ((aConfItem *) NULL);
}

/*
 * find_conf_entry
 * 
 * - looks for a match on all given fields.
 */

aConfItem *
find_conf_entry (aConfItem * aconf, int mask)
{
  aConfItem *bconf;

  for (bconf = conf, mask &= ~CONF_ILLEGAL; bconf; bconf = bconf->next)
  {
    if (!(bconf->status & mask) || (bconf->port != aconf->port))
      continue;

    if ((BadPtr (bconf->host) && !BadPtr (aconf->host)) ||
        (BadPtr (aconf->host) && !BadPtr (bconf->host)))
      continue;
    if (!BadPtr (bconf->host) && (irccmp (bconf->host, aconf->host) != 0))
      continue;

    if ((BadPtr (bconf->passwd) && !BadPtr (aconf->passwd)) ||
        (BadPtr (aconf->passwd) && !BadPtr (bconf->passwd)))
      continue;
    if (!BadPtr (bconf->passwd)
        && (irccmp (bconf->passwd, aconf->passwd) != 0))
      continue;

    if ((BadPtr (bconf->name) && !BadPtr (aconf->name)) ||
        (BadPtr (aconf->name) && !BadPtr (bconf->name)))
      continue;
    if (!BadPtr (bconf->name) && (irccmp (bconf->name, aconf->name) != 0))
      continue;
    break;
  }
  return bconf;
}

/*
 * rehash
 * 
 * Actual REHASH service routine. Called with sig == 0 if it has been
 * called as a result of an operator issuing this command, else assume
 * it has been called as a result of the server receiving a HUP signal.
 */
int
rehash (aClient * client_p, aClient * source_p, int sig)
{
  aConfItem **tmp = &conf, *tmp2;
  aClass *cltmp;
  aClient *target_p;
  int i, ret = 0, fd;
#ifndef _WIN32
  if (sig == SIGHUP)
  {
    sendto_ops
      ("Got signal SIGHUP, reloading ALL server configuration files.");
#ifdef	ULTRIX
    if (fork () > 0)
      exit (0);
    write_pidfile ();
#endif
    load_conf (DCONF, 1);
    remove_userbans_match_flags (UBAN_NETWORK, 0);
    remove_userbans_match_flags (UBAN_LOCAL | UBAN_TEMPORARY, 0);
  }
#endif

  if ((fd = openconf (configfile)) == -1)
  {
    sendto_ops ("Can't open %s file aborting rehash!", configfile);
    return -1;
  }

  /* Shadowfax's LOCKFILE code */
#ifdef LOCKFILE
  do_pending_klines ();
#endif

#ifndef USE_ADNS
  /* XXX this isnt needed anymore right ?  -fish */
  /* Too tired to go trough the code atm. Reenabling it for non ADNS for now - ShadowMaster */
  for (i = 0; i <= highest_fd; i++)
    if ((target_p = local[i]) && !IsMe (target_p))
    {
      /*
       * Nullify any references from client structures to this host
       * structure which is about to be freed. Could always keep
       * reference counts instead of this....-avalon
       */
      target_p->hostp = NULL;
    }
#endif
  while ((tmp2 = *tmp))
  {
    if (tmp2->clients || tmp2->status & CONF_LISTEN_PORT)
    {
      /* Configuration entry is still in use by some local
       * clients, cannot delete it--mark it so that it will be
       * deleted when the last client exits...
       */
      if (!(tmp2->status & (CONF_LISTEN_PORT | CONF_CLIENT)))
      {
        *tmp = tmp2->next;
        tmp2->next = NULL;
      }
      else
        tmp = &tmp2->next;
      tmp2->status |= CONF_ILLEGAL;
    }
    else if (tmp2->status & CONF_SQLINE)
    {
      tmp = &tmp2->next;
    }
    else
    {
      *tmp = tmp2->next;
      free_conf (tmp2);
    }
  }
  /*
   * We don't delete the class table, rather mark all entries for
   * deletion. The table is cleaned up by check_class. - avalon
   */

  for (cltmp = NextClass (FirstClass ()); cltmp; cltmp = NextClass (cltmp))
    MaxLinks (cltmp) = -1;

/* remove perm klines */
  remove_userbans_match_flags (UBAN_LOCAL, UBAN_TEMPORARY);

  clear_conf_list (&EList1);
  clear_conf_list (&EList2);
  clear_conf_list (&EList3);

  clear_conf_list (&FList1);
  clear_conf_list (&FList2);
  clear_conf_list (&FList3);

  (void) initconf (0, fd, source_p);


# ifdef KLINEFILE
  if ((fd = openconf (klinefile)) == -1)
    sendto_ops ("Can't open %s file klines could be missing!", klinefile);
  else
    (void) initconf (0, fd, source_p);
#endif

#ifdef VLINEFILE
  if ((fd = openconf (vlinefile)) == -1)
    sendto_ops ("Can't open %s file vlines could be missing!", vlinefile);
  else
    (void) initconf (0, fd, source_p);
#endif

  close_listeners ();

  /* flush out deleted I and P lines although still in use. */

  for (tmp = &conf; (tmp2 = *tmp);)
    if (!(tmp2->status & CONF_ILLEGAL))
      tmp = &tmp2->next;
    else
    {
      *tmp = tmp2->next;
      tmp2->next = NULL;
      if (!tmp2->clients)
        free_conf (tmp2);
    }
  rehashed = 1;

  for (i = 0; i <= highest_fd; i++)
  {

    /* our Y: lines could have changed, rendering our client ping
       cache invalid. Reset it here. - lucas */

    if ((target_p = local[i]) && !IsMe (target_p))
    {
      if (IsRegistered (target_p))
        target_p->pingval = get_client_ping (target_p);
      target_p->sendqlen = get_sendq (target_p);
    }
  }


  return ret;
}

/*
 * openconf
 * 
 * returns -1 on any error or else the fd opened from which to read the
 * configuration file from.  This may either be the file direct or one
 * end of a pipe from m4.
 */
int
openconf (char *filename)
{
  return open (filename, O_RDONLY);
}

extern char *getfield ();

static int server_info[] = {
  CAPAB_DOZIP, 'Z',
  CAPAB_DODKEY, 'E',
  0, 0
};

char nflagbuf[128];

char *
nflagstr (long nflag)
{
  int *i, flag;
  char m;
  char *p = nflagbuf;

  for (i = &server_info[6], m = *(i + 1); (flag = *i); i += 2, m = *(i + 1))
  {
    if (nflag & flag)
    {
      *p = m;
      p++;
    }
  }
  *p = '\0';
  return nflagbuf;
}

static int oper_access[] = {
  OFLAG_SERVICESOPER, 'a',
  OFLAG_KLINE, 'b',
  OFLAG_LROUTE, 'c',
  OFLAG_GNOTICE, 'g',
  OFLAG_HELPOP, 'h',
  OFLAG_GUESTADMIN, 'j',
  OFLAG_LKILL, 'k',
  OFLAG_LNOTICE, 'l',
  OFLAG_NETCOADMIN, 'n',
  OFLAG_LOCAL, 'o',
  OFLAG_PROTECTEDOPER, 'p',
  OFLAG_REHASH, 'r',
  OFLAG_TECHCOADMIN, 't',
  OFLAG_WALLOP, 'w',

  OFLAG_SERVADMIN, 'A',
  OFLAG_GROUTE, 'C',
  OFLAG_DIE, 'D',
  OFLAG_CANEDCONF, 'E',
  OFLAG_SERVCOADMIN, 'J',
  OFLAG_GKILL, 'K',
  OFLAG_NETADMIN, 'N',
  OFLAG_GLOBAL, 'O',
  OFLAG_SERVICESADMIN, 'P',
  OFLAG_RESTART, 'R',
  OFLAG_TECHADMIN, 'T',
  OFLAG_SERVICESROOT, 'Z',
  0, 0
};


char oflagbuf[128];

char *
oflagstr (long oflag)
{
  int *i, flag;
  char m;
  char *p = oflagbuf;

  for (i = &oper_access[6], m = *(i + 1); (flag = *i); i += 2, m = *(i + 1))
  {
    if (oflag & flag)
    {
      *p = m;
      p++;
    }
  }
  *p = '\0';
  return oflagbuf;
}

/*
 * * initconf()
 *    Read configuration file.
 * 
 * - file descriptor pointing to config file to use returns -1, 
 * if file cannot be opened, 0 if file opened
 */

#define MAXCONFLINKS 150

int
initconf (int opt, int fd, aClient * rehasher)
{
  static char quotes[9][2] = {
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'v', '\v'},
    {'\\', '\\'},
    {0, 0}
  };

  char *tmp, *s;
  int i, dontadd;
  char line[512], c[80];
  int ccount = 0, ncount = 0;
#ifndef INET6
  u_long vaddr;
#else
  char vaddr[sizeof (struct IN_ADDR)];
#endif

  aConfItem *aconf = NULL;

  (void) dgets (-1, NULL, 0);   /* make sure buffer is at empty pos  */

  while ((i = dgets (fd, line, sizeof (line) - 1)) > 0)
  {
    line[i] = '\0';
    if ((tmp = (char *) strchr (line, '\n')))
      *tmp = '\0';
    else
      while (dgets (fd, c, sizeof (c) - 1) > 0)
        if ((tmp = (char *) strchr (c, '\n')))
        {
          *tmp = '\0';
          break;
        }
    /* Do quoting of characters detection. */

    for (tmp = line; *tmp; tmp++)
    {
      if (*tmp == '\\')
      {
        for (i = 0; quotes[i][0]; i++)
          if (quotes[i][0] == *(tmp + 1))
          {
            *tmp = quotes[i][1];
            break;
          }
        if (!quotes[i][0])
          *tmp = *(tmp + 1);
        if (!*(tmp + 1))
          break;
        else
          for (s = tmp; (*s = *(s + 1)); s++);
      }
    }

    if (!*line || line[0] == '#' || line[0] == '\n' ||
        line[0] == ' ' || line[0] == '\t')
      continue;

    /* Could we test if it's conf line at all?        -Vesa */

    if (line[1] != ':')
    {
      Debug ((DEBUG_ERROR, "Bad config line: %s", line));
      continue;
    }

    if (aconf)
      free_conf (aconf);
    aconf = make_conf ();

    tmp = getfield (line);
    if (!tmp)
      continue;
    dontadd = 0;
    switch (*tmp)
    {
       case 'A':
       case 'a':               /* Administrative info */
         aconf->status = CONF_ADMIN;
         break;

       case 'C':               /* Server where I should try to connect */
       case 'c':
         ccount++;
         aconf->status = CONF_CONNECT_SERVER;
         break;

       case 'E':               /* kline exception lines */
       case 'e':
         aconf->status = CONF_ELINE;
         break;

       case 'F':               /* Super-Exempt hosts */
       case 'f':
         aconf->status = CONF_FLINE;
         break;

       case 'G':               /* restricted gcos */
       case 'g':
         aconf->status = CONF_GCOS;
         break;

       case 'H':               /* Hub server line */
       case 'h':
         aconf->status = CONF_HUB;
         break;

#ifdef LITTLE_I_LINES
       case 'i':               /* to connect me */
         aconf->status = CONF_CLIENT;
         aconf->flags |= CONF_FLAGS_LITTLE_I_LINE;
         break;

       case 'I':               /* Just plain normal irc client trying */
         aconf->status = CONF_CLIENT;
         break;
#else
       case 'i':               /* to connect me */
       case 'I':
         aconf->status = CONF_CLIENT;
         break;
#endif
       case 'J':               /* Network administrator lines */
       case 'j':
         aconf->status = CONF_NETWORKADMIN;
         break;

       case 'K':               /* the infamous klines */
       case 'k':
         aconf->status = CONF_KILL;
         break;

       case 'L':               /* guaranteed leaf server */
       case 'l':
         aconf->status = CONF_LEAF;
         break;

         /*
          * Me. Host field is name used for this host
          * and port number is the number of the port
          */
       case 'M':
       case 'm':
         aconf->status = CONF_ME;
         break;

       case 'N':
       case 'n':               /* Server where I should NOT try to
                                 * connect in case of lp failures
                                 * but which tries to connect ME
                                 */
         ++ncount;
         aconf->status = CONF_NOCONNECT_SERVER;
         break;


       case 'O':               /* Operator line */
       case 'o':
         aconf->status = CONF_OPERATOR;
         break;

       case 'P':               /* listen port line */
       case 'p':
         aconf->status = CONF_LISTEN_PORT;
         break;

       case 'Q':               /* restricted nicknames */
       case 'q':
         aconf->status = CONF_QUARANTINED_NICK;
         break;


       case 'U':               /* Ultimate Servers (aka God) */
       case 'u':
         aconf->status = CONF_ULINE;
         break;

       case 'V':               /* VHost Lines for /VHOST */
       case 'v':
         aconf->status = CONF_VHOST;
         break;

       case 'X':               /* die/restart pass line */
       case 'x':
         aconf->status = CONF_DRPASS;
         break;

       case 'Y':               /* Class line */
       case 'y':
         aconf->status = CONF_CLASS;
         break;

       default:
         Debug ((DEBUG_ERROR, "Error in config file: %s", line));
         break;
    }

    if (IsIllegal (aconf))
      continue;

    for (;;)                    /* Fake loop, that I can use break here --msa  */
    {
      if ((tmp = getfield (NULL)) == NULL)
        break;

      DupString (aconf->host, tmp);

      if ((tmp = getfield (NULL)) == NULL)
        break;

      DupString (aconf->passwd, tmp);

      if ((tmp = getfield (NULL)) == NULL)
        break;

      DupString (aconf->name, tmp);

      if ((tmp = getfield (NULL)) == NULL)
        break;

      if (aconf->status & CONF_OPS)
      {
        int *i2, flag;
        char *m = "*";
        for (m = (*tmp) ? tmp : m; *m; m++)
        {
          for (i2 = oper_access; (flag = *i2); i2 += 2)
            if (*m == (char) (*(i2 + 1)))
            {
              aconf->port |= flag;
              break;
            }
        }
        if (!(aconf->port & OFLAG_ISGLOBAL))
        {
          aconf->status = CONF_LOCOP;
        }
      }
      else if (aconf->status & CONF_NOCONNECT_SERVER)
      {
        int *i2, flag;
        char *m = "*";
        for (m = (*tmp) ? tmp : m; *m; m++)
        {
          for (i2 = server_info; (flag = *i2); i2 += 2)
            if (*m == (char) (*(i2 + 1)))
            {
              aconf->port |= flag;
              break;
            }
        }
      }
      else
      {
        aconf->port = atoi (tmp);
      }

      if ((tmp = getfield (NULL)) == NULL)
        break;

      Class (aconf) = find_class (atoi (tmp));

      if (aconf->status & CONF_CONNECT_SERVER)
      {
        if ((tmp = getfield (NULL)) == NULL)
          break;

        DupString (aconf->localhost, tmp);
      }
      break;

      /* NOTREACHED */
    }

    if (aconf->status & CONF_QUARANTINE)
    {
      if (*aconf->name && *aconf->name == '#')
        aconf->status = CONF_QUARANTINED_CHAN;
    }

    /*
     * * If conf line is a class definition, create a class entry *
     * for it and make the conf_line illegal and delete it.
     */
    if (aconf->status & CONF_CLASS)
    {
      add_class (atoi (aconf->host), atoi (aconf->passwd),
                 atoi (aconf->name), aconf->port, tmp ? atoi (tmp) : 0);
      continue;
    }
    /*
     * * associate each conf line with a class by using a pointer *
     * to the correct class record. -avalon
     */
    if (aconf->status & (CONF_CLIENT_MASK | CONF_LISTEN_PORT))
    {
      if (Class (aconf) == 0)
        Class (aconf) = find_class (0);
      if (MaxLinks (Class (aconf)) < 0)
        Class (aconf) = find_class (0);
    }

    if (aconf->status & (CONF_LISTEN_PORT | CONF_CLIENT))
    {
      aConfItem *bconf;

      if ((bconf = find_conf_entry (aconf, aconf->status)))
      {
        delist_conf (bconf);
        bconf->status &= ~CONF_ILLEGAL;
        if (aconf->status == CONF_CLIENT)
        {
          bconf->class->links -= bconf->clients;
          bconf->class = aconf->class;
          bconf->class->links += bconf->clients;
        }
        free_conf (aconf);
        aconf = bconf;
      }
      else if (aconf->host && aconf->status == CONF_LISTEN_PORT)
        (void) add_listener (aconf);
    }
    if (aconf->status & CONF_SERVER_MASK)
      if (ncount > MAXCONFLINKS || ccount > MAXCONFLINKS ||
          !aconf->host || strchr (aconf->host, '*') ||
          strchr (aconf->host, '?') || !aconf->name)
        continue;

    if (aconf->status & (CONF_SERVER_MASK | CONF_LOCOP | CONF_OPERATOR))
      if (!strchr (aconf->host, '@') && *aconf->host != '/')
      {
        char *newhost;
        int len = 3;
        /* *@\0 = 3 */
        len += strlen (aconf->host);
        newhost = (char *) MyMalloc (len);
        (void) ircsprintf (newhost, "*@%s", aconf->host);
        MyFree (aconf->host);
        aconf->host = newhost;
      }
    if (aconf->status & CONF_SERVER_MASK)
    {
      if (BadPtr (aconf->passwd))
        continue;
      else if (!(opt & BOOT_QUICK))
        (void) lookup_confhost (aconf);
    }

    /*
     * * Own port and name cannot be changed after the startup.  (or
     * could be allowed, but only if all links are closed  first). 
     * Configuration info does not override the name and port  if
     * previously defined. Note, that "info"-field can be changed
     * by "/rehash". Can't change vhost mode/address either
     */

    if (aconf->status == CONF_ME)
    {
      strncpyzt (me.info, aconf->name, sizeof (me.info));

      if (me.name[0] == '\0' && aconf->host[0])
      {
        strncpyzt (me.name, aconf->host, sizeof (me.name));
        if ((aconf->passwd[0] != '\0') && (aconf->passwd[0] != '*'))
        {
          memset ((char *) &vserv, '\0', sizeof (vserv));

          vserv.SIN_FAMILY = AFINET;
#ifndef INET6
          inet_pton (AFINET, aconf->passwd, &vaddr);
#else
          inet_pton (AFINET, aconf->passwd, vaddr);
#endif
          memcpy ((char *) &vserv.SIN_ADDR, (char *) &vaddr,
                  sizeof (struct IN_ADDR));
          specific_virtual_host = 1;
        }
      }

      if (portnum < 0 && aconf->port >= 0)
        portnum = aconf->port;

    }

    if ((aconf->status & CONF_KILL) && aconf->host)
    {
      struct userBan *ban;
      char *ub_u, *ub_r;
      int ii;
      char fbuf[512];
      aClient *ub_target_p;

      if (BadPtr (aconf->host))
        continue;

      ub_u = BadPtr (aconf->name) ? "*" : aconf->name;
      ub_r = BadPtr (aconf->passwd) ? "<No Reason>" : aconf->passwd;

      ban = make_hostbased_ban (ub_u, aconf->host);
      if (!ban)
        continue;

      ban->flags |= UBAN_LOCAL;
      ban->reason = (char *) MyMalloc (strlen (ub_r) + 1);
      strcpy (ban->reason, ub_r);
      ban->timeset = NOW;

      add_hostbased_userban (ban);

      /* Check local users against it */
      for (ii = highest_fd; ii >= 0; ii--)
      {
        if (!(ub_target_p = local[i]) || IsMe (ub_target_p)
            || IsLog (ub_target_p) || ub_target_p == rehasher)
          continue;

        if (IsPerson (ub_target_p) && user_match_ban (ub_target_p, ban)
            && !IsExempt (ub_target_p))
        {
          sendto_ops ("Local-ban active for %s",
                      get_client_name (ub_target_p, FALSE));
          ircsprintf (fbuf, "Local-banned: %s", ub_r);
          exit_client (ub_target_p, ub_target_p, &me, fbuf);
        }
      }

      continue;
    }

    if (aconf->host && (aconf->status & CONF_ELINE))
    {
      char *host = host_field (aconf);

      dontadd = 1;
      switch (sortable (host))
      {
         case 0:
           l_addto_conf_list (&EList3, aconf, host_field);
           break;
         case 1:
           addto_conf_list (&EList1, aconf, host_field);
           break;
         case -1:
           addto_conf_list (&EList2, aconf, rev_host_field);
           break;
      }
      MyFree (host);
    }

    if (aconf->host && (aconf->status & CONF_FLINE))
    {
      char *host = host_field (aconf);

      dontadd = 1;
      switch (sortable (host))
      {
         case 0:
           l_addto_conf_list (&FList3, aconf, host_field);
           break;
         case 1:
           addto_conf_list (&FList1, aconf, host_field);
           break;
         case -1:
           addto_conf_list (&FList2, aconf, rev_host_field);
           break;
      }
      MyFree (host);
    }

    (void) collapse (aconf->host);
    (void) collapse (aconf->name);
    Debug ((DEBUG_NOTICE,
            "Read Init: (%d) (%s) (%s) (%s) (%d) (%d)",
            aconf->status, aconf->host, aconf->passwd,
            aconf->name, aconf->port, Class (aconf)));
    if (!dontadd)
    {
      aconf->next = conf;
      conf = aconf;
    }
    aconf = NULL;
  }
  if (aconf)
    free_conf (aconf);
  (void) dgets (-1, NULL, 0);   /* make sure buffer is at empty pos */
  (void) close (fd);
  check_class ();
  nextping = nextconnect = time (NULL);
  return 0;
}

/*
 * lookup_confhost Do (start) DNS lookups of all hostnames in the conf
 * line and convert an IP addresses in a.b.c.d number for to IP#s.
 * 
 * cleaned up Aug 3'97 - Dianora
 */
static int
lookup_confhost (aConfItem * aconf)
{
  char *s;
  struct hostent *hp;
  Link ln;

  if (BadPtr (aconf->host) || BadPtr (aconf->name))
  {

#ifndef INET6
    if (aconf->ipnum.S_ADDR == -1)
#else
    if (!memcmp (aconf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR)))
#endif
      memset ((char *) &aconf->ipnum, '\0', sizeof (struct IN_ADDR));


    Debug ((DEBUG_ERROR, "Host/server name error: (%s) (%s)",
            aconf->host, aconf->name));
    return -1;
  }
  if ((s = strchr (aconf->host, '@')))
    s++;
  else
    s = aconf->host;
  /*
   * * Do name lookup now on hostnames given and store the ip
   * numbers in conf structure.
   */
  if (!MyIsAlpha (*s) && !MyIsDigit (*s))
  {
#ifndef INET6
    if (aconf->ipnum.S_ADDR == -1)
#else
    if (!memcmp (aconf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR)))
#endif
      memset ((char *) &aconf->ipnum, '\0', sizeof (struct IN_ADDR));

    Debug ((DEBUG_ERROR, "Host/server name error: (%s) (%s)",
            aconf->host, aconf->name));
    return -1;
  }
  /*
   * * Prepare structure in case we have to wait for a reply which
   * we get later and store away.
   */
#ifndef USE_ADNS
  ln.value.aconf = aconf;
  ln.flags = ASYNC_CONF;
#endif

#ifndef INET6
  if (MyIsDigit (*s))
    inet_pton (AFINET, s, &aconf->ipnum.S_ADDR);
#else
  if (strchr (s, ':'))
    inet_pton (AFINET, s, aconf->ipnum.S_ADDR);
#endif
#ifdef USE_ADNS
  Debug ((DEBUG_DNS, "Doing DNS I think"));
#else
  else if ((hp = gethost_byname (s, &ln)))
#endif

#ifndef INET6
    if (aconf->ipnum.S_ADDR == -1)
#else
    if (!memcmp (aconf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR)))
#endif
      memset ((char *) &aconf->ipnum, '\0', sizeof (struct IN_ADDR));
  {
#ifdef USE_ADNS
    aconf->dnslookup = malloc (sizeof (struct DNSQuery));
    aconf->dnslookup->ptr = aconf;
    aconf->dnslookup->callback = got_async_conf_dns;
    /* XXX need to change AF_INET to represent either ipv4 or ipv6 */
    adns_gethost (s, AF_INET, aconf->dnslookup);
    Debug ((DEBUG_DEBUG,
            "Calling adns_gethost for Conf for %s", aconf->name));
    return 1;
#else
    Debug ((DEBUG_ERROR, "Host/server name error: (%s) (%s)",
            aconf->host, aconf->name));
    return -1;
#endif
  }

  /* NOTREACHED */
  return 0;
}

int
find_eline (aClient * client_p)
{
  return find_conf_match (client_p, &EList1, &EList2, &EList3);
}

int
find_fline (aClient * client_p)
{
  return find_conf_match (client_p, &FList1, &FList2, &FList3);
}

/*
* find_conf_match is used by find_fline and find_eline
*/

int
find_conf_match (aClient * client_p, aConfList * List1, aConfList * List2,
                 aConfList * List3)
{
  char *host, *name, *ip;
  aConfItem *tmp;
  char rev[HOSTLEN + 1];
  aConfList *list;

  if (!client_p->user)
    return 0;

  host = client_p->sockhost;
  ip = client_p->hostip;
  name = client_p->user->username;

  if (strlen (host) > (size_t) HOSTLEN
      || (name ? strlen (name) : 0) > (size_t) HOSTLEN)
    return (0);

  reverse (rev, host);


  /* Try IP's of the form "word*" */

  list = List1;
  while ((tmp = find_matching_conf (list, ip)) != NULL)
  {
    if (tmp->name && (!name || !match (tmp->name, name)))
      return (tmp ? -1 : 0);
    list = NULL;
  }

  /* Start with hostnames of the form "*word" (most frequent) -Sol */

  list = List2;
  while ((tmp = find_matching_conf (list, rev)) != NULL)
  {
    if (tmp->name && (!name || !match (tmp->name, name)))
      return (tmp ? -1 : 0);
    list = NULL;
  }

  /* Try hostnames of the form "word*" -Sol */

  list = List1;
  while ((tmp = find_matching_conf (list, host)) != NULL)
  {
    if (tmp->name && (!name || !match (tmp->name, name)))
      return (tmp ? -1 : 0);
    list = NULL;
  }

  /* If none of the above worked, try non-sorted entries -Sol */
  list = List3;
  while ((tmp = l_find_matching_conf (list, host)) != NULL)
  {
    if (tmp->host && tmp->name && (!name || !match (tmp->name, name)))
      return (tmp ? -1 : 0);
    list = NULL;
  }

  return (tmp ? -1 : 0);
}

/* m_svsnoop - Remove all ops from a server
 *  Once again pretty much straight outt df
 *    - Raistlin
 * parv[0] = sender prefix
 * parv[1] = server
 * parv[2] = +/-
 */

int
m_svsnoop (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aConfItem *aconf;

  if (!(IsULine (source_p) && parc > 2))
    return 0;

  if (hunt_server (client_p, source_p, ":%s SVSNOOP %s :%s", 1, parc, parv) ==
      HUNTED_ISME)
  {
    if (parv[2][0] == '+')
      for (aconf = conf; aconf; aconf = aconf->next)
      {
        if (aconf->status & CONF_OPERATOR || aconf->status & CONF_LOCOP)
          aconf->status = CONF_ILLEGAL;
      }
    else
      rehash (&me, &me, 2);
  }
  return 0;
}

/* find_exception
** find a virtual exception
*/
int
find_exception (char *abba)
{
  aConfItem *tmp;

  for (tmp = conf; tmp; tmp = tmp->next)
  {
  }

  return 0;
}
