/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/fdlist.h
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
 *  $Id: fdlist.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


#ifndef _IRCD_DOG3_FDLIST
#define _IRCD_DOG3_FDLIST


typedef struct fdstruct
{
  int entry[MAXCONNECTIONS + 2];
  int last_entry;
}
fdlist;

void addto_fdlist (int a, fdlist * b);
void delfrom_fdlist (int a, fdlist * b);
void init_fdlist (fdlist * b);

#endif /* _IRCD_DOG3_FDLIST */
