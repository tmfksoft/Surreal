/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/whowas.c
 *
 *  Copyright (C) 1990 Markku Savela
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
 *  $Id: whowas.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "h.h"
#include "whowas.h"

/* externally defined functions */
unsigned int hash_whowas_name (char *); /*

                                         * defined in hash.c 
                                         */
/*
 * internally defined function 
 */
static void add_whowas_to_clist (aWhowas **, aWhowas *);
static void del_whowas_from_clist (aWhowas **, aWhowas *);
static void add_whowas_to_list (aWhowas **, aWhowas *);
static void del_whowas_from_list (aWhowas **, aWhowas *);

aWhowas WHOWAS[NICKNAMEHISTORYLENGTH];
aWhowas *WHOWASHASH[WW_MAX];

int whowas_next = 0;

void
add_history (aClient * client_p, int online)
{
  aWhowas *new;

  new = &WHOWAS[whowas_next];

  if (new->hashv != -1)
  {
    if (new->online)
      del_whowas_from_clist (&(new->online->whowas), new);
    del_whowas_from_list (&WHOWASHASH[new->hashv], new);
  }
  new->hashv = hash_whowas_name (client_p->name);
  new->logoff = NOW;
  strncpyzt (new->name, client_p->name, NICKLEN + 1);
  strncpyzt (new->username, client_p->user->username, USERLEN + 1);
  strncpyzt (new->hostname, client_p->user->host, HOSTLEN);
  strncpyzt (new->virthostname, client_p->user->virthost, HOSTLEN);
  strncpyzt (new->hostipname, client_p->hostip, HOSTLEN);
  strncpyzt (new->realname, client_p->info, REALLEN);

  if (IsHidden (client_p))
    new->washidden = 1;
  else
    new->washidden = 0;
  /*
   * Its not string copied, a pointer to the scache hash is copied
   * -Dianora
   */
  /*
   * strncpyzt(new->servername, client_p->user->server,HOSTLEN); 
   */
  new->servername = client_p->user->server;

  if (online)
  {
    new->online = client_p;
    add_whowas_to_clist (&(client_p->whowas), new);
  }
  else
    new->online = NULL;
  add_whowas_to_list (&WHOWASHASH[new->hashv], new);
  whowas_next++;
  if (whowas_next == NICKNAMEHISTORYLENGTH)
    whowas_next = 0;
}

void
off_history (aClient * client_p)
{
  aWhowas *temp, *next;

  for (temp = client_p->whowas; temp; temp = next)
  {
    next = temp->cnext;
    temp->online = NULL;
    del_whowas_from_clist (&(client_p->whowas), temp);
  }
}

aClient *
get_history (char *nick, time_t timelimit)
{
  aWhowas *temp;
  int blah;

  timelimit = NOW - timelimit;
  blah = hash_whowas_name (nick);
  temp = WHOWASHASH[blah];
  for (; temp; temp = temp->next)
  {
    if (irccmp (nick, temp->name) != 0)
      continue;
    if (temp->logoff < timelimit)
      continue;
    return temp->online;
  }
  return NULL;
}

void
count_whowas_memory (int *wwu, u_long * wwum)
{
  aWhowas *tmp;
  int i;
  int u = 0;
  u_long um = 0;

  /*
   * count the number of used whowas structs in 'u' 
   */
  /*
   * count up the memory used of whowas structs in um 
   */

  for (i = 0, tmp = &WHOWAS[0]; i < NICKNAMEHISTORYLENGTH; i++, tmp++)
    if (tmp->hashv != -1)
    {
      u++;
      um += sizeof (aWhowas);
    }
  *wwu = u;
  *wwum = um;
  return;
}

/*
 * * m_whowas *      parv[0] = sender prefix *      parv[1] = nickname
 * queried
 */
int
m_whowas (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aWhowas *temp;
  int cur = 0;
  int max = -1, found = 0;
  char *p, *nick, *s;

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NONICKNAMEGIVEN), me.name, parv[0]);
    return 0;
  }
  if (parc > 2)
    max = atoi (parv[2]);
  if (parc > 3)
    if (hunt_server
        (client_p, source_p, ":%s WHOWAS %s %s :%s", 3, parc, parv))
      return 0;

  parv[1] = canonize (parv[1]);
  if (!MyConnect (source_p) && (max > 20))
    max = 20;
  for (s = parv[1]; (nick = strtoken (&p, s, ",")); s = NULL)
  {
    temp = WHOWASHASH[hash_whowas_name (nick)];
    found = 0;
    for (; temp; temp = temp->next)
    {
      if (irccmp (nick, temp->name) == 0)
      {
        sendto_one (source_p, rpl_str (RPL_WHOWASUSER),
                    me.name, parv[0], temp->name,
                    temp->username,
                    (temp->washidden ? temp->virthostname : temp->
                     hostname), temp->realname);
        if (IsAnOper (source_p))
          sendto_one (source_p, rpl_str (RPL_WHOISHOST),
                      me.name, parv[0], temp->name,
                      temp->hostname, temp->hostipname);

        sendto_one (source_p, rpl_str (RPL_WHOISSERVER),
                    me.name, parv[0], temp->name,
                    temp->servername, myctime (temp->logoff));
        cur++;
        found++;
      }
      if (max > 0 && cur >= max)
        break;
    }
    if (!found)
      sendto_one (source_p, err_str (ERR_WASNOSUCHNICK),
                  me.name, parv[0], nick);
    if (p)
      p[-1] = ',';
  }
  sendto_one (source_p, rpl_str (RPL_ENDOFWHOWAS), me.name, parv[0], parv[1]);
  return 0;
}

void
initwhowas ()
{
  int i;

  for (i = 0; i < NICKNAMEHISTORYLENGTH; i++)
  {
    memset ((char *) &WHOWAS[i], '\0', sizeof (aWhowas));
    WHOWAS[i].hashv = -1;
  }
  for (i = 0; i < WW_MAX; i++)
    WHOWASHASH[i] = NULL;
}

static void
add_whowas_to_clist (aWhowas ** bucket, aWhowas * whowas)
{
  whowas->cprev = NULL;
  if ((whowas->cnext = *bucket) != NULL)
    whowas->cnext->cprev = whowas;
  *bucket = whowas;
}

static void
del_whowas_from_clist (aWhowas ** bucket, aWhowas * whowas)
{
  if (whowas->cprev)
    whowas->cprev->cnext = whowas->cnext;
  else
    *bucket = whowas->cnext;
  if (whowas->cnext)
    whowas->cnext->cprev = whowas->cprev;
}

static void
add_whowas_to_list (aWhowas ** bucket, aWhowas * whowas)
{
  whowas->prev = NULL;
  if ((whowas->next = *bucket) != NULL)
    whowas->next->prev = whowas;
  *bucket = whowas;
}

static void
del_whowas_from_list (aWhowas ** bucket, aWhowas * whowas)
{
  if (whowas->prev)
    whowas->prev->next = whowas->next;
  else
    *bucket = whowas->next;
  if (whowas->next)
    whowas->next->prev = whowas->prev;
}
