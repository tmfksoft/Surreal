/*
 * UltimateIRCd - an Internet Relay Chat Daemon, include/res.h
 *  - A header with the DNS functions.
 *
 *  Copyright (C) 1992 Darren Reed.
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
 *  $Id: res.h 985 2007-02-04 20:51:36Z shadowmaster $
 */

#ifndef _RES_H_INCLUDED
#define _RES_H_INCLUDED 1

#include "config.h"

#ifdef _WIN32
#include "res_win32.h"
#else

#ifdef USE_ADNS

#include "adns.h"
#include "struct.h"
#define DNS_BLOCK_SIZE 64

struct DNSQuery {
	void *ptr;
	adns_query query;
	adns_answer *answer;
	void (*callback)(void* vptr, adns_answer *reply);
};

void init_resolver(void);
void restart_resolver(void);
int timeout_adns (time_t);
void dns_writeable (int fd , void *ptr );
void dns_readable (int fd , void *ptr );
void dns_do_callbacks(void);
void dns_select (void);
void adns_gethost (const char *name , int aftype , struct DNSQuery *req );
void adns_getaddr (struct in_addr addr , int aftype , struct DNSQuery *req, int arpa_type );
void delete_adns_queries(struct DNSQuery *q);
#if 0
void report_adns_servers(struct aClient *);
#endif

#else /* USE_ADNS */

#define	RES_INITLIST	1
#define	RES_CALLINIT	2
#define RES_INITSOCK	4
#define RES_INITDEBG	8
#define RES_INITCACH    16

#define MAXPACKET	1024
#define IRC_MAXALIASES	10
#define IRC_MAXADDRS	10

#define	AR_TTL		600	/* TTL in seconds for dns cache entries */


struct hent
{
  char *h_name;			/* official name of host */
  char *h_aliases[IRC_MAXALIASES];	/* alias list */
  int h_addrtype;		/* host address type */
  int h_length;			/* length of address */

  /* list of addresses from name server */
  struct IN_ADDR h_addr_list[IRC_MAXADDRS];

#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

typedef struct reslist
{
  int id;
  int sent;			/* number of requests sent */
  int srch;
  time_t ttl;
  char type;
  char retries;			/* retry counter */
  char sends;			/* number of sends (>1 means resent) */
  char resend;			/* send flag. 0 == dont resend */
  time_t sentat;
  time_t timeout;
  struct IN_ADDR addr;
  char *name;
  Link cinfo;
  struct hent he;
  int has_rev;			/* is he_rev valid? */
  struct hent he_rev;

  struct reslist *next;
  struct reslist *id_hashnext;
  struct reslist *cp_hashnext;
}
ResRQ;

typedef struct cache
{
  time_t expireat;
  time_t ttl;
  struct hostent he;
  struct cache *hname_next, *hnum_next, *list_next;
}
aCache;

typedef struct cachetable
{
  aCache *num_list;
  aCache *name_list;
}
CacheTable;

typedef struct reshash
{
  ResRQ *id_list;
  ResRQ *cp_list;
}
ResHash;

#define ARES_CACSIZE	307

#define ARES_IDCACSIZE  2099

#define	IRC_MAXCACHED	281

#endif /* USE_ADNS */

#endif /* _WIN32 */
#endif
