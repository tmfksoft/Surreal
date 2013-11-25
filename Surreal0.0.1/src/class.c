/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, class.c
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
 *  $Id: class.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "numeric.h"
#include "h.h"

#define BAD_CONF_CLASS		-1
#define BAD_PING		-2
#define BAD_CLIENT_CLASS	-3

aClass *classes;

int
get_conf_class (aConfItem * aconf)
{
  if ((aconf) && Class (aconf))
    return (ConfClass (aconf));

  Debug ((DEBUG_DEBUG, "No Class For %s",
          (aconf) ? aconf->name : "*No Conf*"));

  return (BAD_CONF_CLASS);

}

static int
get_conf_ping (aConfItem * aconf)
{
  if ((aconf) && Class (aconf))
    return (ConfPingFreq (aconf));

  Debug ((DEBUG_DEBUG, "No Ping For %s",
          (aconf) ? aconf->name : "*No Conf*"));

  return (BAD_PING);
}

int
get_client_class (aClient * target_p)
{
  Link *tmp;
  aClass *cl;
  int retc = BAD_CLIENT_CLASS;

  if (target_p && !IsMe (target_p) && (target_p->confs))
    for (tmp = target_p->confs; tmp; tmp = tmp->next)
    {
      if (!tmp->value.aconf || !(cl = tmp->value.aconf->class))
        continue;
      if (Class (cl) > retc)
        retc = Class (cl);
    }

  Debug ((DEBUG_DEBUG, "Returning Class %d For %s", retc, target_p->name));

  return (retc);
}

int
get_client_ping (aClient * target_p)
{
  int ping = 0, ping2;
  aConfItem *aconf;
  Link *mylink;

  mylink = target_p->confs;

  if (mylink)
    while (mylink)
    {
      aconf = mylink->value.aconf;
      if (aconf->status & (CONF_CLIENT | CONF_CONNECT_SERVER |
                           CONF_NOCONNECT_SERVER))
      {
        ping2 = get_conf_ping (aconf);
        if ((ping2 != BAD_PING) && ((ping > ping2) || !ping))
          ping = ping2;
      }
      mylink = mylink->next;
    }
  else
  {
    ping = PINGFREQUENCY;
    Debug ((DEBUG_DEBUG, "No Attached Confs"));
  }
  if (ping <= 0)
    ping = PINGFREQUENCY;
  Debug ((DEBUG_DEBUG, "Client %s Ping %d", target_p->name, ping));
  return (ping);
}

int
get_con_freq (aClass * clptr)
{
  if (clptr)
    return (ConFreq (clptr));
  else
    return (CONNECTFREQUENCY);
}

/*
 * When adding a class, check to see if it is already present first. if
 * so, then update the information for that class, rather than create a
 * new entry for it and later delete the old entry. if no present entry
 * is found, then create a new one and add it in immediately after the
 * first one (class 0).
 */
void
add_class (int class, int ping, int confreq, int maxli, long sendq)
{
  aClass *t, *p;

  t = find_class (class);
  if ((t == classes) && (class != 0))
  {
    p = (aClass *) make_class ();
    NextClass (p) = NextClass (t);
    NextClass (t) = p;
  }
  else
    p = t;
  Debug ((DEBUG_DEBUG,
          "Add Class %d: p %x t %x - cf: %d pf: %d ml: %d sq: %l",
          class, p, t, confreq, ping, maxli, sendq));
  Class (p) = class;

  ConFreq (p) = confreq;
  PingFreq (p) = ping;
  MaxLinks (p) = maxli;
  MaxSendq (p) = (sendq > 0) ? sendq : MAXSENDQLENGTH;
  if (p != t)
    Links (p) = 0;
}

aClass *
find_class (int cclass)
{
  aClass *cltmp;

  for (cltmp = FirstClass (); cltmp; cltmp = NextClass (cltmp))
    if (Class (cltmp) == cclass)
      return cltmp;
  return classes;
}

void
check_class ()
{
  aClass *cltmp, *cltmp2;

  Debug ((DEBUG_DEBUG, "Class check:"));

  for (cltmp2 = cltmp = FirstClass (); cltmp; cltmp = NextClass (cltmp2))
  {
    Debug ((DEBUG_DEBUG,
            "Class %d : CF: %d PF: %d ML: %d LI: %d SQ: %ld",
            Class (cltmp), ConFreq (cltmp), PingFreq (cltmp),
            MaxLinks (cltmp), Links (cltmp), MaxSendq (cltmp)));
    if (MaxLinks (cltmp) < 0)
    {
      NextClass (cltmp2) = NextClass (cltmp);
      if (Links (cltmp) <= 0)
        free_class (cltmp);
    }
    else
      cltmp2 = cltmp;
  }
}

void
initclass ()
{
  classes = (aClass *) make_class ();

  Class (FirstClass ()) = 0;

  ConFreq (FirstClass ()) = CONNECTFREQUENCY;
  PingFreq (FirstClass ()) = PINGFREQUENCY;
  MaxLinks (FirstClass ()) = MAXIMUM_LINKS;
  MaxSendq (FirstClass ()) = MAXSENDQLENGTH;
  Links (FirstClass ()) = 0;
  NextClass (FirstClass ()) = NULL;
}

void
report_classes (aClient * source_p)
{
  aClass *cltmp;

  for (cltmp = FirstClass (); cltmp; cltmp = NextClass (cltmp))
    sendto_one (source_p, rpl_str (RPL_STATSYLINE), me.name, source_p->name,
                'Y', Class (cltmp), PingFreq (cltmp), ConFreq (cltmp),
                MaxLinks (cltmp), MaxSendq (cltmp));
}

long
get_sendq (aClient * client_p)
{
  int sendq = MAXSENDQLENGTH, retc = BAD_CLIENT_CLASS;
  Link *tmp;
  aClass *cl;

  if (client_p && !IsMe (client_p) && (client_p->confs))
    for (tmp = client_p->confs; tmp; tmp = tmp->next)
    {
      if (!tmp->value.aconf || !(cl = tmp->value.aconf->class))
      {
        continue;
      }
      if (Class (cl) > retc)
      {
        retc = Class (cl);
        sendq = MaxSendq (cl);
      }
    }
  return sendq;
}
