/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/inet.h
 *
 *  Copyright (c) 1983 Regents of the University of California.
 *
 *  Copyright (C) 1990-2002 by the past and present ircd coders, and others.
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
 *  $Id: inet.h 893 2005-03-20 09:32:17Z shadowmaster $
 *
 */

/*
 * Copyright (c) 1983 Regents of the University of California. All
 * rights reserved.
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with
 * the distribution and in all advertising materials mentioning
 * features or use of this software. Neither the name of the University
 * nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission. THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.
 * 
 */


/*
 * External definitions for functions in inet(3)
 */
#include "config.h"		/* for system definitions */

#ifdef	__alpha
#define	__u_l	unsigned int
#else
#define	__u_l	unsigned long
#endif

#ifndef _WIN32
# ifdef __STDC__
extern __u_l inet_addr (char *);
extern char *inet_ntoa (char *);
extern char *inet_ntop(int, const void *, char *, size_t); /*INET6*/
extern __u_l inet_makeaddr (int, int);
extern __u_l inet_network (char *);
extern __u_l inet_lnaof (struct in_addr);
extern __u_l inet_netof (struct in_addr);

# else
extern __u_l inet_addr ();
extern char *inet_ntoa ();
extern char *inet_ntop(int, const void *, char *, size_t); /*INET6*/
extern __u_l inet_makeaddr ();

# endif
#endif
extern __u_l inet_network ();
extern __u_l inet_lnaof ();
extern __u_l inet_netof ();

#undef __u_l
