/*
 *UltimateIRCd - an Internet Relay Chat Daemon, src/res_win32.c
 *
 *Copyright (C) 2002-2007 by the past and present ircd coders, and others.
 *Refer to the documentation within doc/authors/ for full credits and copyrights.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation; either version 2 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program; if not, write to the Free Software
 *Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307
 *USA
 *
 *$Id: res_win32.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

/*
 * src/res.c (C)opyright 1992 Darren Reed. All rights reserved. This
 * file may not be distributed without the author's permission in any
 * shape or form. The author takes no responsibility for any damage or
 * loss of property which results from the use of this software.
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "res.h"
#include "numeric.h"
#include "h.h"

#include <signal.h>
#include "nameser.h"
#include "resolv.h"

#ifndef _WIN32
# error "res_win32.c should not be compiled on non-windows systems!"
#endif

#undef	DEBUG                   /* because there is a lot of debug code in here :-) */

extern int dn_expand (char *, char *, char *, char *, int);
extern int dn_skipname (char *, char *);
extern int res_mkquery (int, char *, int, int, char *, int,
                        struct rrec *, char *, int);
extern int highest_fd;
extern aClient *local[];

static char hostbuf[512];
static char dot[] = ".";
static int incache = 0;
static CacheTable hashtable[ARES_CACSIZE];
static aCache *cachetop = NULL;
static ResRQ *last, *first;

static void rem_cache (aCache *);
static void rem_request (ResRQ *);
static int do_query_name (Link *, char *, ResRQ *);
static int do_query_number (Link *, struct in_addr *, ResRQ *);
static void resend_query (ResRQ *);
static int proc_answer (ResRQ *, HEADER *, char *, char *);
static int query_name (char *, int, int, ResRQ *);
static aCache *make_cache (ResRQ *);
static aCache *find_cache_name (char *);
static aCache *find_cache_number (ResRQ *, char *);
static int add_request (ResRQ *);
static ResRQ *make_request (Link *);
static int send_res_msg (char *, int, int);
static ResRQ *find_id (int);
static int hash_number (unsigned char *);
static void update_list (ResRQ *, aCache *);
static int hash_name (char *);
static void async_dns (void *parm);

static struct cacheinfo
{
  int ca_adds;
  int ca_dels;
  int ca_expires;
  int ca_lookups;
  int ca_na_hits;
  int ca_nu_hits;
  int ca_updates;
}
cainfo;

static struct resinfo
{
  int re_errors;
  int re_nu_look;
  int re_na_look;
  int re_replies;
  int re_requests;
  int re_resends;
  int re_sent;
  int re_timeouts;
  int re_shortttl;
  int re_unkrep;
}
reinfo;

char *
inetntoa (in)
     char *in;
{
  static char buf[16];
  u_char *s = (u_char *) in;
  int a, b, c, d;
  a = (int) *s++;
  b = (int) *s++;
  c = (int) *s++;
  d = (int) *s++;
  (void) ircsprintf (buf, "%d.%d.%d.%d", a, b, c, d);
  return buf;
}


int
init_resolver (op)
     int op;
{
  int ret = 0;
#ifdef	LRAND48
  srand48 (timeofday);
#endif
  if (op & RES_INITLIST)
  {
    bzero ((char *) &reinfo, sizeof (reinfo));
    first = last = NULL;
  }
  if (op & RES_CALLINIT)
  {
#ifdef GLIBC2_2
    ret = __res_init ();
#else
    ret = res_init ();
#endif
    if (!_res.nscount)
    {
      _res.nscount = 1;
      _res.nsaddr_list[0].sin_addr.s_addr = inet_addr ("127.0.0.1");
    }
  }

#ifdef DEBUG
  if (op & RES_INITDEBG);
  _res.options |= RES_DEBUG;
#endif
  if (op & RES_INITCACH)
  {
    bzero ((char *) &cainfo, sizeof (cainfo));
    bzero ((char *) hashtable, sizeof (hashtable));
  }

  if (op == 0)
    ret = resfd;
  return ret;
}

static int
add_request (new)
     ResRQ *new;
{
  if (!new)
    return -1;

  if (!first)
    first = last = new;
  else
  {
    last->next = new;
    last = new;
  }

  new->next = NULL;
  reinfo.re_requests++;
  return 0;
}

/*
 * remove a request from the list. This must also free any memory that has
 * been allocated for temporary storage of DNS results.
 */
static void
rem_request (old)
     ResRQ *old;
{
  ResRQ **rptr, *r2ptr = NULL;
  int i;
  char *s;

  if (!old)
    return;

  while (old->locked)
    Sleep (0);

  for (rptr = &first; *rptr; r2ptr = *rptr, rptr = &(*rptr)->next)
    if (*rptr == old)
    {
      *rptr = old->next;
      if (last == old)
        last = r2ptr;
      break;
    }

#ifdef	DEBUG
  Debug ((DEBUG_INFO, "rem_request:Remove %#x at %#x %#x",
          old, *rptr, r2ptr));
#endif
  r2ptr = old;
  if (r2ptr->he)
    MyFree (r2ptr->he);

  if (r2ptr->name)
    MyFree (r2ptr->name);

  MyFree (r2ptr);
  return;
}

/*
 * Create a DNS request record for the server.
 */
static ResRQ *
make_request (lp)
     Link *lp;
{
  ResRQ *nreq;
  nreq = (ResRQ *) MyMalloc (sizeof (ResRQ));
  bzero ((char *) nreq, sizeof (ResRQ));
  nreq->next = NULL;            /* where NULL is non-zero ;) */
  nreq->sentat = timeofday;
  nreq->retries = 2;
  nreq->resend = 1;
  nreq->srch = -1;

  if (lp)
    bcopy ((char *) lp, (char *) &nreq->cinfo, sizeof (Link));
  else
    bzero ((char *) &nreq->cinfo, sizeof (Link));

  nreq->timeout = 2;            /* start at 2 and exponential inc. */
  nreq->he = (struct hostent *) MyMalloc (MAXGETHOSTSTRUCT);
  bzero ((char *) nreq->he, MAXGETHOSTSTRUCT);
  nreq->he->h_addrtype = AF_INET;
  nreq->he->h_name = NULL;
  (void) add_request (nreq);
  return nreq;
}

/*
 * Remove queries from the list which have been there too long without
 * being resolved.
 */
time_t
timeout_query_list (now)
     time_t now;
{
  ResRQ *rptr, *r2ptr;
  time_t next = 0, tout;
  aClient *cptr;
  Debug ((DEBUG_DNS, "timeout_query_list at %s", myctime (now)));

  for (rptr = first; rptr; rptr = r2ptr)
  {
    r2ptr = rptr->next;
    tout = rptr->sentat + rptr->timeout;
    if (now >= tout && !rptr->locked)
    {
      if (--rptr->retries <= 0)
      {
#ifdef DEBUG
        Debug ((DEBUG_ERROR, "timeout %x now %d cptr %x",
                rptr, now, rptr->cinfo.value.cptr));
#endif
        reinfo.re_timeouts++;
        cptr = rptr->cinfo.value.cp;
        switch (rptr->cinfo.flags)
        {
           case ASYNC_CLIENT:
#ifdef SHOW_HEADERS
#ifdef USE_SSL
             if (!IsSSL (cptr))
             {
#endif
               Debug ((DEBUG_DNS, "Sending Header"));
               sendto_one (cptr, REPORT_FIN_DNS);
#ifdef USE_SSL
             }
#endif
#endif
             ClearDNS (cptr);
             check_client_fd (cptr);
             break;

           case ASYNC_SERVER:
             sendto_ops ("Host %s unknown", rptr->name);
             ClearDNS (cptr);
             if (check_server (cptr, NULL, NULL, NULL, 1))
               (void) exit_client (cptr, cptr, &me, "No Permission");

             break;

           case ASYNC_CONNECT:
             sendto_ops ("Host %s unknown", rptr->name);
             break;
        }
        rem_request (rptr);
        continue;
      }
      else
      {
        rptr->sentat = now;
        rptr->timeout += rptr->timeout;
        tout = now + rptr->timeout;

#ifdef DEBUG
        Debug ((DEBUG_INFO, "r %x now %d retry %d c %x",
                rptr, now, rptr->retries, rptr->cinfo.value.cptr));
#endif /* 
        */
      }
    }

    if (!next || tout < next)
      next = tout;
  }

  Debug ((DEBUG_DNS, "Next timeout_query_list() at %s, %d",
          myctime ((next > now) ? next : (now + AR_TTL)),
          (next > now) ? (next - now) : AR_TTL));
  return (next > now) ? next : (now + AR_TTL);
}



/*
 * del_queries - called by the server to cleanup outstanding queries for
 * which there no longer exist clients or conf lines.
 */
void
del_queries (cp)
     char *cp;
{
  ResRQ *rptr, *r2ptr;

  for (rptr = first; rptr; rptr = r2ptr)
  {
    r2ptr = rptr->next;
    if (cp == rptr->cinfo.value.cp)
      rem_request (rptr);
  }
}

/*
 * find a dns request id (id is determined by dn_mkquery)
 */
static ResRQ *
find_id (id)
     int id;
{
  ResRQ *rptr;

  for (rptr = first; rptr; rptr = rptr->next)
    if (rptr->id == id)
      return rptr;
  return NULL;
}

struct hostent *
gethost_byname (name, lp)
     char *name;
     Link *lp;
{
  aCache *cp;
  reinfo.re_na_look++;

  if ((cp = find_cache_name (name)))
    return (struct hostent *) cp->he;

  if (!lp)
    return NULL;

  (void) do_query_name (lp, name, NULL);
  return NULL;
}


struct hostent *
gethost_byaddr (addr, lp)
     char *addr;
     Link *lp;
{
  aCache *cp;
  reinfo.re_nu_look++;

  if ((cp = find_cache_number (NULL, addr)))
    return (struct hostent *) cp->he;

  if (!lp)
    return NULL;

  (void) do_query_number (lp, (struct in_addr *) addr, NULL);
  return NULL;
}

static int
do_query_name (lp, name, rptr)
     Link *lp;
     char *name;
     ResRQ *rptr;
{
/*
 * Store the name passed as the one to lookup and generate other host
 * names to pass onto the nameserver(s) for lookups.
 */
  if (!rptr)
  {
    rptr = make_request (lp);
    rptr->type = T_A;
    rptr->name = (char *) MyMalloc (strlen (name) + 1);
    (void) strcpy (rptr->name, name);
  }

  rptr->id = _beginthread (async_dns, 0, (void *) rptr);
  rptr->sends++;
  return 0;
}



/*
 * Use this to do reverse IP# lookups.
 */
static int
do_query_number (lp, numb, rptr)
     Link *lp;
     struct in_addr *numb;
     ResRQ *rptr;
{
  if (!rptr)
  {
    rptr = make_request (lp);
    rptr->type = T_PTR;
    rptr->addr.s_addr = numb->s_addr;
    rptr->he->h_length = sizeof (struct in_addr);
  }

  rptr->id = _beginthread (async_dns, 0, (void *) rptr);
  rptr->sends++;
  return 0;
}

/*
 * read a dns reply from the nameserver and process it.
 */
struct hostent *
get_res (lp, id)
     char *lp;
     long id;
{
  struct hostent *he;
  ResRQ *rptr = NULL;
  aCache *cp;
  reinfo.re_replies++;

/*
 * response for an id which we have already received an answer for
 * just ignore this response.
 */
  rptr = find_id (id);
  if (!rptr)
  {
    if (lp && rptr)
      bcopy ((char *) &rptr->cinfo, lp, sizeof (Link));
    return (struct hostent *) NULL;
  }

  he = rptr->he;

  if (he && he->h_name && ((struct in_addr *) he->h_addr)->s_addr
      && rptr->locked < 2)
  {
/*
 * We only need to re-check the DNS if its a "byaddr" call,
 * the "byname" calls will work correctly. -Cabal95
 */
    char tempname[120];
    int i;
    long amt;
    struct hostent *hp, *he = rptr->he;

    strcpy (tempname, he->h_name);
    hp = gethostbyname (tempname);

    if (!hp && bcmp (hp->h_addr, he->h_addr, sizeof (struct in_addr)))
      rptr->he->h_name = NULL;
  }

  if (lp)
    bcopy ((char *) &rptr->cinfo, lp, sizeof (Link));

  cp = make_cache (rptr);
# ifdef DEBUG
  Debug ((DEBUG_INFO, "get_res:cp=%#x rptr=%#x (made)", cp, rptr));
# endif
  rptr->locked = 0;
  rem_request (rptr);
  return cp ? (struct hostent *) cp->he : NULL;
}

static int
hash_number (ip)
     unsigned char *ip;
{
  u_int hashv = 0;
/* could use loop but slower */
  hashv += (int) *ip++;
  hashv += hashv + (int) *ip++;
  hashv += hashv + (int) *ip++;
  hashv += hashv + (int) *ip++;
  hashv %= ARES_CACSIZE;
  return (hashv);
}


static int
hash_name (name)
     register char *name;
{
  u_int hashv = 0;

  if (name == NULL)
  {
    sendto_realops
      ("Caught NULL pointer in hash_name().(Bad thing -- tell rwg.)");
    return (0);
  }

  for (; *name && *name != '.'; name++)
    hashv += *name;

  hashv %= ARES_CACSIZE;
  return (hashv);
}



/*
** Add a new cache item to the queue and hash table.
*/
static aCache *
add_to_cache (ocp)
     aCache *ocp;
{
  aCache *cp = NULL;
  int hashv;

#ifdef DEBUG
  Debug ((DEBUG_INFO,
          "add_to_cache:ocp %#x he %#x name %#x addrl %#x 0 %#x",
          ocp,
          &ocp->he, ocp->he.h_name, ocp->he.h_addr_list,
          ocp->he.h_addr_list[0]));
#endif
  ocp->list_next = cachetop;
  cachetop = ocp;
  hashv = hash_name (ocp->he->h_name);
  ocp->hname_next = hashtable[hashv].name_list;
  hashtable[hashv].name_list = ocp;
  hashv = hash_number ((u_char *) ocp->he->h_addr);
  ocp->hnum_next = hashtable[hashv].num_list;
  hashtable[hashv].num_list = ocp;

#ifdef	DEBUG
  Debug ((DEBUG_INFO, "add_to_cache:added %s[%08x] cache %#x.",
          ocp->he->h_name, ocp->he->h_addr_list[0], ocp));

  Debug ((DEBUG_INFO,
          "add_to_cache:h1 %d h2 %x lnext %#x namnext %#x numnext %#x",
          hash_name (ocp->he->h_name), hashv, ocp->list_next,
          ocp->hname_next, ocp->hnum_next));
#endif
/*
 * LRU deletion of excessive cache entries.
 */
  if (++incache > MAXCACHED)
  {
    for (cp = cachetop; cp->list_next; cp = cp->list_next)
      ;
    rem_cache (cp);
  }

  cainfo.ca_adds++;
  return ocp;
}

/*
** update_list does not alter the cache structure passed. It is assumed that
** it already contains the correct expire time, if it is a new entry. Old
** entries have the expirey time updated.
*/
static void
update_list (rptr, cachep)
     ResRQ *rptr;
     aCache *cachep;
{
  aCache **cpp, *cp = cachep;
  char *s, *t, **base;
  int i, j;
  int addrcount;
/*
 ** search for the new cache item in the cache list by hostname.
 ** If found, move the entry to the top of the list and return.
 */
  cainfo.ca_updates++;

  for (cpp = &cachetop; *cpp; cpp = &((*cpp)->list_next))
    if (cp == *cpp)
      break;

  if (!*cpp)
    return;

  *cpp = cp->list_next;
  cp->list_next = cachetop;
  cachetop = cp;
  return;
}


static aCache *
find_cache_name (name)
     char *name;
{
  aCache *cp;
  char *s;
  int hashv, i;

  hashv = hash_name (name);
  cp = hashtable[hashv].name_list;

#ifdef	DEBUG
  Debug ((DEBUG_DNS, "find_cache_name:find %s : hashv = %d", name, hashv));
#endif
  for (; cp; cp = cp->hname_next)
    for (i = 0, s = cp->he->h_name; s; s = cp->he->h_aliases[i++])
      if (irccmp (s, name) == 0)
      {
        cainfo.ca_na_hits++;
        update_list (NULL, cp);
        return cp;
      }

  for (cp = cachetop; cp; cp = cp->list_next)
  {
/*
 * if no aliases or the hash value matches, we've already
 * done this entry and all possiblilities concerning it.
 */
    if (!cp->he->h_aliases)
      continue;

    if (hashv == hash_name (cp->he->h_name))
      continue;

    for (i = 0, s = cp->he->h_aliases[i]; s && i < MAXALIASES; i++)
      if (irccmp (name, s) == 0)
      {
        cainfo.ca_na_hits++;
        update_list (NULL, cp);
        return cp;
      }
  }
  return NULL;
}

/*
 * find a cache entry by ip# and update its expire time
 */
static aCache *
find_cache_number (rptr, numb)
     ResRQ *rptr;
     char *numb;
{
  aCache *cp;
  int hashv, i;
#ifdef	DEBUG
  struct in_addr *ip = (struct in_addr *) numb;
#endif

  hashv = hash_number ((u_char *) numb);
  cp = hashtable[hashv].num_list;

#ifdef DEBUG
  Debug ((DEBUG_DNS, "find_cache_number:find %s[%08x]: hashv = %d",
          inetntoa (numb), ntohl (ip->s_addr), hashv));
#endif

  for (; cp; cp = cp->hnum_next)
    for (i = 0; cp->he->h_addr_list && cp->he->h_addr_list[i]; i++)
      if (!bcmp (cp->he->h_addr_list[i], numb, sizeof (struct in_addr)))
      {
        cainfo.ca_nu_hits++;
        update_list (rptr, cp);
        return cp;
      }

  for (cp = cachetop; cp; cp = cp->list_next)
  {
/*
 * single address entry...would have been done by hashed
 * search above...
 */
    if (!cp->he->h_addr_list[1])
      continue;

/*
 * if the first IP# has the same hashnumber as the IP# we
 * are looking for, its been done already.
 */
    if (hashv == hash_number ((u_char *) cp->he->h_addr_list[0]))
      continue;

    for (i = 1; cp->he->h_addr_list && cp->he->h_addr_list[i]; i++)
      if (!bcmp (cp->he->h_addr_list[i], numb, sizeof (struct in_addr)))
      {
        cainfo.ca_nu_hits++;
        update_list (rptr, cp);
        return cp;
      }
  }
  return NULL;
}


static aCache *
make_cache (rptr)
     ResRQ *rptr;
{
  aCache *cp;
  int i, n;
  struct hostent *hp;
  char *s, **t;

/*
 ** shouldn't happen but it just might...
 */
  if (!rptr->he->h_name || !((struct in_addr *) rptr->he->h_addr)->s_addr)
    return NULL;

/*
 ** Make cache entry.First check to see if the cache already exists
 ** and if so, return a pointer to it.
 */
  if ((cp = find_cache_number (rptr,
                               (char *) &((struct in_addr *) rptr->he->
                                          h_addr)->s_addr)))
    return cp;

  for (i = 1; rptr->he->h_addr_list[i] &&
       ((struct in_addr *) rptr->he->h_addr_list[i])->s_addr; i++)

    if ((cp = find_cache_number (rptr,
                                 (char *) &((struct in_addr *) rptr->he->
                                            h_addr_list[i])->s_addr)))
      return cp;

/*
 ** a matching entry wasnt found in the cache so go and make one up.
 */
  cp = (aCache *) MyMalloc (sizeof (aCache));
  bzero ((char *) cp, sizeof (aCache));
  cp->he = (struct hostent *) MyMalloc (MAXGETHOSTSTRUCT);
  res_copyhostent (rptr->he, cp->he);

  if (rptr->ttl < 600)
  {
    reinfo.re_shortttl++;
    cp->ttl = 600;
  }
  else
    cp->ttl = rptr->ttl;

  cp->expireat = timeofday + cp->ttl;
  rptr->he->h_name = NULL;

#ifdef DEBUG
  Debug ((DEBUG_INFO, "make_cache:made cache %#x", cp));
#endif
  return add_to_cache (cp);
}

/*
 * rem_cache
 * delete a cache entry from the cache structures and lists and return
 * all memory used for the cache back to the memory pool.
 */
static void
rem_cache (ocp)
     aCache *ocp;
{
  aCache **cp;
  struct hostent *hp = ocp->he;
  int hashv;
  aClient *cptr;

#ifdef	DEBUG
  Debug ((DEBUG_DNS, "rem_cache: ocp %#x hp %#x l_n %#x aliases %#x",
          ocp, hp, ocp->list_next, hp->h_aliases));
#endif
/*
 ** Cleanup any references to this structure by destroying the
 ** pointer.
 */
  for (hashv = highest_fd; hashv >= 0; hashv--)
    if ((cptr = local[hashv]) && (cptr->hostp == hp))
      cptr->hostp = NULL;
/*
 * remove cache entry from linked list
 */
  for (cp = &cachetop; *cp; cp = &((*cp)->list_next))
    if (*cp == ocp)
    {
      *cp = ocp->list_next;
      break;
    }
/*
 * remove cache entry from hashed name lists
 */
  hashv = hash_name (hp->h_name);

#ifdef	DEBUG
  Debug ((DEBUG_DEBUG, "rem_cache: h_name %s hashv %d next %#x first %#x",
          hp->h_name, hashv, ocp->hname_next, hashtable[hashv].name_list));
#endif
  for (cp = &hashtable[hashv].name_list; *cp; cp = &((*cp)->hname_next))
    if (*cp == ocp)
    {
      *cp = ocp->hname_next;
      break;
    }
/*
 * remove cache entry from hashed number list
 */
  hashv = hash_number ((u_char *) hp->h_addr);

#ifdef	DEBUG
  Debug ((DEBUG_DEBUG, "rem_cache: h_addr %s hashv %d next %#x first %#x",
          inetntoa (hp->h_addr), hashv, ocp->hnum_next,
          hashtable[hashv].num_list));

#endif
  for (cp = &hashtable[hashv].num_list; *cp; cp = &((*cp)->hnum_next))
    if (*cp == ocp)
    {
      *cp = ocp->hnum_next;
      break;
    }

  MyFree ((char *) hp);
  MyFree ((char *) ocp);
  incache--;
  cainfo.ca_dels++;
  return;
}

/*
 * removes entries from the cache which are older than their expirey times.
 * returns the time at which the server should next poll the cache.
 */
time_t
expire_cache (now)
     time_t now;
{
  aCache *cp, *cp2;
  time_t next = 0;

  for (cp = cachetop; cp; cp = cp2)
  {
    cp2 = cp->list_next;
    if (now >= cp->expireat)
    {
      cainfo.ca_expires++;
      rem_cache (cp);
    }
    else if (!next || next > cp->expireat)
      next = cp->expireat;
  }
  return (next > now) ? next : (now + AR_TTL);
}



/*
 * remove all dns cache entries.
 */
void
flush_cache ()
{
  aCache *cp;

  while ((cp = cachetop))
    rem_cache (cp);
}

int
m_dns (cptr, sptr, parc, parv)
     aClient *cptr, *sptr;
     int parc;
     char *parv[];
{
  aCache *cp;
  int i;

  if (MyOper (sptr) && (parv[1] && *parv[1] == 'l'))
  {
    for (cp = cachetop; cp; cp = cp->list_next)
    {
      sendto_one (sptr, "NOTICE %s :Ex %d ttl %d host %s(%s)",
                  parv[0],
                  cp->expireat - timeofday, cp->ttl,
                  cp->he->h_name, inetntoa (cp->he->h_addr));

      for (i = 0; cp->he->h_aliases[i]; i++)
        sendto_one (sptr, "NOTICE %s : %s = %s (CN)",
                    parv[0], cp->he->h_name, cp->he->h_aliases[i]);

      for (i = 1; cp->he->h_addr_list[i]; i++)
        sendto_one (sptr, "NOTICE %s : %s = %s (IP)",
                    parv[0],
                    cp->he->h_name, inetntoa (cp->he->h_addr_list[i]));
    }
    return 0;
  }
  sendto_one (sptr, "NOTICE %s :Ca %d Cd %d Ce %d Cl %d Ch %d:%d Cu %d",
              sptr->name,
              cainfo.ca_adds, cainfo.ca_dels,
              cainfo.ca_expires,
              cainfo.ca_lookups,
              cainfo.ca_na_hits, cainfo.ca_nu_hits, cainfo.ca_updates);

  sendto_one (sptr, "NOTICE %s :Re %d Rl %d/%d Rp %d Rq %d",
              sptr->name,
              reinfo.re_errors, reinfo.re_nu_look,
              reinfo.re_na_look, reinfo.re_replies, reinfo.re_requests);

  sendto_one (sptr, "NOTICE %s :Ru %d Rsh %d Rs %d(%d) Rt %d", sptr->name,
              reinfo.re_unkrep, reinfo.re_shortttl, reinfo.re_sent,
              reinfo.re_resends, reinfo.re_timeouts);
  return 0;
}

u_long
cres_mem (sptr)
     aClient *sptr;
{
  register aCache *c = cachetop;
  register struct hostent *h;
  register int i;
  u_long nm = 0, im = 0, sm = 0, ts = 0;

  for (; c; c = c->list_next)
  {
    sm += sizeof (*c);
    h = c->he;
    for (i = 0; h->h_addr_list[i]; i++)
    {
      im += sizeof (char *);
      im += sizeof (struct in_addr);
    }
    im += sizeof (char *);
    for (i = 0; h->h_aliases[i]; i++)
    {
      nm += sizeof (char *);
      nm += strlen (h->h_aliases[i]);
    }
    nm += i - 1;
    nm += sizeof (char *);
    if (h->h_name)
      nm += strlen (h->h_name);
  }

  ts = ARES_CACSIZE * sizeof (CacheTable);

  sendto_one (sptr, ":%s %d %s :RES table %d",
              me.name, RPL_STATSDEBUG, sptr->name, ts);

  sendto_one (sptr, ":%s %d %s :Structs %d IP storage %d Name storage %d",
              me.name, RPL_STATSDEBUG, sptr->name, sm, im, nm);
  return ts + sm + im + nm;
}

/*
 * Main thread function for handling DNS requests.
 */
void
async_dns (void *parm)
{
  ResRQ *rptr = (ResRQ *) parm;
  struct hostent *hp, *he = rptr->he;
  int i, x;
  long amt;

  if (rptr->type == T_A)
  {
    rptr->locked = 2;
    hp = gethostbyname (rptr->name);
  }
  else
  {
    rptr->locked = 1;
    hp = gethostbyaddr ((char *) (&rptr->addr.s_addr), 4, PF_INET);
  }
  if (!hp)
  {
/*
 * Now heres a stupid check to forget, this apprently is
 * what hasbeen causing most of the crashes.I hope anyway.
 */
    do_dns_async (rptr->id);
    _endthread ();
  }

  if ((hp->h_aliases[0]
       && (hp->h_aliases[0] - (char *) hp) > MAXGETHOSTSTRUCT)
      || (hp->h_addr_list[0]
          && (hp->h_addr_list[0] - (char *) hp) > MAXGETHOSTSTRUCT))
  {

/*
 * Seems windows does some weird, aka stupid, stuff with DNS.
 * If the address is resolved from the HOSTS file, then the
 * pointers will exceed MAXGETHOSTSTRUCT. Good and bad. Good
 * because its an easy way to tell if the Admin is spoofing
 * with his HOSTS file, bad because it also causes invalid
 * pointers without this check. -Cabal95
 */
    do_dns_async (rptr->id);
    _endthread ();
  }

  res_copyhostent (hp, rptr->he);
  do_dns_async (rptr->id);
  _endthread ();
}

int
res_copyhostent (struct hostent *from, struct hostent *to)
{
  int amt, x, i;

  to->h_addrtype = from->h_addrtype;
  to->h_length = from->h_length;
/*
 * Get to "primary" offset in to hostent buffer and copy over
 * to hostname.
 */
  amt = (long) to + sizeof (struct hostent);
  to->h_name = (char *) amt;
  strcpy (to->h_name, from->h_name);
  amt += strlen (to->h_name) + 1;

/* Setup tto alias list */
  if (amt & 0x3)
    amt = (amt & 0xFFFFFFFC) + 4;
  to->h_aliases = (char **) amt;
  for (x = 0; from->h_aliases[x]; x++)
    ;
  x *= sizeof (char *);
  amt += sizeof (char *);
  for (i = 0; from->h_aliases[i]; i++)
  {
    to->h_aliases[i] = (char *) (amt + x);
    strcpy (to->h_aliases[i], from->h_aliases[i]);
    amt += strlen (to->h_aliases[i]) + 1;

    if (amt & 0x3)
      amt = (amt & 0xFFFFFFFC) + 4;
  }
  to->h_aliases[i] = NULL;
/* Setup tto IP address list */
  to->h_addr_list = (char **) amt;

 /*** FIXME ***/
/* ?? */
  for (x = 0; from->h_addr_list[x]; x++)
    ;
  x *= sizeof (char *);

  for (i = 0; from->h_addr_list[i]; i++)
  {
    amt += 4;
    to->h_addr_list[i] = (char *) (amt + x);
    ((struct in_addr *) to->h_addr_list[i])->s_addr =
      ((struct in_addr *) from->h_addr_list[i])->s_addr;
  }
  to->h_addr_list[i] = NULL;
}
