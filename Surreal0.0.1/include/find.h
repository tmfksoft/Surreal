/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/find.h
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
 *  $Id: find.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


#ifndef	__find_include__
#define __find_include__

#define find_server(a, b)       hash_find_server(a, b)
#define find_name(a, b)         hash_find_server(a, b)
#define find_client(a, b)       hash_find_client(a, b)

static inline aClient *
find_person (char *name, aClient * client_p)
{
  aClient *target_p = find_client (name, client_p);

  return target_p ? (IsClient (target_p) ? target_p : client_p) : client_p;
}

#endif /* __find_include__ */
