/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/res_win32.h
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
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
 *  $Id: res.h,v 1.3 2002/10/19 00:58:03 shadowmaster Exp $
 *
 */

#ifndef	__res_include__
#define	__res_include__

#define	RES_INITLIST	1
#define	RES_CALLINIT	2
#define RES_INITSOCK	4
#define RES_INITDEBG	8
#define RES_INITCACH    16

#define MAXPACKET	1024
#define MAXALIASES	35
#define MAXADDRS	35

#define	AR_TTL		600	/* TTL in seconds for dns cache entries */

struct hent
{
  char *h_name;			/* official name of host */
  char *h_aliases[MAXALIASES];	/* alias list */
  int h_addrtype;		/* host address type */
  int h_length;			/* length of address */
  /* list of addresses from name server */
  struct in_addr h_addr_list[MAXADDRS];
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
  struct in_addr addr;
  char *name;
  struct reslist *next;
  Link cinfo;
  struct hostent *he;
  char locked;
}
ResRQ;

typedef struct cache
{
  time_t expireat;
  time_t ttl;
  struct hostent *he;
  struct cache *hname_next, *hnum_next, *list_next;
}
aCache;

typedef struct cachetable
{
  aCache *num_list;
  aCache *name_list;
}
CacheTable;

#define ARES_CACSIZE	101

#define	MAXCACHED	81

#endif /* __res_include__ */
