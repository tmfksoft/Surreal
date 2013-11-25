/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/fdlist.c
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
 *  $Id: fdlist.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

/*
 * fdlist.c   maintain lists of certain important fds 
 */


#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "config.h"
#include "fdlist.h"

void
addto_fdlist (int fd, fdlist * listp)
{
  int myindex;

  if ((myindex = ++listp->last_entry) >= MAXCONNECTIONS)
  {
    /*
     * list too big.. must exit 
     */
    --listp->last_entry;

#ifdef	USE_SYSLOG
    (void) syslog (LOG_CRIT, "fdlist.c list too big.. must exit");
#endif
    abort ();
  }
  else
    listp->entry[myindex] = fd;
  return;
}

void
delfrom_fdlist (int fd, fdlist * listp)
{
  int i;

  for (i = listp->last_entry; i; i--)
  {
    if (listp->entry[i] == fd)
      break;
  }
  if (!i)
    return;                     /*
                                 * could not find it! 
                                 */
  /*
   * swap with last_entry 
   */
  if (i == listp->last_entry)
  {
    listp->entry[i] = 0;
    listp->last_entry--;
    return;
  }
  else
  {
    listp->entry[i] = listp->entry[listp->last_entry];
    listp->entry[listp->last_entry] = 0;
    listp->last_entry--;
    return;
  }
}

void
init_fdlist (fdlist * listp)
{
  listp->last_entry = 0;
  memset ((char *) listp->entry, '\0', sizeof (listp->entry));
  return;
}
