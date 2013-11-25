/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_numeric.c
 *
 *  Copyright (C) 1990 Jarkko Oikarinen
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
 *  $Id: s_numeric.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "channel.h"
#include "h.h"

static char buffer[1024];

/*
 * * DoNumeric (replacement for the old do_numeric) *
 * 
 *      parc    number of arguments ('sender' counted as one!) *
 * parv[0]      pointer to 'sender' (may point to empty string) (not
 * used) *      parv[1]..parv[parc-1] *         pointers to additional
 * parameters, this is a NULL *         terminated list (parv[parc] ==
 * NULL). *
 * 
 * *WARNING* *  Numerics are mostly error reports. If there is
 * something *  wrong with the message, just *DROP* it! Don't even
 * think of *   sending back a neat error message -- big danger of
 * creating *   a ping pong error message...
 */
int
do_numeric (int numeric,
            aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  aChannel *channel_p;
  char *nick, *p;
  int i;

  if (parc < 1 || !IsServer (source_p))
    return 0;
  /*
   * Remap low number numerics. 
   */
  if (numeric < 100)
    numeric += 100;
  /*
   * * Prepare the parameter portion of the message into 'buffer'. *
   * (Because the buffer is twice as large as the message buffer * for
   * the socket, no overflow can occur here... ...on current *
   * assumptions--bets are off, if these are changed --msa) * Note: if
   * buffer is non-empty, it will begin with SPACE.
   */
  buffer[0] = '\0';
  if (parc > 1)
  {
    for (i = 2; i < (parc - 1); i++)
    {
      (void) strcat (buffer, " ");
      (void) strcat (buffer, parv[i]);
    }
    (void) strcat (buffer, " :");
    (void) strcat (buffer, parv[parc - 1]);
  }
  for (; (nick = strtoken (&p, parv[1], ",")); parv[1] = NULL)
  {
    if ((target_p = find_client (nick, (aClient *) NULL)))
    {
      /*
       * * Drop to bit bucket if for me... * ...one might consider
       * sendto_ops * here... --msa * And so it was done. -avalon *
       * And regretted. Dont do it that way. Make sure * it goes
       * only to non-servers. -avalon * Check added to make sure
       * servers don't try to loop * with numerics which can happen
       * with nick collisions. * - Avalon
       */
      if (!IsMe (target_p) && IsPerson (target_p))
        sendto_prefix_one (target_p, source_p, ":%s %d %s%s",
                           parv[0], numeric, nick, buffer);
      else if (IsServer (target_p) && target_p->from != client_p)
        sendto_prefix_one (target_p, source_p, ":%s %d %s%s",
                           parv[0], numeric, nick, buffer);
    }
    else if ((target_p = find_server (nick, (aClient *) NULL)))
    {
      if (!IsMe (target_p) && target_p->from != client_p)
        sendto_prefix_one (target_p, source_p, ":%s %d %s%s",
                           parv[0], numeric, nick, buffer);
    }
    else if ((channel_p = find_channel (nick, (aChannel *) NULL)))
      sendto_channel_butone (client_p, source_p, channel_p, ":%s %d %s%s",
                             parv[0], numeric, channel_p->chname, buffer);
  }
  return 0;
}
