/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/drone.c
 *
 *  Copyright (C) 2002, DALnet coding team
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
 *  $Id: drone.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

/*
 *
 * NOTE
 * 
 * This file is essentially for drone detection functions,
 * to be called from register_user.
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include "throttle.h"
#include "h.h"

/* Forward declarations go here. */
int check_drone_PB (char *, char *);

#ifdef UNUSED
/* I couldn't find this used anywhere so its commented out
** Fish (23/08/2003)
*/

/*
 * drone_init
 * called once at ircd startup
 */
void
drone_init ()
{
}
#endif
/* 
 * is_a_drone:
 * main drone detection function.
 * returns 1 if client is a drone, zero otherwise.
 */
int
is_a_drone (aClient * source_p)
{
#ifdef REJECT_ACEBOTS
  if (check_drone_PB (source_p->user->username, source_p->info))
  {
    sendto_realops_lev (REJ_LEV,
                        "Rejecting acebot-style drone: %s (%s@%s) [%s]",
                        source_p->name, source_p->user->username,
                        source_p->user->host, source_p->info);
    return 1;
  }
#endif

  return 0;
}

/*
 * Returns 1 if the user matches a drone style
 * discovered by PB@DAL.net
 */
#ifdef REJECT_ACEBOTS
int
check_drone_PB (char *username, char *gcos)
{
  unsigned char *x;

  if (*username == '~')
    username++;

  if (strlen (username) <= 2)
    return 0;

  /* verify that it's all uppercase leters */
  for (x = (unsigned char *) username; *x; x++)
  {
    if (*x < 'A' || *x > 'Z')
      return 0;
  }

  if (strcmp (username, gcos))
    return 0;

  return 1;
}
#endif
