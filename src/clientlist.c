/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/clientlist.c
 *
 *  Copyright (C) 2003 Lucas Madar
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
 *  $Id: clientlist.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "numeric.h"
#include "blalloc.h"

DLink *server_list = NULL;
DLink *oper_list = NULL;

/* Clients currently doing a /list */
DLink *listing_clients = NULL;
DLink *recvq_clients = NULL;

static int get_list_memory (DLink *);


int
get_list_memory (DLink * list)
{
  DLink *lp;
  int count = 0;

  for (lp = list; lp; lp = lp->next)
    count++;

  return count;
}

void
print_list_memory (aClient * cptr)
{
  int lc;

  lc = get_list_memory (server_list);
  sendto_one (cptr, ":%s %d %s :   server_list %d(%d)",
              me.name, RPL_STATSDEBUG, cptr->name, lc, lc * sizeof (DLink));

  lc = get_list_memory (oper_list);
  sendto_one (cptr, ":%s %d %s :   oper_list %d(%d)",
              me.name, RPL_STATSDEBUG, cptr->name, lc, lc * sizeof (DLink));

  lc = get_list_memory (listing_clients);
  sendto_one (cptr, ":%s %d %s :   listing_clients %d(%d)",
              me.name, RPL_STATSDEBUG, cptr->name, lc, lc * sizeof (DLink));

  lc = get_list_memory (recvq_clients);
  sendto_one (cptr, ":%s %d %s :   recvq_clients %d(%d)",
              me.name, RPL_STATSDEBUG, cptr->name, lc, lc * sizeof (DLink));
}

DLink *
add_to_list (DLink ** list, void *ptr)
{
  DLink *lp = make_dlink ();

  lp->value.cp = (char *) ptr;
  lp->next = *list;
  lp->prev = NULL;
  if (lp->next)
    lp->next->prev = lp;
  *list = lp;

  return lp;
}

static inline void
remove_dlink_list (DLink ** list, DLink * mylink)
{
  if (mylink->next)
    mylink->next->prev = mylink->prev;

  if (mylink->prev)
    mylink->prev->next = mylink->next;
  else
  {
    *list = mylink->next;
    if (*list)
      (*list)->prev = NULL;
  }

  free_dlink (mylink);
}

void
remove_from_list (DLink ** list, void *ptr, DLink * mylink)
{
  DLink *lp;

  if (mylink)
  {
    remove_dlink_list (list, mylink);
    return;
  }

  for (lp = *list; lp; lp = lp->next)
  {
    if (lp->value.cp == (char *) ptr)
    {
      remove_dlink_list (list, lp);
      return;
    }
  }

  sendto_realops ("remove_from_list(%x, %x) failed!!", (int) list, (int) ptr);
}
