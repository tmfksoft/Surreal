/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/channel.c
 *
 *  Copyright (C) 1990 Jarkko Oikarinen and University of Oulu, Computing Center
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
 *  $Id: channel.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "channel.h"
#include "h.h"

#ifdef NO_CHANOPS_WHEN_SPLIT
#include "fdlist.h"

int server_was_split = YES;
time_t server_split_time = 0;
int server_split_recovery_time = (MAX_SERVER_SPLIT_RECOVERY_TIME * 60);

#endif

extern aCRline *crlines;
extern char *cannotjoin_msg;

aChannel *channel = NullChn;

static void add_invite (aClient *, aChannel *);
static int add_banid (aClient *, aChannel *, char *);
static int add_exemptid (aClient *, aChannel *, char *);
static int can_join (aClient *, aChannel *, char *);
static void channel_modes (aClient *, char *, char *, aChannel *);
static int del_banid (aChannel *, char *);
static int del_exemptid (aChannel *, char *);
static aBan *is_banned (aClient *, aChannel *);
static anExempt *is_exempted (aClient *, aChannel *);
static int set_mode (aClient *, aClient *, aChannel *, int, int, char **,
                     char *, char *);
static void sub1_from_channel (aChannel *);

int check_channelname (aClient *, unsigned char *);
void clean_channelname (unsigned char *);
void del_invite (aClient *, aChannel *);
static int check_modes (int, char, char);
#if 0
static void kill_ban_list (aClient *, aChannel *);
static void kill_exempt_list (aClient *, aChannel *);
#endif

#ifdef ORATIMING
struct timeval tsdnow, tsdthen;
unsigned long tsdms;
#endif

/* number of seconds to add to all readings of time() when making TS's */

static char *PartFmt = ":%s PART %s";
static char *PartFmt2 = ":%s PART %s :%s";


static char *newSJOINFmt = ":%s SJOIN %ld %s %s %s :%s";
static char *newSJOINFmtNP = ":%s SJOIN %ld %s %s :%s";
static char *newCliSJOINFmt = ":%s SJOIN %ld %s";

/* some buffers for rebuilding channel/nick lists with ,'s */
static char nickbuf[BUFSIZE], buf[BUFSIZE];
static char modebuf[REALMODEBUFLEN], parabuf[REALMODEBUFLEN];

/* htm ... */
extern int lifesux;

#ifdef ANTI_SPAMBOT
extern int spam_num;            /* defined in s_serv.c */
extern int spam_time;           /* defined in s_serv.c */
#endif

/* return the length (>=0) of a chain of links. */
static int
list_length (Link * lp)
{
  int count = 0;

  for (; lp; lp = lp->next)
    count++;
  return count;
}

/*
 * find_chasing
 *   Find the client structure for a nick name (user) using history 
 *   mechanism if necessary. If the client is not found, an error message 
 *   (NO SUCH NICK) is generated. If the client was found through the 
 *   history, chasing will be 1 and otherwise 0.
 */
aClient *
find_chasing (aClient * source_p, char *user, int *chasing)
{
  aClient *who = find_client (user, (aClient *) NULL);

  if (chasing)
    *chasing = 0;
  if (who)
    return who;
  if (!(who = get_history (user, (long) KILLCHASETIMELIMIT)))
  {
    sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, source_p->name,
                user);
    return ((aClient *) NULL);
  }
  if (chasing)
    *chasing = 1;
  return who;
}

/*
 * Fixes a string so that the first white space found becomes an end of
 * string marker (`\-`).  returns the 'fixed' string or "*" if the
 * string was NULL length or a NULL pointer.
 */
static char *
check_string (char *s)
{
  static char star[2] = "*";
  char *str = s;

  if (BadPtr (s))
    return star;

  for (; *s; s++)
    if (MyIsSpace (*s))
    {
      *s = '\0';
      break;
    }

  return (BadPtr (str)) ? star : str;
}

/*
 * create a string of form "foo!bar@fubar" given foo, bar and fubar as
 * the parameters.  If NULL, they become "*".
 */
static char *
make_nick_user_host (char *nick, char *name, char *host)
{
  static char namebuf[NICKLEN + USERLEN + HOSTLEN + 6];
  int n;
  char *ptr1, *ptr2;

  ptr1 = namebuf;
  for (ptr2 = check_string (nick), n = NICKLEN; *ptr2 && n--;)
    *ptr1++ = *ptr2++;
  *ptr1++ = '!';
  for (ptr2 = check_string (name), n = USERLEN; *ptr2 && n--;)
    *ptr1++ = *ptr2++;
  *ptr1++ = '@';
  for (ptr2 = check_string (host), n = HOSTLEN; *ptr2 && n--;)
    *ptr1++ = *ptr2++;
  *ptr1 = '\0';
  return (namebuf);
}

/* Ban functions to work with mode +b */
/* add_banid - add an id to be banned to the channel  (belongs to client_p) */

static int
add_banid (aClient * client_p, aChannel * channel_p, char *banid)
{
  aBan *ban;
  int cnt = 0;
  chanMember *cm;
  char *s, nickuhost[NICKLEN + USERLEN + HOSTLEN + 6];

  for (ban = channel_p->banlist; ban; ban = ban->next)
  {
    if (MyClient (client_p) && (++cnt >= MAXBANS))
    {
      sendto_one (client_p, getreply (ERR_BANLISTFULL), me.name,
                  client_p->name, channel_p->chname, banid);
      return -1;
    }
    /* yikes, we were doing all sorts of weird crap here before, now
     * we ONLY want to know if current bans cover this ban, not if this
     * ban covers current ones, since it may cover other things too -wd */
    else if (!match (ban->banstr, banid))
      return -1;
  }

  ban = (aBan *) MyMalloc (sizeof (aBan));
  ban->banstr = (char *) MyMalloc (strlen (banid) + 1);
  (void) strcpy (ban->banstr, banid);
  ban->next = channel_p->banlist;

  if (IsPerson (client_p))
  {
    ban->who = (char *) MyMalloc (strlen (client_p->name) +
                                  strlen (client_p->user->username) +
                                  strlen (IsHidden (client_p) ? client_p->
                                          user->virthost : client_p->user->
                                          host) + 3);
    (void) ircsprintf (ban->who, "%s!%s@%s", client_p->name,
                       client_p->user->username,
                       (IsHidden (client_p) ? client_p->user->
                        virthost : client_p->user->host));
  }
  else
  {
    ban->who = (char *) MyMalloc (strlen (client_p->name) + 1);
    (void) strcpy (ban->who, client_p->name);
  }

  /* determine what 'type' of mask this is, for less matching later */

  if (banid[0] == '*' && banid[1] == '!')
  {
    if (banid[2] == '*' && banid[3] == '@')
      ban->type = MTYP_HOST;
    else
      ban->type = MTYP_USERHOST;
  }
  else
    ban->type = MTYP_FULL;

  ban->when = timeofday;
  channel_p->banlist = ban;

  for (cm = channel_p->members; cm; cm = cm->next)
  {
    if (!MyConnect (cm->client_p))
      continue;

    strcpy (nickuhost,
            make_nick_user_host (cm->client_p->name,
                                 cm->client_p->user->username,
                                 cm->client_p->hostip));
    s =
      make_nick_user_host (cm->client_p->name, cm->client_p->user->username,
                           cm->client_p->user->host);
    if (match (banid, nickuhost) == 0 || match (banid, s) == 0)
      cm->bans++;
  }
  return 0;
}

/*
 * del_banid - delete an id belonging to client_p if banid is null,
 * deleteall banids belonging to client_p.
 */
static int
del_banid (aChannel * channel_p, char *banid)
{
  aBan **ban;
  aBan *tmp;
  chanMember *cm;
  char *s, nickuhost[NICKLEN + USERLEN + HOSTLEN + 6];

  if (!banid)
    return -1;
  for (ban = &(channel_p->banlist); *ban; ban = &((*ban)->next))
  {
    if (irccmp (banid, (*ban)->banstr) == 0)
    {
      tmp = *ban;
      *ban = tmp->next;
      {

        for (cm = channel_p->members; cm; cm = cm->next)
        {
          if (!MyConnect (cm->client_p) || cm->bans == 0)
            continue;

          strcpy (nickuhost,
                  make_nick_user_host (cm->client_p->name,
                                       cm->client_p->user->username,
                                       cm->client_p->hostip));
          s =
            make_nick_user_host (cm->client_p->name,
                                 cm->client_p->user->username,
                                 cm->client_p->user->host);

          if (match (banid, nickuhost) == 0 || match (banid, s) == 0)
            cm->bans--;
        }
        MyFree (tmp->banstr);
        MyFree (tmp->who);
        MyFree (tmp);
      }
      return 0;
    }
  }
  return -1;
}

/*
 * is_banned - returns a pointer to the ban structure if banned else
 * NULL
 * 
 * IP_BAN_ALL from comstud always on...
 */

static aBan *
is_banned (aClient * client_p, aChannel * channel_p)
{
  aBan *tmp;
  char s[NICKLEN + USERLEN + HOSTLEN + 6];
  char v[NICKLEN + USERLEN + HOSTLEN + 6];
  char *s2;

  if (!IsPerson (client_p))
    return NULL;

  strcpy (s, make_nick_user_host (client_p->name, client_p->user->username,
                                  client_p->user->host));
  strcpy (v, make_nick_user_host (client_p->name, client_p->user->username,
                                  client_p->user->virthost));
  s2 =
    make_nick_user_host (client_p->name, client_p->user->username,
                         client_p->hostip);

  for (tmp = channel_p->banlist; tmp; tmp = tmp->next)
    if ((match (tmp->banstr, s) == 0) ||
        (match (tmp->banstr, s2) == 0) || (match (tmp->banstr, v) == 0))
      break;
  return (tmp);
}

aBan *
nick_is_banned (aChannel * channel_p, char *nick, aClient * client_p)
{
  aBan *tmp;
  char *s, s2[NICKLEN + USERLEN + HOSTLEN + 6];
  char v[NICKLEN + USERLEN + HOSTLEN + 6];

  if (!IsPerson (client_p))
    return NULL;

  strcpy (s2,
          make_nick_user_host (nick, client_p->user->username,
                               client_p->user->host));
  strcpy (v,
          make_nick_user_host (nick, client_p->user->username,
                               client_p->user->virthost));
  s = make_nick_user_host (nick, client_p->user->username, client_p->hostip);

  for (tmp = channel_p->banlist; tmp; tmp = tmp->next)
    if (tmp->type == MTYP_FULL &&       /* only check applicable bans */
        ((match (tmp->banstr, s2) == 0) ||      /* check host before IP */
         (match (tmp->banstr, s) == 0) || (match (tmp->banstr, v) == 0)))
      break;
  return (tmp);
}

void
remove_matching_bans (aChannel * channel_p, aClient * client_p,
                      aClient * from)
{
  aBan *ban, *bnext;
  char targhost[NICKLEN + USERLEN + HOSTLEN + 6];
  char targip[NICKLEN + USERLEN + HOSTLEN + 6];
  char targvirthost[NICKLEN + USERLEN + HOSTLEN + 6];
  char *m;
  int count = 0, mysend = 0;

  if (!IsPerson (client_p))
    return;

  strcpy (targhost,
          make_nick_user_host (client_p->name, client_p->user->username,
                               client_p->user->host));
  strcpy (targip,
          make_nick_user_host (client_p->name, client_p->user->username,
                               client_p->hostip));
  strcpy (targvirthost,
          make_nick_user_host (client_p->name, client_p->user->username,
                               client_p->user->virthost));

  m = modebuf;
  *m++ = '-';
  *m = '\0';

  *parabuf = '\0';

  ban = channel_p->banlist;

  while (ban)
  {
    bnext = ban->next;
    if ((match (ban->banstr, targhost) == 0)
        || (match (ban->banstr, targip) == 0)
        || (match (ban->banstr, targvirthost) == 0))
    {
      if (strlen (parabuf) + strlen (ban->banstr) + 10 < (size_t) MODEBUFLEN)
      {
        if (*parabuf)
          strcat (parabuf, " ");
        strcat (parabuf, ban->banstr);
        count++;
        *m++ = 'b';
        *m = '\0';
      }
      else if (*parabuf)
        mysend = 1;

      if (count == MAXTSMODEPARAMS)
        mysend = 1;

      if (mysend)
      {
        sendto_channel_butserv (channel_p, from, ":%s MODE %s %s %s",
                                from->name, channel_p->chname, modebuf,
                                parabuf);
        sendto_tsmode_servs (0, channel_p, from, ":%s MODE %s %s %s",
                             from->name, channel_p->chname, modebuf, parabuf);
        sendto_tsmode_servs (1, channel_p, from,
                             ":%s MODE %s %ld %s %s", from->name,
                             channel_p->chname, channel_p->channelts,
                             modebuf, parabuf);

        mysend = 0;
        *parabuf = '\0';
        m = modebuf;
        *m++ = '-';
        if (count != MAXTSMODEPARAMS)
        {
          strcpy (parabuf, ban->banstr);
          *m++ = 'b';
          count = 1;
        }
        else
          count = 0;
        *m = '\0';
      }

      del_banid (channel_p, ban->banstr);
    }
    ban = bnext;
  }

  if (*parabuf)
  {
    sendto_channel_butserv (channel_p, from, ":%s MODE %s %s %s",
                            from->name, channel_p->chname, modebuf, parabuf);

    sendto_tsmode_servs (0, channel_p, from, ":%s MODE %s %s %s",
                         from->name, channel_p->chname, modebuf, parabuf);

    sendto_tsmode_servs (1, channel_p, from, ":%s MODE %s %ld %s %s",
                         from->name, channel_p->chname,
                         channel_p->channelts, modebuf, parabuf);
  }

  return;
}

/* Exempt functions to work with mode +e */
/* add_exemptid - add an id to be exempted to the channel  (belongs to client_p) */
/* Based on the original add_banid from Bahamut */

static int
add_exemptid (aClient * client_p, aChannel * channel_p, char *exemptid)
{
  anExempt *exempt;
  int cnt = 0;
  chanMember *cm;
  char *s, nickuhost[NICKLEN + USERLEN + HOSTLEN + 6];

  for (exempt = channel_p->exemptlist; exempt; exempt = exempt->next)
  {
    if (MyClient (client_p) && (++cnt >= MAXEXEMPTS))
    {
      sendto_one (client_p, getreply (ERR_EXEMPTLISTFULL), me.name,
                  client_p->name, channel_p->chname, exemptid);
      return -1;
    }
    /* yikes, we were doing all sorts of weird crap here before, now
     * we ONLY want to know if current exempts cover this exempt, not if this
     * exempt covers current ones, since it may cover other things too */
    else if (!match (exempt->exemptstr, exemptid))
      return -1;
  }

  exempt = (anExempt *) MyMalloc (sizeof (anExempt));
  exempt->exemptstr = (char *) MyMalloc (strlen (exemptid) + 1);
  (void) strcpy (exempt->exemptstr, exemptid);
  exempt->next = channel_p->exemptlist;

  if (IsPerson (client_p))
  {
    exempt->who = (char *) MyMalloc (strlen (client_p->name) +
                                     strlen (client_p->user->username) +
                                     strlen (IsHidden (client_p) ?
                                             client_p->user->
                                             virthost : client_p->user->
                                             host) + 3);
    (void) ircsprintf (exempt->who, "%s!%s@%s", client_p->name,
                       client_p->user->username,
                       (IsHidden (client_p) ? client_p->user->
                        virthost : client_p->user->host));
  }
  else
  {
    exempt->who = (char *) MyMalloc (strlen (client_p->name) + 1);
    (void) strcpy (exempt->who, client_p->name);
  }

  /* determine what 'type' of mask this is, for less matching later */

  if (exemptid[0] == '*' && exemptid[1] == '!')
  {
    if (exemptid[2] == '*' && exemptid[3] == '@')
      exempt->type = MTYP_HOST;
    else
      exempt->type = MTYP_USERHOST;
  }
  else
    exempt->type = MTYP_FULL;

  exempt->when = timeofday;
  channel_p->exemptlist = exempt;

  for (cm = channel_p->members; cm; cm = cm->next)
  {
    if (!MyConnect (cm->client_p))
      continue;

    strcpy (nickuhost,
            make_nick_user_host (cm->client_p->name,
                                 cm->client_p->user->username,
                                 cm->client_p->hostip));
    s =
      make_nick_user_host (cm->client_p->name, cm->client_p->user->username,
                           cm->client_p->user->host);
    if (match (exemptid, nickuhost) == 0 || match (exemptid, s) == 0)
      cm->bans--;

  }
  return 0;
}

/*
 * del_exemptid - delete an id belonging to client_p if execmptid is null,
 * deleteall exemptids belonging to client_p.
 * Based on the del_banid from Bahamut
 */
static int
del_exemptid (aChannel * channel_p, char *exemptid)
{
  anExempt **exempt;
  anExempt *tmp;
  chanMember *cm;
  char *s, nickuhost[NICKLEN + USERLEN + HOSTLEN + 6];

  if (!exemptid)
    return -1;
  for (exempt = &(channel_p->exemptlist); *exempt;
       exempt = &((*exempt)->next))
  {
    if (irccmp (exemptid, (*exempt)->exemptstr) == 0)
    {
      tmp = *exempt;
      *exempt = tmp->next;
      {
        for (cm = channel_p->members; cm; cm = cm->next)
        {
          if (!MyConnect (cm->client_p))
            continue;

          strcpy (nickuhost,
                  make_nick_user_host (cm->client_p->name,
                                       cm->client_p->user->username,
                                       cm->client_p->hostip));
          s =
            make_nick_user_host (cm->client_p->name,
                                 cm->client_p->user->username,
                                 cm->client_p->user->host);

          if (match (exemptid, nickuhost) == 0 || match (exemptid, s) == 0)
            cm->bans++;
        }
        MyFree (tmp->exemptstr);
        MyFree (tmp->who);
        MyFree (tmp);
      }
      return 0;
    }
  }
  return -1;
}

/*
 * is_exempted - returns a pointer to the exempt structure if exempted else
 * NULL
 * 
 * IP_BAN_ALL from comstud always on...
 */

static anExempt *
is_exempted (aClient * client_p, aChannel * channel_p)
{
  anExempt *tmp;
  char s[NICKLEN + USERLEN + HOSTLEN + 6];
  char v[NICKLEN + USERLEN + HOSTLEN + 6];
  char *s2;

  if (!IsPerson (client_p))
    return NULL;

  strcpy (s, make_nick_user_host (client_p->name, client_p->user->username,
                                  client_p->user->host));
  strcpy (v, make_nick_user_host (client_p->name, client_p->user->username,
                                  client_p->user->virthost));
  s2 =
    make_nick_user_host (client_p->name, client_p->user->username,
                         client_p->hostip);

  for (tmp = channel_p->exemptlist; tmp; tmp = tmp->next)
    if ((match (tmp->exemptstr, s) == 0) ||
        (match (tmp->exemptstr, s2) == 0) || (match (tmp->exemptstr, v) == 0))
      break;
  return (tmp);
}

anExempt *
nick_is_exempted (aChannel * channel_p, char *nick, aClient * client_p)
{
  anExempt *tmp;
  char *s, s2[NICKLEN + USERLEN + HOSTLEN + 6];
  char v[NICKLEN + USERLEN + HOSTLEN + 6];

  if (!IsPerson (client_p))
    return NULL;

  strcpy (s2,
          make_nick_user_host (nick, client_p->user->username,
                               client_p->user->host));
  strcpy (v,
          make_nick_user_host (nick, client_p->user->username,
                               client_p->user->virthost));
  s = make_nick_user_host (nick, client_p->user->username, client_p->hostip);

  for (tmp = channel_p->exemptlist; tmp; tmp = tmp->next)
    if (tmp->type == MTYP_FULL &&       /* only check applicable bans */
        ((match (tmp->exemptstr, s2) == 0) ||   /* check host before IP */
         (match (tmp->exemptstr, s) == 0)
         || (match (tmp->exemptstr, v) == 0)))
      break;
  return (tmp);
}

void
remove_matching_exempts (aChannel * channel_p, aClient * client_p,
                         aClient * from)
{
  anExempt *exempt, *bnext;
  char targhost[NICKLEN + USERLEN + HOSTLEN + 6];
  char targip[NICKLEN + USERLEN + HOSTLEN + 6];
  char targvirthost[NICKLEN + USERLEN + HOSTLEN + 6];
  char *m;
  int count = 0, mysend = 0;

  if (!IsPerson (client_p))
    return;

  strcpy (targhost,
          make_nick_user_host (client_p->name, client_p->user->username,
                               client_p->user->host));
  strcpy (targip,
          make_nick_user_host (client_p->name, client_p->user->username,
                               client_p->hostip));
  strcpy (targvirthost,
          make_nick_user_host (client_p->name, client_p->user->username,
                               client_p->user->virthost));

  m = modebuf;
  *m++ = '-';
  *m = '\0';

  *parabuf = '\0';

  exempt = channel_p->exemptlist;

  while (exempt)
  {
    bnext = exempt->next;
    if ((match (exempt->exemptstr, targhost) == 0)
        || (match (exempt->exemptstr, targip) == 0)
        || (match (exempt->exemptstr, targvirthost) == 0))
    {
      if (strlen (parabuf) + strlen (exempt->exemptstr) + 10 <
          (size_t) MODEBUFLEN)
      {
        if (*parabuf)
          strcat (parabuf, " ");
        strcat (parabuf, exempt->exemptstr);
        count++;
        *m++ = 'e';
        *m = '\0';
      }
      else if (*parabuf)
        mysend = 1;

      if (count == MAXTSMODEPARAMS)
        mysend = 1;

      if (mysend)
      {
        sendto_channel_butserv (channel_p, from, ":%s MODE %s %s %s",
                                from->name, channel_p->chname, modebuf,
                                parabuf);
        sendto_tsmode_servs (0, channel_p, from, ":%s MODE %s %s %s",
                             from->name, channel_p->chname, modebuf, parabuf);
        sendto_tsmode_servs (1, channel_p, from,
                             ":%s MODE %s %ld %s %s", from->name,
                             channel_p->chname, channel_p->channelts,
                             modebuf, parabuf);
        mysend = 0;
        *parabuf = '\0';
        m = modebuf;
        *m++ = '-';
        if (count != MAXTSMODEPARAMS)
        {
          strcpy (parabuf, exempt->exemptstr);
          *m++ = 'e';
          count = 1;
        }
        else
          count = 0;
        *m = '\0';
      }

      del_exemptid (channel_p, exempt->exemptstr);
    }
    exempt = bnext;
  }

  if (*parabuf)
  {
    sendto_channel_butserv (channel_p, from, ":%s MODE %s %s %s",
                            from->name, channel_p->chname, modebuf, parabuf);
    sendto_tsmode_servs (0, channel_p, from, ":%s MODE %s %s %s",
                         from->name, channel_p->chname, modebuf, parabuf);
    sendto_tsmode_servs (1, channel_p, from, ":%s MODE %s %ld %s %s",
                         from->name, channel_p->chname,
                         channel_p->channelts, modebuf, parabuf);
  }

  return;
}

/*
 * adds a user to a channel by adding another link to the channels
 * member chain.
 */
static void
add_user_to_channel (aChannel * channel_p, aClient * who, int flags)
{
  Link *ptr;
  chanMember *cm;

#ifdef DUMP_DEBUG
  fprintf (dumpfp, "Add to channel %s: %p:%s\n", channel_p->chname, who,
           who->name);
#endif


  if (who->user)
  {
    cm = make_chanmember ();
    cm->flags = flags;
    cm->client_p = who;
    cm->next = channel_p->members;
    cm->bans = 0;
    channel_p->members = cm;
    channel_p->users++;

    ptr = make_link ();
    ptr->value.channel_p = channel_p;
    ptr->next = who->user->channel;
    who->user->channel = ptr;
    who->user->joined++;
  }
}

void
remove_user_from_channel (aClient * source_p, aChannel * channel_p)
{
  chanMember **curr, *tmp;
  Link **lcurr, *ltmp;

  for (curr = &channel_p->members; (tmp = *curr); curr = &tmp->next)
    if (tmp->client_p == source_p)
    {
      *curr = tmp->next;
      free_chanmember (tmp);
      break;
    }

  for (lcurr = &source_p->user->channel; (ltmp = *lcurr); lcurr = &ltmp->next)
    if (ltmp->value.channel_p == channel_p)
    {
      *lcurr = ltmp->next;
      free_link (ltmp);
      break;
    }
  source_p->user->joined--;
  sub1_from_channel (channel_p);
}

int
is_chan_admin (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_CHANADMIN);

  return 0;
}

#ifdef UNUSED
/* this doesn't seem to be used anywhere, so lets not use it 
** - Fish (23/8/03)
*/
int
is_deadmined (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_DEADMINED);

  return 0;
}
#endif

int
is_chan_op (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_CHANOP);

  return 0;
}

#ifdef UNUSED
/* this doesn't seem to be used anywhere, so lets not use it 
** - Fish (23/8/03)
*/

int
is_deopped (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_DEOPPED);

  return 0;
}
#endif
int
is_half_op (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_HALFOP);

  return 0;
}

#ifdef UNUSED
/* this doesn't seem to be used anywhere, so lets not use it 
** - Fish (23/8/03)
*/

int
is_dehalfopped (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_DEHALFOPPED);

  return 0;
}
#endif
int
has_voice (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;

  if (channel_p)
    if ((cm = find_user_member (channel_p->members, client_p)))
      return (cm->flags & CHFL_VOICE);

  return 0;
}

int
can_send (aClient * client_p, aChannel * channel_p, char *msg)
{
  chanMember *cm;
  int member;

  if (IsServer (client_p) || IsULine (client_p))
    return 0;

  member = (cm = find_user_member (channel_p->members, client_p)) ? 1 : 0;

  if (!member)
  {
    if (channel_p->mode.mode & MODE_MODERATED)
      return (MODE_MODERATED);
    if (channel_p->mode.mode & MODE_NOPRIVMSGS)
      return (MODE_NOPRIVMSGS);
    if ((channel_p->mode.mode & MODE_MODREG) && !IsRegNick (client_p))
      return (MODE_MODREG);
    if ((channel_p->mode.mode & MODE_NOCOLOR) && msg_has_colors (msg))
      return (MODE_NOCOLOR);
    if (MyClient (client_p) && is_banned (client_p, channel_p)
        && !is_exempted (client_p, channel_p))
      return (MODE_BAN);        /* channel is -n and user is not there; we need to bquiet them if we can */
  }
  else
  {
    if (channel_p->mode.mode & MODE_MODERATED
        && !(cm->
             flags & (CHFL_CHANADMIN | CHFL_CHANOP | CHFL_HALFOP |
                      CHFL_VOICE)))
      return (MODE_MODERATED);
    if ((channel_p->mode.mode & MODE_MODREG) && !IsRegNick (client_p) &&
        !(cm->
          flags & (CHFL_CHANADMIN | CHFL_CHANOP | CHFL_HALFOP | CHFL_VOICE)))
      return (MODE_MODREG);
    if ((channel_p->mode.mode & MODE_NOCOLOR) && msg_has_colors (msg))
      return (MODE_NOCOLOR);
    if (is_banned (client_p, channel_p)
        && !is_exempted (client_p, channel_p)
        && !(cm->
             flags & (CHFL_CHANADMIN | CHFL_CHANOP | CHFL_HALFOP |
                      CHFL_VOICE)) && !IsAnOper (client_p))
      return (MODE_BAN);
  }

  return 0;
}

/*
 * write the "simple" list of channel modes for channel channel_p onto
 * buffer mbuf with the parameters in pbuf.
 */
static void
channel_modes (aClient * client_p, char *mbuf, char *pbuf,
               aChannel * channel_p)
{
  *mbuf++ = '+';
  if (channel_p->mode.mode & MODE_SECRET)
    *mbuf++ = 's';
  else if (channel_p->mode.mode & MODE_PRIVATE)
    *mbuf++ = 'p';
  if (channel_p->mode.mode & MODE_MODERATED)
    *mbuf++ = 'm';
  if (channel_p->mode.mode & MODE_TOPICLIMIT)
    *mbuf++ = 't';
  if (channel_p->mode.mode & MODE_INVITEONLY)
    *mbuf++ = 'i';
  if (channel_p->mode.mode & MODE_NOPRIVMSGS)
    *mbuf++ = 'n';
  if (channel_p->mode.mode & MODE_REGISTERED)
    *mbuf++ = 'r';
  if (channel_p->mode.mode & MODE_REGONLY)
    *mbuf++ = 'R';
  if (channel_p->mode.mode & MODE_NOCOLOR)
    *mbuf++ = 'c';
  if (channel_p->mode.mode & MODE_OPERONLY)
    *mbuf++ = 'O';
  if (channel_p->mode.mode & MODE_NOINVITE)
    *mbuf++ = 'N';
  if (channel_p->mode.mode & MODE_NOKNOCK)
    *mbuf++ = 'K';
  if (channel_p->mode.mode & MODE_SECURE)
    *mbuf++ = 'S';
  if (channel_p->mode.mode & MODE_ADMINONLY)
    *mbuf++ = 'A';
  if (channel_p->mode.mode & MODE_MODREG)
    *mbuf++ = 'M';
  if (channel_p->mode.mode & MODE_NOQUITREASON)
    *mbuf++ = 'q';

  if (channel_p->mode.limit)
  {
    *mbuf++ = 'l';
    if (IsMember (client_p, channel_p) || IsServer (client_p)
        || IsULine (client_p))
    {
      if (*channel_p->mode.key)
        ircsprintf (pbuf, "%d ", channel_p->mode.limit);
      else
        ircsprintf (pbuf, "%d", channel_p->mode.limit);
    }
  }
  if (*channel_p->mode.key)
  {
    *mbuf++ = 'k';
    if (IsMember (client_p, channel_p) || IsServer (client_p)
        || IsULine (client_p))
    {
      strcat (pbuf, channel_p->mode.key);
    }
  }

  *mbuf++ = '\0';
  return;
}

static void
send_ban_list (aClient * client_p, aChannel * channel_p)
{
  aBan *bp;
  char *cp;
  int count = 0, mysend = 0;

  cp = modebuf + strlen (modebuf);

  if (*parabuf)                 /* mode +l or +k xx */
    count = 1;

  for (bp = channel_p->banlist; bp; bp = bp->next)
  {
    if (strlen (parabuf) + strlen (bp->banstr) + 20 < (size_t) MODEBUFLEN)
    {
      if (*parabuf)
        strcat (parabuf, " ");
      strcat (parabuf, bp->banstr);
      count++;
      *cp++ = 'b';
      *cp = '\0';
    }
    else if (*parabuf)
      mysend = 1;

    if (count == MAXTSMODEPARAMS)
      mysend = 1;

    if (mysend)
    {
      if (IsTSMODE (client_p))
      {
        sendto_one (client_p, ":%s MODE %s %ld %s %s", me.name,
                    channel_p->chname, channel_p->channelts, modebuf,
                    parabuf);
      }
      else
      {
        sendto_one (client_p, ":%s MODE %s %s %s", me.name,
                    channel_p->chname, modebuf, parabuf);
      }
      mysend = 0;
      *parabuf = '\0';
      cp = modebuf;
      *cp++ = '+';
      if (count != MAXTSMODEPARAMS)
      {
        strcpy (parabuf, bp->banstr);
        *cp++ = 'b';
        count = 1;
      }
      else
        count = 0;
      *cp = '\0';
    }
  }
}

static void
send_exempt_list (aClient * client_p, aChannel * channel_p)
{
  anExempt *bp;
  char *cp;
  int count = 0, mysend = 0;

  cp = modebuf + strlen (modebuf);

  if (*parabuf)                 /* mode +l or +k xx */
    count = 1;

  for (bp = channel_p->exemptlist; bp; bp = bp->next)
  {
    if (strlen (parabuf) + strlen (bp->exemptstr) + 20 < (size_t) MODEBUFLEN)
    {
      if (*parabuf)
        strcat (parabuf, " ");
      strcat (parabuf, bp->exemptstr);
      count++;
      *cp++ = 'e';
      *cp = '\0';
    }
    else if (*parabuf)
      mysend = 1;

    if (count == MAXTSMODEPARAMS)
      mysend = 1;

    if (mysend)
    {
      if (IsTSMODE (client_p))
      {
        sendto_one (client_p, ":%s MODE %s %ld %s %s", me.name,
                    channel_p->chname, channel_p->channelts, modebuf,
                    parabuf);
      }
      else
      {
        sendto_one (client_p, ":%s MODE %s %s %s", me.name,
                    channel_p->chname, modebuf, parabuf);
      }
      mysend = 0;
      *parabuf = '\0';
      cp = modebuf;
      *cp++ = '+';
      if (count != MAXTSMODEPARAMS)
      {
        strcpy (parabuf, bp->exemptstr);
        *cp++ = 'e';
        count = 1;
      }
      else
        count = 0;
      *cp = '\0';
    }
  }
}

/* send "client_p" a full list of the modes for channel channel_p. */
void
send_channel_modes (aClient * client_p, aChannel * channel_p)
{
  chanMember *l, *anop = NULL, *skip = NULL;
  int n = 0;
  char *t;

  if (*channel_p->chname != '#')
    return;

  *modebuf = *parabuf = '\0';
  channel_modes (client_p, modebuf, parabuf, channel_p);

  ircsprintf (buf, ":%s SJOIN %ld %s %s %s :", me.name,
              channel_p->channelts, channel_p->chname, modebuf, parabuf);

  t = buf + strlen (buf);
  for (l = channel_p->members; l; l = l->next)
    if (l->flags & (MODE_CHANADMIN | MODE_CHANOP | MODE_HALFOP))
    {
      anop = l;
      break;
    }
  /*
   * follow the channel, but doing anop first if it's defined *
   * -orabidoo
   */
  l = NULL;
  for (;;)
  {
    if (anop)
    {
      l = skip = anop;
      anop = NULL;
    }
    else
    {
      if (l == NULL || l == skip)
        l = channel_p->members;
      else
        l = l->next;
      if (l && l == skip)
        l = l->next;
      if (l == NULL)
        break;
    }

    if (l->flags & MODE_CHANADMIN)
    {
      *t++ = '!';
    }
    if (l->flags & MODE_HALFOP)
    {
      if (IsSSJoin5 (client_p))
      {
        *t++ = '%';
      }
    }
    if (l->flags & MODE_CHANOP)
      *t++ = '@';

    if (l->flags & MODE_VOICE)
      *t++ = '+';

    /*     if (modebuf[1] || *parabuf)
       sendto_one (client_p, ":%s MODE %s %s %s",
       me.name, channel_p->chname, modebuf, parabuf);
     */
    strcpy (t, l->client_p->name);
    t += strlen (t);
    *t++ = ' ';
    n++;
    if (t - buf > BUFSIZE - 80)
    {
      *t++ = '\0';
      if (t[-1] == ' ')
        t[-1] = '\0';
      sendto_one (client_p, "%s", buf);

      sprintf (buf, ":%s SJOIN %ld %s 0 :", me.name, channel_p->channelts,
               channel_p->chname);

      t = buf + strlen (buf);
      n = 0;
    }
  }

  if (n)
  {
    *t++ = '\0';
    if (t[-1] == ' ')
      t[-1] = '\0';
    sendto_one (client_p, "%s", buf);
  }
  *parabuf = '\0';
  *modebuf = '+';
  modebuf[1] = '\0';
  send_ban_list (client_p, channel_p);
  send_exempt_list (client_p, channel_p);
  if (modebuf[1] || *parabuf)
  {
    if (IsTSMODE (client_p))
    {
      sendto_one (client_p, ":%s MODE %s %ld %s %s",
                  me.name, channel_p->chname, channel_p->channelts,
                  modebuf, parabuf);
    }
    else
    {
      sendto_one (client_p, ":%s MODE %s %s %s",
                  me.name, channel_p->chname, modebuf, parabuf);
    }
  }
}


/*
** We use this to send channel modes +a and +h to non SSJOIN3, SSJOIN4 and SSJOIN5 servers - ShadowMaster
*/
void
send_channel_modes_pressjoin3 (aClient * client_p, aChannel * channel_p)
{
  chanMember *l, *anop = NULL, *skip = NULL;

  if (*channel_p->chname != '#')
    return;

  *parabuf = '\0';
  *modebuf = '+';
  modebuf[1] = '\0';


  for (l = channel_p->members; l; l = l->next)
    if (l->flags & (MODE_CHANADMIN | MODE_CHANOP | MODE_HALFOP))
    {
      anop = l;
      break;
    }
  /*
   * follow the channel, but doing anop first if it's defined *
   * -orabidoo
   */
  l = NULL;
  for (;;)
  {
    if (anop)
    {
      l = skip = anop;
      anop = NULL;
    }
    else
    {
      if (l == NULL || l == skip)
        l = channel_p->members;
      else
        l = l->next;
      if (l && l == skip)
        l = l->next;
      if (l == NULL)
        break;
    }



    if (l->flags & MODE_CHANADMIN)
    {
      strcat (modebuf, "a");
      strcat (parabuf, l->client_p->name);
      strcat (parabuf, " ");

    }
    if (l->flags & MODE_HALFOP)
    {

      strcat (modebuf, "h");
      strcat (parabuf, l->client_p->name);
      strcat (parabuf, " ");

    }

  }
  if (modebuf[1] || *parabuf)
  {
    if (IsTSMODE (client_p))
    {
      sendto_one (client_p, ":%s MODE %s %ld %s %s",
                  me.name, channel_p->chname, channel_p->channelts,
                  modebuf, parabuf);
    }
    else
    {
      sendto_one (client_p, ":%s MODE %s %s %s",
                  me.name, channel_p->chname, modebuf, parabuf);
    }
  }

}


/* m_fmode - Basically SAMODE with diff name - ShadowMaster
**
** parv[0] = sender
** parv[1] = channel
** parv[2] = modes
*/
int
m_fmode (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int sendts;
  aChannel *channel_p;

  if (check_registered (client_p))
  {
    return 0;
  }

  if (!IsPrivileged (client_p))
  {
    sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
    return 0;
  }

  if (parc < 3)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS), me.name, parv[0],
                "FMODE");
    return 0;
  }

  channel_p = find_channel (parv[1], NullChn);
  if (channel_p == NullChn)
  {
    return 0;
  }

  if (!check_channelname (source_p, (unsigned char *) parv[1]))
  {
    return 0;
  }

  sendts =
    set_mode (client_p, source_p, channel_p, 4, parc - 2, parv + 2, modebuf,
              parabuf);

  if (strlen (modebuf) > (size_t) 1)
  {
    sendto_channel_butserv (channel_p, source_p, ":%s MODE %s %s %s",
                            parv[0], channel_p->chname, modebuf, parabuf);

    sendto_tsmode_servs (1, channel_p, client_p,
                         ":%s MODE %s %ld %s %s",
                         parv[0], channel_p->chname, channel_p->channelts,
                         modebuf, parabuf);
    sendto_tsmode_servs (0, channel_p, client_p, ":%s MODE %s %s %s",
                         parv[0], channel_p->chname, modebuf, parabuf);

    if (MyClient (source_p))
    {
      sendto_serv_butone (NULL,
                          ":%s NETGLOBAL :\2[FMODE]\2 %s forced a modechange on %s (%s%s%s)",
                          me.name, source_p->name, channel_p->chname,
                          modebuf, (*parabuf != 0 ? " " : ""), parabuf);
      sendto_netglobal
        ("from %s: \2[FMODE]\2 %s forced a modechange on %s (%s%s%s)",
         me.name, source_p->name, channel_p->chname, modebuf,
         (*parabuf != 0 ? " " : ""), parabuf);

      sendto_allchannelops_butone (NULL, &me, channel_p,
                                   ":%s NOTICE !@%%%s :[To %s Staff] -- %s forced a modechange (%s%s%s)",
                                   me.name, channel_p->chname,
                                   channel_p->chname, source_p->name,
                                   modebuf, (*parabuf != 0 ? " " : ""),
                                   parabuf);

      ilog (LOGF_OPERACT, "[FMODE] %s forced a modechange on %s (%s%s%s)",
            source_p->name, channel_p->chname,
            modebuf, (*parabuf != 0 ? " " : ""), parabuf);
    }
  }
  return 0;
}

/*
 * m_mode
 *
 * parv[0] - sender
 * parv[1] - channel
 */
int
m_mode (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int mcount = 0, chanop = 0;
  aChannel *channel_p;
  int subparc = 2;


  /* Now, try to find the channel in question */
  if (parc > 1)
  {
    channel_p = find_channel (parv[1], NullChn);
    if (channel_p == NullChn)
    {
      return m_umode (client_p, source_p, parc, parv);
    }
  }
  else
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "MODE");
    return 0;
  }

  if (!check_channelname (source_p, (unsigned char *) parv[1]))
  {
    return 0;
  }

  /*
   * We are now operating with 4 different levels of channel access.
   * Channel Halfops, ChanOps and Admins. In addition we have the ULined clients - ShadowMaster
   */

  if (IsServer (source_p) && channel_p->channelts != 0)
  {
    chanop = 6;
  }
  else if (IsULine (source_p))
  {
    chanop = 5;
  }
  else if (IsAnOper (source_p) && !MyClient (source_p))
  {
    chanop = 4;
  }
  else if (is_chan_admin (source_p, channel_p))
  {
    chanop = 3;
  }
  else if (is_chan_op (source_p, channel_p))
  {
    chanop = 2;
  }
  else if (is_half_op (source_p, channel_p))
  {
    chanop = 1;
  }

  if (parc < 3)
  {
    *modebuf = *parabuf = '\0';
    modebuf[1] = '\0';
    channel_modes (source_p, modebuf, parabuf, channel_p);
    sendto_one (source_p, rpl_str (RPL_CHANNELMODEIS), me.name, parv[0],
                channel_p->chname, modebuf, parabuf);
    sendto_one (source_p, rpl_str (RPL_CREATIONTIME), me.name, parv[0],
                channel_p->chname, channel_p->channelts);
    return 0;
  }

  if (IsServer (client_p) && IsTSMODE (client_p) && MyIsDigit (parv[2][0]))
  {
    ts_val modets = atol (parv[2]);

    if (modets != 0 && (modets > channel_p->channelts))
    {
      /* ignore this mode! */
      return 0;
    }
    subparc++;
  }
  mcount =
    set_mode (client_p, source_p, channel_p, chanop, parc - subparc,
              parv + subparc, modebuf, parabuf);

  if (strlen (modebuf) > (size_t) 1)
  {
    switch (mcount)
    {
       case 0:
         break;
       case -1:
         if (MyClient (source_p))
         {
           sendto_one (source_p,
                       err_str (ERR_CHANOPRIVSNEEDED),
                       me.name, parv[0], channel_p->chname);
         }
         else
         {
           ircstp->is_fake++;
         }
         break;
       default:

         sendto_channel_butserv (channel_p, source_p,
                                 ":%s MODE %s %s %s", parv[0],
                                 channel_p->chname, modebuf, parabuf);
         sendto_tsmode_servs (1, channel_p, client_p,
                              ":%s MODE %s %ld %s %s",
                              parv[0], channel_p->chname,
                              channel_p->channelts, modebuf, parabuf);
         sendto_tsmode_servs (0, channel_p, client_p, ":%s MODE %s %s %s",
                              parv[0], channel_p->chname, modebuf, parabuf);
    }
  }
  return 0;
}

static int chan_modes[] = {
  MODE_PRIVATE, 'p',
  MODE_SECRET, 's',
  MODE_MODERATED, 'm',
  MODE_NOPRIVMSGS, 'n',
  MODE_TOPICLIMIT, 't',
  MODE_REGONLY, 'R',
  MODE_INVITEONLY, 'i',
  MODE_NOCOLOR, 'c',
  MODE_OPERONLY, 'O',
  MODE_EXEMPT, 'e',
  MODE_NOINVITE, 'N',
  MODE_NOKNOCK, 'K',
  MODE_SECURE, 'S',
  MODE_ADMINONLY, 'A',
  MODE_CHANOP, 'o',
  MODE_VOICE, 'v',
  MODE_HALFOP, 'h',
  MODE_CHANADMIN, 'a',
  MODE_MODREG, 'M',
  MODE_NOQUITREASON, 'q',
  0x0, 0x0
};

char cmodestring[128];

void
build_cmodestr (void)
{
  int *s;
  char *m;

  m = cmodestring;

  for (s = chan_modes; *s; s += 2)
  {
    *m++ = (char) (*(s + 1));
  }

  *m = '\0';
}


/* this function checks for modes beeing changed twice
   for the same target during one mode line
   it will always use the first change made - againaway */
/* mcount -1 will reset this */

int
check_modes (int mcount, char mode, char arg)
{
  int t;
  char dmodes[SM_MAXMODES];
  char dargs[SM_MAXMODES];

  if (mcount == -1)             /* we start clean up */
  {
    /* Servers can do 512 mode changes .. !? we dont expect to get
       them in here which may cause a desync */
    for (t = 0; t <= SM_MAXMODES; t++)
    {
      if (dmodes[t])
      {
        dmodes[t] = '\0';
        dargs[t] = '\0';
      }
    }
  }

  for (t = 0; t <= SM_MAXMODES; t++)
  {
    if ((dmodes[t] != '\0') && (dmodes[t] == mode) && (dargs[t] == arg))
    {
      return -1;
    }
  }

  dmodes[mcount] = mode;
  dargs[mcount] = arg;

  return 0;
}

/* the old set_mode was pissing me off with it's disgusting
 * hackery, so I rewrote it.  Hope this works. }:> --wd
 */
static int
set_mode (aClient * client_p, aClient * source_p, aChannel * channel_p,
          int level, int parc, char *parv[], char *mbuf, char *pbuf)
{
#define SM_ERR_NOPRIVS 0x0001   /* is not an op */
#define SM_ERR_MOREPARMS 0x0002 /* needs more parameters */
#define SM_ERR_RESTRICTED 0x0004        /* not allowed to op others or be op'd */
#define SM_ERR_NOTOPER    0x0008        /* not an irc op */
#define SM_ERR_NOTSSLCLIENT 0x0010      /* not an SSL Client */

/* this macro appends to pbuf */
#define ADD_PARA(p) pptr = p; if(pidx) pbuf[pidx++] = ' '; while(*pptr) pbuf[pidx++] = *pptr++;

  static int flags[] = {
    MODE_PRIVATE, 'p', MODE_SECRET, 's',
    MODE_MODERATED, 'm', MODE_NOPRIVMSGS, 'n',
    MODE_TOPICLIMIT, 't', MODE_REGONLY, 'R',
    MODE_INVITEONLY, 'i', MODE_NOCOLOR, 'c', MODE_OPERONLY, 'O',
    MODE_EXEMPT, 'e', MODE_NOINVITE, 'N',
    MODE_NOKNOCK, 'K', MODE_SECURE, 'S', MODE_ADMINONLY, 'A',
    MODE_MODREG, 'M', MODE_NOQUITREASON, 'q',
    0x0, 0x0
  };

  Link *lp;                     /* for walking lists */
  chanMember *cm;               /* for walking channel member lists */
  aBan *bp;                     /* for walking banlists */
  anExempt *ep;                 /* for walking exemptlists */
  char *modes = parv[0];        /* user's idea of mode changes */
  int args;                     /* counter for what argument we're on */
  int banlsent = 0;             /* Only list bans once in a command. */
  int keyset = 0;               /* Only allow one key change per command */
  int limitset = 0;             /* ONly allow one limit change per command */
  int exemptlsent = 0;          /* Only list exempts once in a command. */
  char change = '+';            /* by default we + things... */
  int errors = 0;               /* errors returned, set with bitflags so we only return them once */
  /* from remote servers, ungodly numbers of modes can be sent, but
   * from local users only SM_MAXMODES are allowed */
  int maxmodes =
    ((IsServer (source_p) || IsULine (source_p)) ? 512 : SM_MAXMODES);
  int nmodes = 0;               /* how many modes we've set so far */
  aClient *who = NULL;          /* who we're doing a mode for */
  int chasing = 0;
  int i = 0;
  char moreparmsstr[] = "MODE   ";
  char nuhbuf[NICKLEN + USERLEN + HOSTLEN + 6]; /* for bans */
  char tmp[16];                 /* temporary buffer */
  int pidx = 0;                 /* index into pbuf */
  char *pptr;                   /* temporary paramater pointer */
  char *morig = mbuf;           /* beginning of mbuf */
  /* :client_p-name MODE channel_p->chname [MBUF] [PBUF] (buflen - 3 max and NULL) */
  /* added another 11 bytes to this, for TSMODE -epi */
  int prelen = strlen (client_p->name) + strlen (channel_p->chname) + 27;

  args = 1;

  if (parc < 1)
    return 0;

  check_modes (-1, '\0', '\0'); /* lets start with a clear one */

  *mbuf++ = '+';                /* add the plus, even if they don't */
  /* go through once to clean the user's mode string so we can
   * have a simple parser run through it...*/

  while (*modes && (nmodes < maxmodes))
  {
    switch (*modes)
    {
       case '+':
         if (*(mbuf - 1) == '-')
         {
           *(mbuf - 1) = '+';   /* change it around now */
           change = '+';
           break;
         }
         else if (change == '+')        /* we're still doing a +, we don't care */
         {
           break;
         }
         change = *modes;
         *mbuf++ = '+';
         break;

       case '-':
         if (*(mbuf - 1) == '+')
         {
           *(mbuf - 1) = '-';   /* change it around now */
           change = '-';
           break;
         }
         else if (change == '-')
         {
           break;               /* we're still doing a -, we don't care */
         }
         change = *modes;
         *mbuf++ = '-';
         break;

         /*case 'q': */
                /*** FIXME ***/
         /* Just a temporary cludge to disable +q until
          * i can hunt down a remaining bug which wont happen for b1
          */
/*	  break;*/
       case 'O':
         if (level < 4 && (level < 2 || !IsOper (source_p)))
         {
           errors |= SM_ERR_NOTOPER;
           break;
         }
         else
         {
           if (level < 5 && check_modes (nmodes, *modes, '\0') == -1)
           {
             break;
           }
           if (change == '+')
           {
             if (channel_p->mode.mode & MODE_OPERONLY && level < 5)
             {
               break;
             }
             else
             {
               channel_p->mode.mode |= MODE_OPERONLY;
             }
           }
           else
           {
             if (!(channel_p->mode.mode & MODE_OPERONLY) && level < 5)
             {
               break;
             }
             else
             {
               channel_p->mode.mode &= ~MODE_OPERONLY;
             }
           }
           *mbuf++ = *modes;
           nmodes++;
         }
         break;
       case 'A':
         if (level < 4 && (level < 2 || !IsSkoAdmin (source_p)))
         {
           errors |= SM_ERR_NOTOPER;
           break;
         }
         else
         {
           if (level < 5 && check_modes (nmodes, *modes, '\0') == -1)
           {
             break;
           }
           if (change == '+')
           {
             if (channel_p->mode.mode & MODE_ADMINONLY && level < 5)
             {
               break;
             }
             else
             {
               channel_p->mode.mode |= MODE_ADMINONLY;
             }
           }
           else
           {
             if (!(channel_p->mode.mode & MODE_ADMINONLY) && level < 5)
             {
               break;
             }
             else
             {
               channel_p->mode.mode &= ~MODE_ADMINONLY;
             }
           }
           *mbuf++ = *modes;
           nmodes++;
         }
         break;
       case 'S':
         if (!IsSSLClient (source_p) && level < 5)
         {
           errors |= SM_ERR_NOTSSLCLIENT;
           break;
         }
         if (level < 1)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         else
         {
           if (level < 5 && check_modes (nmodes, *modes, '\0') == -1)
           {
             break;
           }
           if (change == '+')
           {
             if (channel_p->mode.mode & MODE_SECURE && level < 5)
             {
               break;
             }
             else
             {
               channel_p->mode.mode |= MODE_SECURE;
             }
           }
           else
           {
             if (!(channel_p->mode.mode & MODE_SECURE) && level < 5)
             {
               break;
             }
             else
             {
               channel_p->mode.mode &= ~MODE_SECURE;
             }
           }
           *mbuf++ = *modes;
           nmodes++;
         }
         break;

       case 'a':
         if (level < 3)
         {
           if (parv[args] != NULL)
           {
             /* swallow the arg */
             args++;
           }
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         if (parv[args] == NULL)
         {
           /* silently drop the spare +a's */
           break;
         }

         who = find_chasing (source_p, parv[args], &chasing);
         cm = find_user_member (channel_p->members, who);
         if (cm == NULL)
         {
           sendto_one (source_p, err_str (ERR_USERNOTINCHANNEL),
                       me.name, client_p->name, parv[args],
                       channel_p->chname);
           /* swallow the arg */
           args++;
           break;
         }
         if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
         {
           args++;
           break;
         }
         /* to prevent mode flood - againaway */
         if (is_chan_admin (who, channel_p)
             && (change == '+' && *modes == 'a') && level < 6)
         {
           args++;
           break;
         }
         if (!is_chan_admin (who, channel_p)
             && (change == '-' && *modes == 'a') && level < 6)
         {
           args++;
           break;
         }
         /* to prevent mode fights with Service Clients - againaway */
         if (is_chan_admin (who, channel_p) && IsServicesClient (who)
             && (change == '-' && *modes == 'a') && level < 6)
         {
           sendto_one (source_p,
                       ":%s NOTICE %s :You cannot deadmin %s (Services Client)",
                       me.name, parv[0], who->name);
           args++;
           break;
         }

#ifdef LITTLE_I_LINE
         if (IsRestricted (source_p) && (change == '+' && *modes == 'a'))
         {
           errors |= SM_ERR_RESTRICTED;
           args++;
           break;
         }
#endif
         /* if we're going to overflow our mode buffer,
          * drop the change instead */
         if ((prelen + (mbuf - morig) + pidx + NICKLEN + 1) > REALMODEBUFLEN)
         {
           args++;
           break;
         }
#ifdef LITTLE_I_LINE
         if (MyClient (who) && IsRestricted (who)
             && (change == '+' && *modes == 'a'))
         {
           /* pass back to client_p a MODE -a to avoid desynch */
           sendto_one (client_p, ":%s MODE %s -a %s", me.name,
                       channel_p->chname, who->name);
           sendto_one (who,
                       ":%s NOTICE %s :*** Notice -- %s attempted to chanadmin you. You are restricted and cannot be chanadmined",
                       me.name, who->name, source_p->name);
           sendto_one (source_p,
                       ":%s NOTICE %s :*** Notice -- %s is restricted and cannot be chanadmined",
                       me.name, source_p->name, who->name);
           args++;
           break;
         }
#endif

         /* if we have the user, set them +/-[a] */
         if (change == '+')
         {
           cm->flags |= (CHFL_CHANADMIN);
         }
         else
         {
           cm->flags &= ~(CHFL_CHANADMIN);
         }

         /* we've decided their mode was okay, cool */
         *mbuf++ = *modes;
         ADD_PARA (cm->client_p->name);
         args++;
         nmodes++;
         if (IsServer (source_p) && *modes == 'a' && change == '+')
         {
           channel_p->channelts = 0;
           sendto_ops ("Server %s setting +a and blasting TS on %s",
                       source_p->name, channel_p->chname);
         }
         break;

       case 'o':
         if (level < 2)
         {
           if (parv[args] != NULL)
           {
             /* swallow the arg */
             args++;
           }
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         if (parv[args] == NULL)
         {
           /* silently drop the spare +o's */
           break;
         }

         who = find_chasing (source_p, parv[args], &chasing);
         cm = find_user_member (channel_p->members, who);
         if (cm == NULL)
         {
           sendto_one (source_p, err_str (ERR_USERNOTINCHANNEL),
                       me.name, client_p->name, parv[args],
                       channel_p->chname);
           /* swallow the arg */
           args++;
           break;
         }
         if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
         {
           args++;
           break;
         }
         /* to prevent mode flood - againaway */
         if (is_chan_op (who, channel_p) && (change == '+' && *modes == 'o')
             && level < 6)
         {
           args++;
           break;
         }
         if (!is_chan_op (who, channel_p) && (change == '-' && *modes == 'o')
             && level < 6)
         {
           args++;
           break;
         }
         /* to prevent mode fights with Service Clients - againaway */
         if (is_chan_op (who, channel_p) && IsServicesClient (who)
             && (change == '-' && *modes == 'o') && level < 6)
         {
           sendto_one (source_p,
                       ":%s NOTICE %s :You cannot deop %s (Services Client)",
                       me.name, parv[0], who->name);
           args++;
           break;
         }

#ifdef LITTLE_I_LINE
         if (IsRestricted (source_p) && (change == '+' && *modes == 'o'))
         {
           errors |= SM_ERR_RESTRICTED;
           args++;
           break;
         }
#endif
         /* if we're going to overflow our mode buffer,
          * drop the change instead */
         if ((prelen + (mbuf - morig) + pidx + NICKLEN + 1) > REALMODEBUFLEN)
         {
           args++;
           break;
         }
#ifdef LITTLE_I_LINE
         if (MyClient (who) && IsRestricted (who)
             && (change == '+' && *modes == 'o'))
         {
           /* pass back to client_p a MODE -o to avoid desynch */
           sendto_one (client_p, ":%s MODE %s -o %s", me.name,
                       channel_p->chname, who->name);
           sendto_one (who,
                       ":%s NOTICE %s :*** Notice -- %s attempted to chanop you. You are restricted and cannot be chanopped",
                       me.name, who->name, source_p->name);
           sendto_one (source_p,
                       ":%s NOTICE %s :*** Notice -- %s is restricted and cannot be chanopped",
                       me.name, source_p->name, who->name);
           args++;
           break;
         }
#endif

         /* if we have the user, set them +/-[o] */
         if (change == '+')
         {
           cm->flags |= (CHFL_CHANOP);
         }
         else
         {
           cm->flags &= ~(CHFL_CHANOP);
         }

         /* we've decided their mode was okay, cool */
         *mbuf++ = *modes;
         ADD_PARA (cm->client_p->name);
         args++;
         nmodes++;
         if (IsServer (source_p) && *modes == 'o' && change == '+')
         {
           channel_p->channelts = 0;
           sendto_ops ("Server %s setting +o and blasting TS on %s",
                       source_p->name, channel_p->chname);
         }
         break;

       case 'h':
         /*
          * Only Chanops (2)  and above may give halfops
          */
         if (level < 2 && change == '+')
         {
           if (parv[args] != NULL)
           {
             /* swallow the arg */
             args++;
           }
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         /*
          * Non halfoped users may not dehalfop
          */
         if (level < 1 && change == '-')
         {
           if (parv[args] != NULL)
           {
             /* swallow the arg */
             args++;
           }
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         /*
          * If theres no args, quietly drop the spare +h
          */
         if (parv[args] == NULL)
         {
           break;
         }

         who = find_chasing (source_p, parv[args], &chasing);

         /*
          * If the dehalfop is not for the user issuing the dehalfop, and the user it not
          * at least a Chanop (2), then we reject.
          */
         if (change == '-' && who != source_p && level < 2)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         cm = find_user_member (channel_p->members, who);
         if (cm == NULL)
         {
           sendto_one (source_p, err_str (ERR_USERNOTINCHANNEL),
                       me.name, client_p->name, parv[args],
                       channel_p->chname);
           /* swallow the arg */
           args++;
           break;
         }
         if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
         {
           args++;
           break;
         }
         /* to prevent mode flood - againaway */
         if (is_half_op (who, channel_p) && (change == '+' && *modes == 'h')
             && level < 6)
         {
           args++;
           break;
         }
         if (!is_half_op (who, channel_p) && (change == '-' && *modes == 'h')
             && level < 6)
         {
           args++;
           break;
         }
         /* to prevent mode fights with Service Clients - againaway */
         if (is_half_op (who, channel_p) && IsServicesClient (who)
             && (change == '-' && *modes == 'h') && level < 6)
         {
           sendto_one (source_p,
                       ":%s NOTICE %s :You cannot dehalfop %s (Services Client)",
                       me.name, parv[0], who->name);
           args++;
           break;
         }
#ifdef LITTLE_I_LINE
         if (IsRestricted (source_p) && (change == '+' && *modes == 'h'))
         {
           errors |= SM_ERR_RESTRICTED;
           args++;
           break;
         }
#endif

         /* if we're going to overflow our mode buffer,
          * drop the change instead */
         if ((prelen + (mbuf - morig) + pidx + NICKLEN + 1) > REALMODEBUFLEN)
         {
           args++;
           break;
         }
#ifdef LITTLE_I_LINE
         if (MyClient (who) && IsRestricted (who)
             && (change == '+' && *modes == 'h'))
         {
           /* pass back to client_p a MODE -h to avoid desynch */
           sendto_one (client_p, ":%s MODE %s -h %s", me.name,
                       channel_p->chname, who->name);
           sendto_one (who,
                       ":%s NOTICE %s :*** Notice -- %s attempted to halfop you. You are restricted and cannot be halfopped",
                       me.name, who->name, source_p->name);
           sendto_one (source_p,
                       ":%s NOTICE %s :*** Notice -- %s is restricted and cannot be halfopped",
                       me.name, source_p->name, who->name);
           args++;
           break;
         }
#endif

         /* if we have the user, set them +/-[h] */
         if (change == '+')
         {
           cm->flags |= (CHFL_HALFOP);
         }
         else
         {
           cm->flags &= ~(CHFL_HALFOP);
         }

         /* we've decided their mode was okay, cool */
         *mbuf++ = *modes;
         ADD_PARA (cm->client_p->name);
         args++;
         nmodes++;
         if (IsServer (source_p) && *modes == 'h' && change == '+')
         {
           channel_p->channelts = 0;
           sendto_ops ("Server %s setting +h and blasting TS on %s",
                       source_p->name, channel_p->chname);
         }
         break;

       case 'v':
         /*
          * Only Halfops (1)  and above may give/remove voice
          */
         if (level < 1)
         {
           if (parv[args] != NULL)
           {
             /* swallow the arg */
             args++;
           }
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         /*
          * If theres no args, quietly drop the spare +v
          */
         if (parv[args] == NULL)
         {
           break;
         }

         who = find_chasing (source_p, parv[args], &chasing);
         cm = find_user_member (channel_p->members, who);
         if (cm == NULL)
         {
           sendto_one (source_p, err_str (ERR_USERNOTINCHANNEL),
                       me.name, client_p->name, parv[args],
                       channel_p->chname);
           /* swallow the arg */
           args++;
           break;
         }
         if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
         {
           args++;
           break;
         }
         /* to prevent mode flood - againaway */
         if (has_voice (who, channel_p) && (change == '+' && *modes == 'v')
             && level < 6)
         {
           args++;
           break;
         }
         if (!has_voice (who, channel_p) && (change == '-' && *modes == 'v')
             && level < 6)
         {
           args++;
           break;
         }
         /* to prevent mode fights with Service Clients - againaway */
         if (has_voice (who, channel_p) && IsServicesClient (who)
             && (change == '-' && *modes == 'v') && level < 6)
         {
           sendto_one (source_p,
                       ":%s NOTICE %s :You cannot devoice %s (Services Client)",
                       me.name, parv[0], who->name);
           args++;
           break;
         }

         /* if we're going to overflow our mode buffer,
          * drop the change instead */
         if ((prelen + (mbuf - morig) + pidx + NICKLEN + 1) > REALMODEBUFLEN)
         {
           args++;
           break;
         }

         /* if we have the user, set them +/-[v] */
         if (change == '+')
         {
           cm->flags |= (CHFL_VOICE);
         }
         else
         {
           cm->flags &= ~(CHFL_VOICE);
         }

         /* we've decided their mode was okay, cool */
         *mbuf++ = *modes;
         ADD_PARA (cm->client_p->name);
         args++;
         nmodes++;
         break;

       case 'b':
         /* if the user has no more arguments, then they just want
          * to see the bans, okay, cool. */
         if (parv[args] == NULL)
         {
           if (level < 1 && !IsAnOper (source_p))
           {
             sendto_one (client_p, rpl_str (RPL_ENDOFBANLIST), me.name,
                         client_p->name, channel_p->chname);
             break;
           }
           if (banlsent || IsServer (source_p))
           {
             break;             /* Send only once */
           }
           for (bp = channel_p->banlist; bp; bp = bp->next)
           {
             sendto_one (source_p, rpl_str (RPL_BANLIST), me.name,
                         client_p->name, channel_p->chname, bp->banstr,
                         bp->who, bp->when);
           }
           sendto_one (client_p, rpl_str (RPL_ENDOFBANLIST), me.name,
                       client_p->name, channel_p->chname);
           banlsent = 1;
           break;               /* we don't pass this along, either.. */
         }
         else
         {
           if (level < 1)
           {
             errors |= SM_ERR_NOPRIVS;
             break;
           }

           /* do not allow : in bans, or a null ban */
           if (*parv[args] == ':' || *parv[args] == '\0')
           {
             args++;
             break;
           }

           /* make a 'pretty' ban mask here, then try and set it */
           /* okay kids, let's do this again.
            * the buffer returned by pretty_mask is from
            * make_nick_user_host. This buffer is eaten by add/del banid.
            * Thus, some poor schmuck gets himself on the banlist. Fixed. - lucas */
           strcpy (nuhbuf, collapse (pretty_mask (parv[args])));
           parv[args] = nuhbuf;
           /* if we're going to overflow our mode buffer,
            * drop the change instead */
           if ((prelen + (mbuf - morig) + pidx + strlen (nuhbuf) + 1) >
               REALMODEBUFLEN)
           {
             args++;
             break;
           }
           /* if we can't add or delete (depending) the ban, change is
            * worthless anyhow */

           if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
           {
             args++;
             break;
           }
           if (!
               (change == '+'
                && !add_banid (source_p, channel_p, parv[args]))
               && !(change == '-' && !del_banid (channel_p, parv[args])))
           {
             args++;
             break;
           }

           *mbuf++ = 'b';
           ADD_PARA (parv[args]);
           args++;
           nmodes++;
         }
         break;

       case 'e':
         /* if the user has no more arguments, then they just want
          * to see the exempts, okay, cool. */
         if (parv[args] == NULL)
         {
           if (level < 1 && !IsAnOper (source_p))
           {
             errors |= SM_ERR_NOPRIVS;
             break;
           }
           if (exemptlsent)
           {
             break;             /* Send only once */
           }
           for (ep = channel_p->exemptlist; ep; ep = ep->next)
           {
             sendto_one (source_p, rpl_str (RPL_EXEMPTLIST), me.name,
                         client_p->name, channel_p->chname,
                         ep->exemptstr, ep->who, ep->when);
           }
           sendto_one (client_p, rpl_str (RPL_ENDOFEXEMPTLIST), me.name,
                       client_p->name, channel_p->chname);
           exemptlsent = 1;
           break;               /* we don't pass this along, either.. */
         }
         else
         {
           if (level < 1)
           {
             errors |= SM_ERR_NOPRIVS;
             break;
           }

           /* do not allow : in exempts, or a null exempt */
           if (*parv[args] == ':' || *parv[args] == '\0')
           {
             args++;
             break;
           }

           /* make a 'pretty' exempt mask here, then try and set it */
           /* okay kids, let's do this again.
            * the buffer returned by pretty_mask is from
            * make_nick_user_host. This buffer is eaten by add/del exemptid.
            * Thus, some poor schmuck gets himself on the exemptlist. Fixed. - lucas */
           strcpy (nuhbuf, collapse (pretty_mask (parv[args])));
           parv[args] = nuhbuf;
           /* if we're going to overflow our mode buffer,
            * drop the change instead */
           if ((prelen + (mbuf - morig) + pidx + strlen (nuhbuf) + 1) >
               REALMODEBUFLEN)
           {
             args++;
             break;
           }
           /* if we can't add or delete (depending) the ban, change is
            * worthless anyhow */

           if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
           {
             args++;
             break;
           }
           if (!
               (change == '+'
                && !add_exemptid (source_p, channel_p, parv[args]))
               && !(change == '-' && !del_exemptid (channel_p, parv[args])))
           {
             args++;
             break;
           }

           *mbuf++ = 'e';
           ADD_PARA (parv[args]);
           args++;
           nmodes++;
         }
         break;

       case 'l':
         if (level < 2)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         /* We only allow one limit modechange per command */
         if (limitset)
         {
           break;
         }

         /* if it's a -, just change the flag, we have no arguments */
         if (change == '-')
         {
           if ((prelen + (mbuf - morig) + pidx + 1) > REALMODEBUFLEN)
           {
             break;
           }

           if (!(channel_p->mode.mode & MODE_LIMIT))
           {
             break;
           }
           else
           {
             *mbuf++ = 'l';
             channel_p->mode.mode &= ~MODE_LIMIT;
             channel_p->mode.limit = 0;
             nmodes++;
             limitset = 1;
             break;
           }
         }
         else
         {
           if (parv[args] == NULL)
           {
             errors |= SM_ERR_MOREPARMS;
             break;
           }

           /* if we're going to overflow our mode buffer,
            * drop the change instead */
           if ((prelen + (mbuf - morig) + pidx + 16) > REALMODEBUFLEN)
           {
             args++;
             break;
           }

           i = atoi (parv[args]);

           /* toss out invalid modes */
           if (i < 1)
           {
             args++;
             break;
           }
           if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
           {
             args++;
             break;
           }
           ircsprintf (tmp, "%d", i);
           {
             if (channel_p->mode.limit == i)
             {
               args++;
               break;
             }
             else
             {
               channel_p->mode.limit = i;
             }
           }
           channel_p->mode.mode |= MODE_LIMIT;
           *mbuf++ = 'l';
           ADD_PARA (tmp);
           args++;
           nmodes++;
           limitset = 1;
           break;
         }

       case 'k':
         if (level < 2)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         /* We only allow one key change per command */
         if (keyset)
         {
           break;
         }

         if (parv[args] == NULL)
         {
           errors |= SM_ERR_MOREPARMS;
           break;
         }

         /* do not allow keys to start with :! ack! - lucas */
         /* another ack: don't let people set null keys! */
         /* and yet a third ack: no spaces in keys -epi  */
         if (*parv[args] == ':' || *parv[args] == '\0'
             || strchr (parv[args], ' '))
         {
           args++;
           break;
         }

         /*
          * Do not let *'s in keys in preperation for key hiding - Raist
          *  Also take out ",", which makes a channel unjoinable - lucas
          */

         if (strchr (parv[args], '*') != NULL
             || strchr (parv[args], ',') != NULL)
         {
           args++;
           break;
         }

         /* if we're going to overflow our mode buffer,
          * drop the change instead */
         if ((prelen + (mbuf - morig) + pidx + KEYLEN + 2) > REALMODEBUFLEN)
         {
           args++;
           break;
         }

         /* if they're an op, they can futz with the key in
          * any manner they like, we're not picky */
         if (level < 5 && check_modes (nmodes, *modes, *parv[args]) == -1)
         {
           args++;
           break;
         }
         if (change == '+')
         {
           if (irccmp (channel_p->mode.key, parv[args]) == 0)
           {
             break;
           }
           else
           {
             strncpy (channel_p->mode.key, parv[args], KEYLEN);
             ADD_PARA (channel_p->mode.key);
           }
         }
         else
         {
           if (!*channel_p->mode.key)
           {
             break;
           }
           else
           {
             ADD_PARA (channel_p->mode.key);
             *channel_p->mode.key = '\0';
           }
         }
         *mbuf++ = 'k';
         args++;
         nmodes++;
         keyset = 1;
         break;

       case 'r':
         if (level < 5)
         {
           sendto_one (source_p, err_str (ERR_ONLYSERVERSCANCHANGE),
                       me.name, client_p->name, channel_p->chname);
           break;
         }
         else
         {
           if ((prelen + (mbuf - morig) + pidx + 1) > REALMODEBUFLEN)
           {
             break;
           }

           if (change == '+')
           {
             if (channel_p->mode.mode & MODE_REGISTERED)
             {
               break;
             }
             else
             {
               channel_p->mode.mode |= MODE_REGISTERED;
             }
           }
           else
           {
             if (!(channel_p->mode.mode & MODE_REGISTERED))
             {
               break;
             }
             else
             {
               channel_p->mode.mode &= ~MODE_REGISTERED;
             }
           }
         }
         *mbuf++ = 'r';
         nmodes++;
         break;

       case 's':
         if (level < 1)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         if (level < 5 && check_modes (nmodes, 's', '\0') == -1)
         {
           break;
         }
         if (change == '+')
         {
           if (channel_p->mode.mode & MODE_SECRET)
           {
             break;
           }

           if (channel_p->mode.mode & MODE_PRIVATE)
           {
             if (level < 5 && check_modes (nmodes + 1, 'p', '\0') == -1)
             {
               break;
             }
             if (!(*(mbuf - 1) == ('+')))
             {
               *mbuf++ = '+';
             }
             *mbuf++ = *modes;
             nmodes++;
             channel_p->mode.mode &= ~MODE_PRIVATE;
             *mbuf++ = '-';
             *mbuf++ = 'p';
             channel_p->mode.mode |= MODE_SECRET;
             nmodes++;
             break;
           }
           channel_p->mode.mode |= MODE_SECRET;
         }
         else
         {
           if (!(channel_p->mode.mode & MODE_SECRET))
           {
             break;
           }
           else
           {
             channel_p->mode.mode &= ~MODE_SECRET;
           }
         }
         *mbuf++ = *modes;
         nmodes++;
         break;

       case 'p':
         if (level < 1)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         if (level < 5 && check_modes (nmodes, 'p', '\0') == -1)
         {
           break;
         }
         if (change == '+')
         {
           if (channel_p->mode.mode & MODE_PRIVATE)
           {
             break;
           }

           if (channel_p->mode.mode & MODE_SECRET)
           {
             if (level < 5 && check_modes (nmodes + 1, 's', '\0') == -1)
             {
               break;
             }
             if (!(*(mbuf - 1) == ('+')))
             {
               *mbuf++ = '+';
             }
             *mbuf++ = *modes;
             nmodes++;
             channel_p->mode.mode &= ~MODE_SECRET;
             *mbuf++ = '-';
             *mbuf++ = 's';
             channel_p->mode.mode |= MODE_PRIVATE;
             nmodes++;
             break;
           }
           channel_p->mode.mode |= MODE_PRIVATE;
         }
         else
         {
           if (!(channel_p->mode.mode & MODE_PRIVATE))
           {
             break;
           }
           else
           {
             channel_p->mode.mode &= ~MODE_PRIVATE;
           }
         }
         *mbuf++ = *modes;
         nmodes++;
         break;

       case 'i':
         if (level < 2)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         if (level < 5 && check_modes (nmodes, 'i', '\0') == -1)
         {
           break;
         }
         if (change == '+')
         {
           if (channel_p->mode.mode & MODE_INVITEONLY)
           {
             break;
           }

           if (channel_p->mode.mode & MODE_NOINVITE)
           {
             if (level < 5 && check_modes (nmodes + 1, 'N', '\0') == -1)
             {
               break;
             }
             if (!(*(mbuf - 1) == ('+')))
             {
               *mbuf++ = '+';
             }
             *mbuf++ = *modes;
             nmodes++;
             channel_p->mode.mode &= ~MODE_NOINVITE;
             *mbuf++ = '-';
             *mbuf++ = 'N';
             channel_p->mode.mode |= MODE_INVITEONLY;
             nmodes++;
             break;
           }
           else
             channel_p->mode.mode |= MODE_INVITEONLY;
         }
         else
         {
           if (!(channel_p->mode.mode & MODE_INVITEONLY))
           {
             break;
           }
           else
           {
             channel_p->mode.mode &= ~MODE_INVITEONLY;
           }
         }
         *mbuf++ = *modes;
         nmodes++;
         break;

       case 'N':
         if (level < 2)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }
         if (level < 5 && check_modes (nmodes, 'N', '\0') == -1)
         {
           break;
         }
         if (change == '+')
         {
           if (channel_p->mode.mode & MODE_NOINVITE)
           {
             break;
           }

           if (channel_p->mode.mode & MODE_INVITEONLY)
           {
             if (level < 5 && check_modes (nmodes + 1, 'i', '\0') == -1)
             {
               break;
             }
             if (!(*(mbuf - 1) == ('+')))
             {
               *mbuf++ = '+';
             }
             *mbuf++ = *modes;
             nmodes++;
             channel_p->mode.mode &= ~MODE_INVITEONLY;
             *mbuf++ = '-';
             *mbuf++ = 'i';
             channel_p->mode.mode |= MODE_NOINVITE;
             nmodes++;
             break;
           }
           else
             channel_p->mode.mode |= MODE_NOINVITE;
         }
         else
         {
           if (!(channel_p->mode.mode & MODE_NOINVITE))
           {
             break;
           }
           else
           {
             while ((lp = channel_p->invites))
               del_invite (lp->value.client_p, channel_p);
           }
           channel_p->mode.mode &= ~MODE_NOINVITE;
         }
         *mbuf++ = *modes;
         nmodes++;
         break;

       default:                /* fall through to default case */
         /* phew, no more tough modes. }:>, the rest are all covered in one step
          * with the above array */
         if (level < 1)
         {
           errors |= SM_ERR_NOPRIVS;
           break;
         }

         for (i = 1; flags[i] != 0x0; i += 2)
         {
           if ((prelen + (mbuf - morig) + pidx + 1) > REALMODEBUFLEN)
           {
             break;
           }

           if (*modes == (char) flags[i])
           {
             if (level < 5 && check_modes (nmodes, *modes, '\0') == -1)
             {
               nmodes++;
               break;
             }
             if (change == '+')
               /* Let's check if modes are already set .. if so ... break */
             {
               if (((flags[i - 1] == MODE_MODERATED)
                    && (channel_p->mode.mode & MODE_MODERATED))
                   || ((flags[i - 1] == MODE_NOPRIVMSGS)
                       && (channel_p->mode.mode & MODE_NOPRIVMSGS))
                   || ((flags[i - 1] == MODE_REGONLY)
                       && (channel_p->mode.mode & MODE_REGONLY))
                   || ((flags[i - 1] == MODE_NOCOLOR)
                       && (channel_p->mode.mode & MODE_NOCOLOR))
                   || ((flags[i - 1] == MODE_NOKNOCK)
                       && (channel_p->mode.mode & MODE_NOKNOCK))
                   || ((flags[i - 1] == MODE_TOPICLIMIT)
                       && (channel_p->mode.mode & MODE_TOPICLIMIT))
                   || ((flags[i - 1] == MODE_MODREG)
                       && (channel_p->mode.mode & MODE_MODREG))
                   || ((flags[i - 1] == MODE_NOQUITREASON)
                       && (channel_p->mode.mode & MODE_NOQUITREASON)))
                 break;
               else
               {
                 channel_p->mode.mode |= flags[i - 1];
               }
             }
             else
               /* Now we are checking it modes are already removed or not realy set .. if so ... break */
             {
               if (((flags[i - 1] == MODE_MODERATED)
                    && !(channel_p->mode.mode & MODE_MODERATED))
                   || ((flags[i - 1] == MODE_NOPRIVMSGS)
                       && !(channel_p->mode.mode & MODE_NOPRIVMSGS))
                   || ((flags[i - 1] == MODE_REGONLY)
                       && !(channel_p->mode.mode & MODE_REGONLY))
                   || ((flags[i - 1] == MODE_NOCOLOR)
                       && !(channel_p->mode.mode & MODE_NOCOLOR))
                   || ((flags[i - 1] == MODE_NOKNOCK)
                       && !(channel_p->mode.mode & MODE_NOKNOCK))
                   || ((flags[i - 1] == MODE_TOPICLIMIT)
                       && !(channel_p->mode.mode & MODE_TOPICLIMIT))
                   || ((flags[i - 1] == MODE_MODREG)
                       && !(channel_p->mode.mode & MODE_MODREG))
                   || ((flags[i - 1] == MODE_NOQUITREASON)
                       && !(channel_p->mode.mode & MODE_NOQUITREASON)))
               {
                 break;
               }
               else
               {
                 channel_p->mode.mode &= ~flags[i - 1];
               }
             }
             *mbuf++ = *modes;
             nmodes++;
             break;
           }
         }
         /* unknown mode.. */
         if (flags[i] == 0x0)
         {
           /* we still spew lots of unknown mode bits... */
           /* but only to our own clients, silently ignore bogosity
            * from other servers... */
           if (MyClient (source_p))
           {
             sendto_one (source_p, err_str (ERR_UNKNOWNMODE), me.name,
                         source_p->name, *modes);
           }
         }
         break;
    }

    /* spit out more parameters error here */
    if (errors & SM_ERR_MOREPARMS && MyClient (source_p))
    {
      moreparmsstr[5] = change;
      moreparmsstr[6] = *modes;
      sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS), me.name,
                  source_p->name, moreparmsstr);
      errors &= ~SM_ERR_MOREPARMS;      /* oops, kill it in this case */
    }
    modes++;
  }
  /* clean up the end of the string... */
  if (*(mbuf - 1) == '+' || *(mbuf - 1) == '-')
  {
    *(mbuf - 1) = '\0';
  }
  else
  {
    *mbuf = '\0';
  }
  pbuf[pidx] = '\0';
  if (MyClient (source_p))
  {
    if (errors & SM_ERR_NOPRIVS)
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED), me.name,
                  source_p->name, channel_p->chname);
    }
    if (errors & SM_ERR_NOTOPER)
    {
      sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name,
                  source_p->name);
    }
    if (errors & SM_ERR_RESTRICTED)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- You are restricted and cannot chanop others",
                  me.name, source_p->name);
    }
    if (errors & SM_ERR_NOTSSLCLIENT)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- You need to be a secured client (SSL) to set a channel secured clients only",
                  me.name, source_p->name);
    }
  }

  /* all done! */
  return nmodes;
#undef ADD_PARA
}


static int
can_join (aClient * source_p, aChannel * channel_p, char *key)
{
  Link *lp;
  int invited = 0;
  for (lp = source_p->user->invited; lp; lp = lp->next)
  {
    if (lp->value.channel_p == channel_p)
    {
      invited = 1;
      break;
    }
  }
  if (IsULine (source_p))
    return 0;

  if (is_banned (source_p, channel_p))
  {
    if (is_exempted (source_p, channel_p))
    {
      sendto_allchannelops_butone (NULL, &me, channel_p,
                                   ":%s NOTICE !@%%%s :[To %s Staff] -- %s matches an exempt line (overriding ban)",
                                   me.name, channel_p->chname,
                                   channel_p->chname, source_p->name);
    }
    else if (invited)
    {
      sendto_allchannelops_butone (NULL, &me, channel_p,
                                   ":%s NOTICE !@%%%s :[To %s Staff] -- %s carries an invitation (overriding ban)",
                                   me.name, channel_p->chname,
                                   channel_p->chname, source_p->name);
    }
    else
    {
      return (ERR_BANNEDFROMCHAN);
    }
  }
  if (channel_p->mode.mode & MODE_SECURE && !IsSSLClient (source_p))
  {
    if (invited)
    {
      sendto_channel_butone (NULL, &me, channel_p,
                             ":%s NOTICE %s :[To %s clients] -- %s carries an invitation and is NOT a secure client",
                             me.name, channel_p->chname,
                             channel_p->chname, source_p->name);
    }
    else
    {
      return (ERR_SSLCLIENTSONLY);
    }
  }
  if (channel_p->mode.mode & MODE_REGONLY && !IsRegNick (source_p))
  {
    if (invited)
    {
      sendto_channel_butone (NULL, &me, channel_p,
                             ":%s NOTICE %s :[To %s clients] -- %s carries an invitation and does NOT have a registered nickname",
                             me.name, channel_p->chname,
                             channel_p->chname, source_p->name);
    }
    else
    {
      return (ERR_NEEDREGGEDNICK);
    }
  }

  if (invited)
    return 0;
  if (channel_p->mode.mode & MODE_INVITEONLY)
  {
    return (ERR_INVITEONLYCHAN);
  }
  /*
     if (channel_p->mode.mode & MODE_REGONLY && !IsRegNick (source_p))
     {
     return (ERR_NEEDREGGEDNICK);
     }
   */
  /*
     if (channel_p->mode.mode & MODE_SECURE && !IsSSLClient (source_p))
     {
     return (ERR_SSLCLIENTSONLY);
     }
   */
  if (channel_p->mode.mode & MODE_ADMINONLY && !IsSkoAdmin (source_p))
  {
    return (ERR_NOPRIVILEGES);
  }
  if (channel_p->mode.mode & MODE_OPERONLY && !IsAnOper (source_p))
  {
    return (ERR_NOPRIVILEGES);
  }
  if (*channel_p->mode.key
      && (BadPtr (key) || (irccmp (channel_p->mode.key, key) != 0)))
  {
    return (ERR_BADCHANNELKEY);
  }
  if (channel_p->mode.limit && channel_p->users >= channel_p->mode.limit)
  {
    return (ERR_CHANNELISFULL);
  }

  return 0;
}

/*
 * * Remove bells and commas from channel name
 */

void
clean_channelname (unsigned char *cn)
{
  for (; *cn; cn++)
    /*
     * All characters >33 are allowed, except commas, and the weird
     * fake-space character mIRCers whine about -wd
     */
    if (*cn < 33 || *cn == ',' || (*cn == 160))
    {
      *cn = '\0';
      return;
    }
  return;
}

/* we also tell the client if the channel is invalid. */
int
check_channelname (aClient * client_p, unsigned char *cn)
{
  if (!MyClient (client_p))
    return 1;
  for (; *cn; cn++)
  {
    if (*cn < 33 || *cn == ',' || *cn == 160)
    {
      sendto_one (client_p, getreply (ERR_BADCHANNAME), me.name,
                  client_p->name, cn);
      return 0;
    }
  }
  return 1;
}

/*
 * *  Get Channel block for chname (and allocate a new channel *
 * block, if it didn't exist before).
 */
static aChannel *
get_channel (aClient * client_p, char *chname, int flag, int *created)
{
  aChannel *channel_p;
  int len;

  if (created)
    *created = 0;

  if (BadPtr (chname))
    return NULL;

  len = strlen (chname);
  if (MyClient (client_p) && len > CHANNELLEN)
  {
    len = CHANNELLEN;
    *(chname + CHANNELLEN) = '\0';
  }
  if ((channel_p = find_channel (chname, (aChannel *) NULL)))
    return (channel_p);
  if (flag == CREATE)
  {
    channel_p = make_channel ();

    if (created)
      *created = 1;

    strncpyzt (channel_p->chname, chname, len + 1);
    if (channel)
      channel->prevch = channel_p;
    channel_p->prevch = NULL;
    channel_p->nextch = channel;
    channel = channel_p;
    channel_p->channelts = timeofday;
    channel_p->mode.mode |= MODE_TOPICLIMIT;
    channel_p->mode.mode |= MODE_NOPRIVMSGS;
    (void) add_to_channel_hash_table (chname, channel_p);
    Count.chan++;
  }
  return channel_p;
}

static void
add_invite (aClient * client_p, aChannel * channel_p)
{
  Link *inv, **tmp;

  del_invite (client_p, channel_p);
  /*
   * delete last link in chain if the list is max length
   */
  if (list_length (client_p->user->invited) >= MAXCHANNELSPERUSER)
  {
    /*
     * This forgets the channel side of invitation     -Vesa inv =
     * client_p->user->invited; client_p->user->invited = inv->next;
     * free_link(inv);
     */
    del_invite (client_p, client_p->user->invited->value.channel_p);

  }
  /*
   * add client to channel invite list
   */
  inv = make_link ();
  inv->value.client_p = client_p;
  inv->next = channel_p->invites;
  channel_p->invites = inv;
  /*
   * add channel to the end of the client invite list
   */
  for (tmp = &(client_p->user->invited); *tmp; tmp = &((*tmp)->next));
  inv = make_link ();
  inv->value.channel_p = channel_p;
  inv->next = NULL;
  (*tmp) = inv;
}

/*
 * Delete Invite block from channel invite list and client invite list
 */
void
del_invite (aClient * client_p, aChannel * channel_p)
{
  Link **inv, *tmp;

  for (inv = &(channel_p->invites); (tmp = *inv); inv = &tmp->next)
    if (tmp->value.client_p == client_p)
    {
      *inv = tmp->next;
      free_link (tmp);
      break;
    }

  for (inv = &(client_p->user->invited); (tmp = *inv); inv = &tmp->next)
    if (tmp->value.channel_p == channel_p)
    {
      *inv = tmp->next;
      free_link (tmp);
      break;
    }
}

/*
 * Subtract one user from channel i
 * (and free channel block, if channel became empty).
 */
static void
sub1_from_channel (aChannel * channel_p)
{
  Link *tmp;
  aBan *bp, *bprem;
  anExempt *ep, *eprem;

  if (--channel_p->users <= 0)
  {
    /*
     * Now, find all invite links from channel structure
     */
    while ((tmp = channel_p->invites))
      del_invite (tmp->value.client_p, channel_p);

    bp = channel_p->banlist;
    while (bp)
    {
      bprem = bp;
      bp = bp->next;
      MyFree (bprem->banstr);
      MyFree (bprem->who);
      MyFree (bprem);
    }
    ep = channel_p->exemptlist;
    while (ep)
    {
      eprem = ep;
      ep = ep->next;
      MyFree (eprem->exemptstr);
      MyFree (eprem->who);
      MyFree (eprem);
    }

    if (channel_p->prevch)
      channel_p->prevch->nextch = channel_p->nextch;
    else
      channel = channel_p->nextch;
    if (channel_p->nextch)
      channel_p->nextch->prevch = channel_p->prevch;
    (void) del_from_channel_hash_table (channel_p->chname, channel_p);
#ifdef FLUD
    free_fluders (NULL, channel_p);
#endif
    free_channel (channel_p);
    Count.chan--;
  }
}

/*
 * m_join
 *
 * parv[0] = sender prefix
 * parv[1] = channel
 * parv[2] = channel password (key)
 */
int
m_join (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  static char jbuf[BUFSIZE];
  Link *lp;
  aConfItem *aconf = NULL;
  aChannel *channel_p;
  char *name, *key = NULL;
  int i, flags = 0, chanlen = 0;
  int allow_op = YES;
  char *p = NULL, *p2 = NULL;
  char *nick;

#ifdef ANTI_SPAMBOT
  int successful_join_count = 0;
  /* Number of channels successfully joined */
#endif

  if (!(source_p->user))
  {
    /* something is *fucked* - bail */
    return 0;
  }

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "JOIN");
    return 0;
  }

  *jbuf = '\0';
  /*
   * * Rebuild list of channels joined to be the actual result of the *
   * JOIN.  Note that "JOIN 0" is the destructive problem.
   */

#define CHANNEL_RESTRICT 1
  for (i = 0, name = strtoken (&p, parv[1], ","); name;
       name = strtoken (&p, (char *) NULL, ","))
  {
    /*
     * pathological case only on longest channel name. * If not dealt
     * with here, causes desynced channel ops * since ChannelExists()
     * doesn't see the same channel * as one being joined. cute bug.
     * Oct 11 1997, Dianora/comstud
     */
    if (!check_channelname (source_p, (unsigned char *) name))
    {
      continue;
    }

    chanlen = strlen (name);

    if (chanlen > CHANNELLEN)
    {                           /* same thing is done in get_channel() */
      name[CHANNELLEN] = '\0';
      chanlen = CHANNELLEN;
    }

    if (*name == '&' && !MyConnect (source_p))
    {
      continue;
    }

    if (*name == '0' && !atoi (name))
    {
      *jbuf = '\0';
    }
    else if (!IsChannelName (name))
    {
      if (MyClient (source_p))
      {
        sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                    me.name, parv[0], name);
      }
      continue;
    }

    if (CHANNEL_RESTRICT == 1)
    {
      if (crlines && (channel_canjoin (source_p, name) != 1))
      {
        sendto_one (source_p,
                    ":%s NOTICE %s :*** Notice -- Cannot join %s. (%s)",
                    me.name, source_p->name, name,
                    cannotjoin_msg ? cannotjoin_msg :
                    "Channel access restricted");
        sendto_one (source_p,
                    ":%s NOTICE %s :*** Notice -- Please use \2/SETTINGS CHANNELS\2 to see a list of all channels that can be joined.",
                    me.name, source_p->name);
        return 0;
      }
    }

    if (*jbuf)
    {
      strcat (jbuf, ",");
    }
    strncat (jbuf, name, sizeof (jbuf) - i - 1);
    i += chanlen + 1;
  }


  /*
   * (void)strcpy(parv[1], jbuf); 
   */

  p = NULL;
  if (parv[2])
  {
    key = strtoken (&p2, parv[2], ",");
  }

  parv[2] = NULL;               /*
                                 * for m_names call later, parv[parc]
                                 * * must == NULL 
                                 */
  for (name = strtoken (&p, jbuf, ","); name;
       key = (key) ? strtoken (&p2, NULL, ",") : NULL,
       name = strtoken (&p, NULL, ","))
  {
    /*
     * * JOIN 0 sends out a part for all channels a user * has
     * joined.
     */
    if (*name == '0' && !atoi (name))
    {
      if (source_p->user->channel == NULL)
      {
        continue;
      }

      while ((lp = source_p->user->channel))
      {
        channel_p = lp->value.channel_p;
        sendto_channel_butserv (channel_p, source_p, PartFmt,
                                parv[0], channel_p->chname);
        remove_user_from_channel (source_p, channel_p);
      }
      /*
       * Added /quote set for SPAMBOT
       * 
       * int spam_time = MIN_JOIN_LEAVE_TIME; int spam_num =
       * MAX_JOIN_LEAVE_COUNT;
       */
#ifdef ANTI_SPAMBOT             /*
                                 * Dianora 
                                 */

      if (MyConnect (source_p) && !IsAnOper (source_p))
      {
        if (source_p->join_leave_count >= spam_num)
        {
          sendto_realops_lev (SPAM_LEV,
                              "User %s (%s@%s) is a possible spambot",
                              source_p->name,
                              source_p->user->username, source_p->user->host);
          source_p->oper_warn_count_down = OPER_SPAM_COUNTDOWN;
        }
        else
        {
          int t_delta;

          if ((t_delta = (NOW - source_p->last_leave_time)) >
              JOIN_LEAVE_COUNT_EXPIRE_TIME)
          {
            int decrement_count;

            decrement_count = (t_delta / JOIN_LEAVE_COUNT_EXPIRE_TIME);

            if (decrement_count > source_p->join_leave_count)
            {
              source_p->join_leave_count = 0;
            }
            else
            {
              source_p->join_leave_count -= decrement_count;
            }
          }
          else
          {
            if ((NOW - (source_p->last_join_time)) < spam_time)
            {
              /*
               * oh, its a possible spambot 
               */
              source_p->join_leave_count++;
            }
          }
          source_p->last_leave_time = NOW;
        }
      }
#endif
      sendto_match_servs (NULL, client_p, ":%s JOIN 0", parv[0]);
      continue;
    }

    if (MyConnect (source_p))
    {
      if (!IsOper (source_p)
          && (aconf = find_conf_name (name, CONF_QUARANTINED_CHAN)))
      {
        sendto_one (source_p, getreply (ERR_CHANBANREASON), me.name,
                    parv[0], name,
                    BadPtr (aconf->
                            passwd) ? "Reserved channel" : aconf->passwd);
        continue;
      }

      /*
       * local client is first to enter previously nonexistent 
       * channel so make them (rightfully) the Channel Operator.
       */

      flags = (ChannelExists (name)) ? 0 : CHFL_CHANOP;
#ifdef NO_CHANOPS_WHEN_SPLIT
      if (!IsAnOper (source_p) && server_was_split
          && server_split_recovery_time)
      {
        if ((server_split_time + server_split_recovery_time) < NOW)
        {
          if (server_list != NULL)
          {
            server_was_split = NO;
          }
          else
          {
            server_split_time = NOW;    /*
                                         * still split 
                                         */
            allow_op = NO;
          }
        }
        else
        {
          allow_op = NO;
        }
      }
#endif

#ifdef LITTLE_I_LINES
      if (!IsAnOper (source_p) && IsRestricted (source_p))
      {
        allow_op = NO;
        sendto_one (source_p,
                    ":%s NOTICE %s :*** Notice -- You are restricted and cannot be chanopped",
                    me.name, source_p->name);
      }
#endif
      if ((source_p->user->joined >= MAXCHANNELSPERUSER) &&
          (!IsAnOper (source_p)
           || (source_p->user->joined >= MAXCHANNELSPERUSER * 3)))
      {
        sendto_one (source_p, err_str (ERR_TOOMANYCHANNELS),
                    me.name, parv[0], name);
#ifdef ANTI_SPAMBOT
        if (successful_join_count)
        {
          source_p->last_join_time = NOW;
        }
#endif
        return 0;
      }
#ifdef ANTI_SPAMBOT

      /*
       * if channel doesn't exist, don't penalize
       */
      if (flags == 0)
      {
        successful_join_count++;
      }
      if (source_p->join_leave_count >= spam_num)
      {
        /*
         * Its already known as a possible spambot
         */

        if (source_p->oper_warn_count_down > 0)
        {
          source_p->oper_warn_count_down--;
        }
        else
        {
          source_p->oper_warn_count_down = 0;
        }

        if (source_p->oper_warn_count_down == 0)
        {
          sendto_realops_lev (SPAM_LEV,
                              "User %s (%s@%s) trying to join %s is a possible spambot",
                              source_p->name,
                              source_p->user->username,
                              source_p->user->host, name);
          source_p->oper_warn_count_down = OPER_SPAM_COUNTDOWN;
        }
# ifndef ANTI_SPAMBOT_WARN_ONLY
        return 0;               /*
                                 * Don't actually JOIN anything, but
                                 * * don't let spambot know that
                                 */
# endif
      }
#endif
    }
    else
    {
      /*
       * * complain for remote JOINs to existing channels * (they
       * should be SJOINs) -orabidoo
       */
      if (!ChannelExists (name))
      {
        ts_warn ("User on %s remotely JOINing new channel",
                 source_p->user->server);
      }
    }

    channel_p = get_channel (source_p, name, CREATE, NULL);

    if (IsMember (source_p, channel_p))
    {
      continue;
    }

    if (!channel_p
        || (MyConnect (source_p)
            && (i = can_join (source_p, channel_p, key))))
    {
      sendto_one (source_p, err_str (i), me.name, parv[0], name);

#ifdef ANTI_SPAMBOT
      if (successful_join_count > 0)
      {
        successful_join_count--;
      }
#endif
      continue;
    }

/* only complain when the user can join the channel, the channel is being created by this user,
   and this user is not allowed to be an op. - lucas */

#ifdef NO_CHANOPS_WHEN_SPLIT
    if (flags && !allow_op)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- Due to a network split, you can not obtain channel operator status in a new channel at this time.",
                  me.name, source_p->name);
    }
#endif

    nick = source_p->name;

    /*
     * Complete user entry to the new channel (if any)
     */
    if (allow_op)
    {
      add_user_to_channel (channel_p, source_p, flags);
    }
    else
    {
      add_user_to_channel (channel_p, source_p, 0);
    }

    /*
     * Propogate channel creation SJOIN when a user should recieve ChanOP
     * if server is not split.
     */
    if (MyClient (source_p) && flags == CHFL_CHANOP)
    {
      channel_p->channelts = timeofday;

      if (allow_op)
      {

        sendto_ssjoin5_servs (channel_p, client_p,
                              ":%s SJOIN %ld %s +nt :@%s", me.name,
                              channel_p->channelts, name, parv[0]);

        sendto_pressjoin3_servs (channel_p, client_p,
                                 ":%s SJOIN %ld %s +nt :@%s", me.name,
                                 channel_p->channelts, name, parv[0]);
      }
      else
      {
        sendto_ssjoin5_servs (channel_p, client_p,
                              ":%s SJOIN %ld %s +nt :%s", me.name,
                              channel_p->channelts, name, parv[0]);

        sendto_pressjoin3_servs (channel_p, client_p,
                                 ":%s SJOIN %ld %s +nt :%s", me.name,
                                 channel_p->channelts, name, parv[0]);
      }
    }
    /*
     * Propogate channel creation SJOIN.
     */
    else if (MyClient (source_p) && flags)
    {
      channel_p->channelts = timeofday;

      sendto_ssjoin5_servs (channel_p, client_p,
                            ":%s SJOIN %ld %s +nt :%s", me.name,
                            channel_p->channelts, name, parv[0]);

      sendto_pressjoin3_servs (channel_p, client_p,
                               ":%s SJOIN %ld %s +nt :%s", me.name,
                               channel_p->channelts, name, parv[0]);
    }
    /*
     * Propogate channel join SJOIN.
     */
    else if (MyClient (source_p))
    {
      sendto_ssjoin5_servs (channel_p, client_p, newCliSJOINFmt, parv[0],
                            channel_p->channelts, name);


      sendto_pressjoin3_servs (channel_p, client_p, newCliSJOINFmt,
                               parv[0], channel_p->channelts, name);
    }
    /*
     * Should never happen.
     */
    else
    {
      sendto_match_servs (channel_p, client_p, ":%s JOIN :%s", parv[0], name);
    }

    /*
     * notify all other users on the new channel
     */
    sendto_channel_butserv (channel_p, source_p, ":%s JOIN :%s", parv[0],
                            name);

    if (MyClient (source_p))
    {
      del_invite (source_p, channel_p);
      if (channel_p->topic[0] != '\0')
      {
        sendto_one (source_p, rpl_str (RPL_TOPIC), me.name,
                    parv[0], name, channel_p->topic);
        sendto_one (source_p, rpl_str (RPL_TOPICWHOTIME),
                    me.name, parv[0], name,
                    channel_p->topic_info, channel_p->topic_time);
      }

      parv[1] = name;
      m_names (client_p, source_p, 2, parv);
    }
  }

#ifdef ANTI_SPAMBOT
  if (MyConnect (source_p) && successful_join_count)
  {
    source_p->last_join_time = NOW;
  }
#endif
  return 0;
}


/*
** m_fjoin
**
** m_fjoin is a masterkey system allowing opers to bypass
** any limitations which normally would prevent them to enter
** the channel. This eliminates all the previous cludges and hacks
** in can_join where opers could previously walk trough anything using
** m_join. - ShadowMaster
**
** parv[0] = sender prefix
** parv[1] = channel
*/
int
m_fjoin (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  static char jbuf[BUFSIZE];
  aChannel *channel_p;
  char *name, *key = NULL;
  int i, flags = 0, chanlen = 0;
  char *p = NULL, *p2 = NULL;
  char *nick;

  if (!(source_p->user))
  {
    /* something is *fucked* - bail */
    return 0;
  }

  if (!IsAnOper (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
    return 0;
  }

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "FJOIN");
    return 0;
  }

  *jbuf = '\0';
  /*
   * * Rebuild list of channels joined to be the actual result of the *
   * JOIN.  Note that "JOIN 0" is the destructive problem.
   */

  for (i = 0, name = strtoken (&p, parv[1], ","); name;
       name = strtoken (&p, (char *) NULL, ","))
  {
    /*
     * pathological case only on longest channel name. * If not dealt
     * with here, causes desynced channel ops * since ChannelExists()
     * doesn't see the same channel * as one being joined. cute bug.
     * Oct 11 1997, Dianora/comstud
     */
    if (!check_channelname (source_p, (unsigned char *) name))
      continue;

    chanlen = strlen (name);

    if (chanlen > CHANNELLEN)
    {                           /* same thing is done in get_channel() */
      name[CHANNELLEN] = '\0';
      chanlen = CHANNELLEN;
    }
    if (*name == '&' && !MyConnect (source_p))
      continue;

    else if (!IsChannelName (name))
    {
      if (MyClient (source_p))
        sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                    me.name, parv[0], name);
      continue;
    }

    if (*jbuf)
      (void) strcat (jbuf, ",");
    (void) strncat (jbuf, name, sizeof (jbuf) - i - 1);
    i += chanlen + 1;
  }


  /*
   * (void)strcpy(parv[1], jbuf); 
   */

  p = NULL;
  if (parv[2])
    key = strtoken (&p2, parv[2], ",");
  parv[2] = NULL;               /*
                                 * for m_names call later, parv[parc]
                                 * * must == NULL 
                                 */
  for (name = strtoken (&p, jbuf, ","); name;
       key = (key) ? strtoken (&p2, NULL, ",") : NULL,
       name = strtoken (&p, NULL, ","))
  {
    if (MyConnect (source_p))
    {
      flags = (ChannelExists (name)) ? 0 : CHFL_CHANOP;
    }
    else
    {
      /*
       * * complain for remote JOINs to existing channels * (they
       * should be SJOINs) -orabidoo
       */
      if (!ChannelExists (name))
        ts_warn ("User on %s remotely FJOINing new channel",
                 source_p->user->server);
    }

    /* ugly hack - ShadowMaster */

    if (MyClient (source_p) && flags == CHFL_CHANOP)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [FJOIN] -- Error: Cannot FJOIN into non existent channel %s",
                  me.name, parv[0], name);

      continue;
    }

    /*
     * We never allow channel creation in FJOIN - ShadowMaster
     */
    channel_p = get_channel (source_p, name, 0, NULL);

    /*
     * This is a bit ugly. We for orders sake do not want opers to use FJOIN when they
     * do not need to do so as we dont want to encourage the ability to use FJOIN as a replacement
     * for JOIN for those who have access to it. - ShadowMaster
     */

    if (!is_banned (source_p, channel_p)
        && (!channel_p->mode.mode & MODE_INVITEONLY)
        && (!*channel_p->mode.key))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [FJOIN] -- Error: Cannot FJOIN into a channel you can use JOIN to get into.",
                  me.name, parv[0]);

      continue;

    }


    if (!channel_p)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [FJOIN] -- Error: Sorry, cannot join channel %s.",
                  me.name, parv[0], name);

      continue;
    }
    if (IsMember (source_p, channel_p))
      continue;

    nick = source_p->name;

    /*
     * *  Complete user entry to the new channel (if any)
     */

    add_user_to_channel (channel_p, source_p, 0);
    /*
     * *  Set timestamp if appropriate, and propagate
     */

    if (MyClient (source_p))
    {
      sendto_ssjoin5_servs (channel_p, client_p, newCliSJOINFmt, parv[0],
                            channel_p->channelts, name);

      sendto_pressjoin3_servs (channel_p, client_p, newCliSJOINFmt,
                               parv[0], channel_p->channelts, name);



      sendto_netglobal
        ("from %s: \2[FJOIN]\2 %s is forcing entry to %s",
         me.name, source_p->name, channel_p->chname);
      sendto_serv_butone (source_p,
                          ":%s NETGLOBAL :\2[FJOIN]\2 %s is forcing entry to %s",
                          me.name, source_p->name, channel_p->chname);

      sendto_channelops_butone (NULL, &me, channel_p,
                                ":%s NOTICE !@%%%s :[To %s Staff] -- %s has forced entry into the channel.",
                                me.name, channel_p->chname,
                                channel_p->chname, source_p->name,
                                channel_p->chname);

      ilog (LOGF_OPERACT, "[FJOIN] %s is forcing %s to join %s",
            source_p->name, client_p->name, channel_p->chname);
    }
    else
    {
      sendto_match_servs (channel_p, client_p, ":%s FJOIN :%s", parv[0],
                          name);
    }

    /*
     * notify all other users on the new channel
     */
    sendto_channel_butserv (channel_p, source_p, ":%s JOIN :%s", parv[0],
                            name);

    if (MyClient (source_p))
    {
      del_invite (source_p, channel_p);
      if (channel_p->topic[0] != '\0')
      {
        sendto_one (source_p, rpl_str (RPL_TOPIC), me.name,
                    parv[0], name, channel_p->topic);
        sendto_one (source_p, rpl_str (RPL_TOPICWHOTIME),
                    me.name, parv[0], name,
                    channel_p->topic_info, channel_p->topic_time);
      }
      parv[1] = name;
      (void) m_names (client_p, source_p, 2, parv);
    }
  }
  return 0;
}


/*
 * * m_part * parv[0] = sender prefix *       parv[1] = channel *
 * parv[2] = Optional part reason
 */
int
m_part (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p;
  char *p, *name;
  char *reason = (parc > 2 && parv[2]) ? parv[2] : NULL;

  if (parc < 2 || parv[1][0] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "PART");
    return 0;
  }

  name = strtoken (&p, parv[1], ",");

#ifdef ANTI_SPAMBOT             /*
                                 * Dianora 
                                 */
  /*
   * if its my client, and isn't an oper 
   */

  if (name && MyConnect (source_p) && !IsAnOper (source_p))
  {
    if (source_p->join_leave_count >= spam_num)
    {
      sendto_realops_lev (SPAM_LEV,
                          "User %s (%s@%s) is a possible spambot",
                          source_p->name, source_p->user->username,
                          source_p->user->host);
      source_p->oper_warn_count_down = OPER_SPAM_COUNTDOWN;
    }
    else
    {
      int t_delta;

      if ((t_delta = (NOW - source_p->last_leave_time)) >
          JOIN_LEAVE_COUNT_EXPIRE_TIME)
      {
        int decrement_count;

        decrement_count = (t_delta / JOIN_LEAVE_COUNT_EXPIRE_TIME);

        if (decrement_count > source_p->join_leave_count)
          source_p->join_leave_count = 0;
        else
          source_p->join_leave_count -= decrement_count;
      }
      else
      {
        if ((NOW - (source_p->last_join_time)) < spam_time)
        {
          /*
           * oh, its a possible spambot 
           */
          source_p->join_leave_count++;
        }
      }
      source_p->last_leave_time = NOW;
    }
  }
#endif

  while (name)
  {
    channel_p = get_channel (source_p, name, 0, NULL);
    if (!channel_p)
    {
      sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                  me.name, parv[0], name);
      name = strtoken (&p, (char *) NULL, ",");
      continue;
    }

    if (!IsMember (source_p, channel_p))
    {
      sendto_one (source_p, err_str (ERR_NOTONCHANNEL), me.name, parv[0],
                  name);
      name = strtoken (&p, (char *) NULL, ",");
      continue;
    }
    /*
     * *  Remove user from the old channel (if any)
     */

    if (parc < 3 || can_send (source_p, channel_p, reason))
      sendto_match_servs (channel_p, client_p, PartFmt, parv[0], name);
    else
      sendto_match_servs (channel_p, client_p, PartFmt2, parv[0], name,
                          reason);
    if (parc < 3 || can_send (source_p, channel_p, reason))
      sendto_channel_butserv (channel_p, source_p, PartFmt, parv[0], name);
    else
      sendto_channel_butserv (channel_p, source_p, PartFmt2, parv[0], name,
                              reason);
    remove_user_from_channel (source_p, channel_p);
    name = strtoken (&p, (char *) NULL, ",");
  }
  return 0;
}

/*
 * * m_kick * parv[0] = sender prefix *       parv[1] = channel *
 * parv[2] = client to kick *   parv[3] = kick comment
 */
int
m_kick (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *who;
  aChannel *channel_p;
  int chasing = 0;
  int user_count;               /*

                                 * count nicks being kicked, only allow 4 
                                 */
  char *comment, *name, *p = NULL, *user, *p2 = NULL;

  if (parc < 3 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "KICK");
    return 0;
  }
  if (IsServer (source_p) && !IsULine (source_p))
    sendto_ops ("KICK from %s for %s %s", parv[0], parv[1], parv[2]);
  comment = (BadPtr (parv[3])) ? parv[0] : parv[3];
  if (strlen (comment) > (size_t) TOPICLEN)
    comment[TOPICLEN] = '\0';

  *nickbuf = *buf = '\0';
  name = strtoken (&p, parv[1], ",");

  while (name)
  {
    channel_p = get_channel (source_p, name, !CREATE, NULL);
    if (!channel_p)
    {
      sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                  me.name, parv[0], name);
      name = strtoken (&p, (char *) NULL, ",");
      continue;
    }

    /*
     * You either have chan op privs, or you don't -Dianora 
     */
    /*
     * orabidoo and I discussed this one for a while... I hope he
     * approves of this code, users can get quite confused...
     * -Dianora
     */

    if (!IsServer (source_p) && !is_chan_admin (source_p, channel_p)
        && !is_chan_op (source_p, channel_p)
        && !is_half_op (source_p, channel_p) && !IsULine (source_p))
    {
      /*
       * was a user, not a server, and user isn't seen as a chanop
       * here
       */

      if (MyConnect (source_p))
      {
        /*
         * user on _my_ server, with no chanops.. so go away 
         */

        sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                    me.name, parv[0], channel_p->chname);
        name = strtoken (&p, (char *) NULL, ",");
        continue;
      }

      if (channel_p->channelts == 0)
      {
        /*
         * If its a TS 0 channel, do it the old way 
         */

        sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                    me.name, parv[0], channel_p->chname);
        name = strtoken (&p, (char *) NULL, ",");
        continue;
      }

      /*
       * Its a user doing a kick, but is not showing as chanop
       * locally its also not a user ON -my- server, and the channel
       * has a TS. There are two cases we can get to this point
       * then...
       * 
       * 1) connect burst is happening, and for some reason a legit op
       * has sent a KICK, but the SJOIN hasn't happened yet or been
       * seen. (who knows.. due to lag...)
       * 
       * 2) The channel is desynced. That can STILL happen with TS
       * 
       * Now, the old code roger wrote, would allow the KICK to go
       * through. Thats quite legit, but lets weird things like
       * KICKS by users who appear not to be chanopped happen, or
       * even neater, they appear not to be on the channel. This
       * fits every definition of a desync, doesn't it? ;-) So I
       * will allow the KICK, otherwise, things are MUCH worse. But
       * I will warn it as a possible desync.
       * 
       * -Dianora
       */
      /*
       * sendto_one(source_p, err_str(ERR_DESYNC), me.name, parv[0],
       * channel_p->chname);
       */
      /*
       * After more discussion with orabidoo...
       * 
       * The code was sound, however, what happens if we have +h (TS4)
       * and some servers don't understand it yet? we will be seeing
       * servers with users who appear to have no chanops at all,
       * merrily kicking users.... -Dianora
       * 
       */
    }
    user = strtoken (&p2, parv[2], ",");
    user_count = 4;
    while (user && user_count)
    {
      user_count--;
      if (!(who = find_chasing (source_p, user, &chasing)))
      {
        user = strtoken (&p2, (char *) NULL, ",");
        continue;               /*
                                 * No such user left! 
                                 */
      }

      if (IsMember (who, channel_p))
      {
        if (MyConnect (source_p) && IsServicesClient (who))
        {
          sendto_one (source_p,
                      ":%s NOTICE %s :You cannot kick %s on %s (Services Client)",
                      me.name, parv[0], user, channel_p->chname);
        }
        else if (MyConnect (source_p) && IsProtectedOper (who)
                 && !IsProtectedOper (source_p)
                 && !IsNetAdmin (source_p) && !IsNetCoAdmin (source_p))
        {
          sendto_one (source_p,
                      ":%s NOTICE %s :You cannot kick %s on %s (Protected IRC Operator)",
                      me.name, parv[0], user, channel_p->chname);
          sendto_one (who,
                      ":%s NOTICE %s :%s just attempted to kick you on %s with reason: %s",
                      me.name, user, parv[0], channel_p->chname, comment);
        }
        else if (MyConnect (source_p) && is_chan_admin (who, channel_p)
                 && !is_chan_admin (source_p, channel_p))
        {
          sendto_one (source_p,
                      ":%s NOTICE %s :You cannot kick %s on %s without being a Channel Administrator",
                      me.name, parv[0], user, channel_p->chname);
          sendto_one (who,
                      ":%s NOTICE %s :%s just attempted to kick you on %s with reason: %s",
                      me.name, user, parv[0], channel_p->chname, comment);
        }
        else if (MyConnect (source_p) && is_chan_op (who, channel_p)
                 && !is_chan_op (source_p, channel_p)
                 && !is_chan_admin (source_p, channel_p))
        {
          sendto_one (source_p,
                      ":%s NOTICE %s :You cannot kick %s on %s without being a Channel Operator",
                      me.name, parv[0], user, channel_p->chname);
          sendto_one (who,
                      ":%s NOTICE %s :%s just attempted to kick you on %s with reason: %s",
                      me.name, user, parv[0], channel_p->chname, comment);
        }
        else if (MyConnect (source_p) && is_half_op (who, channel_p)
                 && !is_chan_op (source_p, channel_p)
                 && !is_chan_admin (source_p, channel_p))
        {
          sendto_one (source_p,
                      ":%s NOTICE %s :You cannot kick %s on %s without being a Channel Operator",
                      me.name, parv[0], user, channel_p->chname);
          sendto_one (who,
                      ":%s NOTICE %s :%s just attempted to kick you on %s with reason: %s",
                      me.name, user, parv[0], channel_p->chname, comment);
        }
        else
        {
          sendto_channel_butserv (channel_p, source_p,
                                  ":%s KICK %s %s :%s", parv[0], name,
                                  who->name, comment);
          sendto_match_servs (channel_p, client_p,
                              ":%s KICK %s %s :%s", parv[0], name,
                              who->name, comment);
          remove_user_from_channel (who, channel_p);
        }
      }
      else
        sendto_one (source_p, err_str (ERR_USERNOTINCHANNEL),
                    me.name, parv[0], user, name);
      user = strtoken (&p2, (char *) NULL, ",");
    }                           /*
                                 * loop on parv[2] 
                                 */

    name = strtoken (&p, (char *) NULL, ",");
  }                             /*
                                 * loop on parv[1] 
                                 */

  return (0);
}

int
count_channels (aClient * source_p)
{
  aChannel *channel_p;
  int count = 0;

  for (channel_p = channel; channel_p; channel_p = channel_p->nextch)
    count++;
  return (count);
}

void
send_topic_burst (aClient * client_p)
{
  aChannel *channel_p;
  aClient *target_p;
  for (channel_p = channel; channel_p; channel_p = channel_p->nextch)
  {
    if (channel_p->topic[0] != '\0')
      sendto_one (client_p, ":%s TOPIC %s %s %ld :%s", me.name,
                  channel_p->chname, channel_p->topic_info,
                  channel_p->topic_time, channel_p->topic);
  }
  for (target_p = client; target_p; target_p = target_p->next)
  {
    if (!IsPerson (target_p) || target_p->from == client_p)
      continue;
    if (target_p->user->away)
      sendto_one (client_p, ":%s AWAY :%s", target_p->name,
                  target_p->user->away);
  }
}

/*
 * m_topic
 * parv[0] = sender prefix
 * parv[1] = topic text
 */
int
m_topic (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p = NullChn;
  char *topic = NULL, *name;
  time_t ts = timeofday;
  int member;
  char tmp_topic_info[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS), me.name, parv[0],
                "TOPIC");
    return 0;
  }

  name = parv[1];
  channel_p = find_channel (name, NullChn);
  if (!channel_p)
  {
    sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL), me.name, parv[0],
                name);
    return 0;
  }

  member = IsMember (source_p, channel_p);

  if (parc == 2)                /* user is requesting a topic */
  {
    char *namep = channel_p->chname;
    char tempchname[CHANNELLEN + 2];

    if (!member && !(ShowChannel (source_p, channel_p)))
    {
      if (IsServerAdmin (source_p))
      {
        tempchname[0] = '%';
        strcpy (&tempchname[1], channel_p->chname);
        namep = tempchname;
      }
      else
      {
        sendto_one (source_p, err_str (ERR_NOTONCHANNEL), me.name,
                    parv[0], name);
        return 0;
      }
    }

    if (channel_p->topic[0] == '\0')
      sendto_one (source_p, rpl_str (RPL_NOTOPIC), me.name, parv[0], namep);
    else
    {
      sendto_one (source_p, rpl_str (RPL_TOPIC), me.name, parv[0], namep,
                  channel_p->topic);
      sendto_one (source_p, rpl_str (RPL_TOPICWHOTIME), me.name, parv[0],
                  namep, channel_p->topic_info, channel_p->topic_time);
    }
    return 0;
  }

  if (!member && !IsServer (source_p) && !IsULine (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOTONCHANNEL), me.name, parv[0], name);
    return 0;
  }

  if (parc > 3
      && (!MyConnect (source_p) || IsULine (source_p) || IsServer (source_p)))
  {
    topic = (parc > 4 ? parv[4] : "");
    strcpy (tmp_topic_info, parv[2]);
    ts = atoi (parv[3]);
  }
  else
  {
    /*
       ircsprintf (tmp_topic_info, "%s!%s@%s",
       source_p->name, source_p->user->username,
       IsHidden (source_p) ? source_p->user->virthost : source_p->
       user->host);
     */
    strcpy (tmp_topic_info, source_p->name);
    topic = parv[2];
  }

  if (((!(channel_p->mode.mode & MODE_TOPICLIMIT)
        || is_chan_op (source_p, channel_p)
        || is_chan_admin (source_p, channel_p)
        || is_half_op (source_p, channel_p)) || IsULine (source_p)
       || IsServer (source_p)))
  {
    /* setting a topic */

    /* local topic is newer than remote topic and we have a topic
       and we're in a synch (server setting topic) */

    if (IsServer (source_p) && !IsULine (source_p)
        && channel_p->topic_time >= ts && channel_p->topic[0])
      return 0;

    strncpyzt (channel_p->topic, topic, TOPICLEN + 1);
    strcpy (channel_p->topic_info, tmp_topic_info);
    channel_p->topic_time = ts;

    /* in this case I think it's better that we send all the info that df 
     * sends with the topic, so I changed everything to work like that. -wd */

    sendto_match_servs (channel_p, client_p, ":%s TOPIC %s %s %lu :%s",
                        parv[0], channel_p->chname, channel_p->topic_info,
                        channel_p->topic_time, channel_p->topic);
    sendto_channel_butserv (channel_p, source_p, ":%s TOPIC %s :%s",
                            parv[0], channel_p->chname, channel_p->topic);
  }
  else
    sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED), me.name, parv[0],
                channel_p->chname);

  return 0;
}

/*
 * * m_invite *       parv[0] - sender prefix *       parv[1] - user to
 * invite *     parv[2] - channel number
 */
int
m_invite (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  aChannel *channel_p;

  if (parc < 3 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "INVITE");
    return -1;
  }

  if (!(target_p = find_person (parv[1], (aClient *) NULL)))
  {
    sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0],
                parv[1]);
    return 0;
  }

  if (!check_channelname (source_p, (unsigned char *) parv[2]))
    return 0;

  if (!(channel_p = find_channel (parv[2], NullChn)))
  {
    sendto_prefix_one (target_p, source_p, ":%s INVITE %s :%s",
                       parv[0], parv[1], parv[2]);
    return 0;
  }

  if (channel_p && !IsMember (source_p, channel_p) && !IsULine (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOTONCHANNEL),
                me.name, parv[0], parv[2]);
    return -1;
  }

  if (IsMember (target_p, channel_p))
  {
    sendto_one (source_p, err_str (ERR_USERONCHANNEL),
                me.name, parv[0], parv[1], parv[2]);
    return 0;
  }
  /*
   * Channel is +R (Registered Nicknames only)
   *
   * We will let SSL Client rooms invite anyone, but we ensure everyone knows
   * when a non SSL Client joins the channel.
   */

  if (channel_p && (channel_p->mode.mode & MODE_SECURE))
  {
    if (!is_chan_op (source_p, channel_p)
        && !is_chan_admin (source_p, channel_p) && (!IsULine (source_p)))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0], channel_p->chname);
      return -1;
    }
    else if (!IsMember (source_p, channel_p) && !IsULine (source_p))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0],
                  ((channel_p) ? (channel_p->chname) : parv[2]));
      return -1;
    }
  }

  /*
   * Channel is +S (SSL CLients only)
   */

  if (channel_p && (channel_p->mode.mode & MODE_REGONLY))
  {
    if (!is_chan_op (source_p, channel_p)
        && !is_chan_admin (source_p, channel_p) && (!IsULine (source_p)))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0], channel_p->chname);
      return -1;
    }
    else if (!IsMember (source_p, channel_p) && !IsULine (source_p))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0],
                  ((channel_p) ? (channel_p->chname) : parv[2]));
      return -1;
    }
  }

  /*
   * Channel is +O (Oper Only) or Channel is +A (Admin Only)
   *
   * We want staff above or equal to Channel Operator to be able to invite
   * users to the channel. +i and +N have no function whatsoever on +O and +A channels.
   */
  if (channel_p
      && ((channel_p->mode.mode & MODE_OPERONLY)
          || (channel_p->mode.mode & MODE_ADMINONLY)))
  {
    if (!is_chan_op (source_p, channel_p)
        && !is_chan_admin (source_p, channel_p) && (!IsULine (source_p)))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0], channel_p->chname);
      return -1;
    }
    else if (!IsMember (source_p, channel_p) && !IsULine (source_p))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0],
                  ((channel_p) ? (channel_p->chname) : parv[2]));
      return -1;
    }
  }

  /*
   * Channel is +i (Invite only)
   *
   * We want staff above or equal to channel operators to be able to invite
   * users to the channel. +N have no effect on +i channels.
   */
  if (channel_p && (channel_p->mode.mode & MODE_INVITEONLY))
  {
    if (!is_chan_op (source_p, channel_p)
        && !is_chan_admin (source_p, channel_p) && (!IsULine (source_p)))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0], channel_p->chname);
      return -1;
    }
    else if (!IsMember (source_p, channel_p) && !IsULine (source_p))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0],
                  ((channel_p) ? (channel_p->chname) : parv[2]));
      return -1;
    }
  }
  /*
   * Channel is +N (No invites)
   *
   * We want staff above or equal to channel operators to be able to invite
   * users to the channel. +N is useless on +i channels..
   */
  if (channel_p && (channel_p->mode.mode & MODE_NOINVITE))
  {
    if (!is_chan_op (source_p, channel_p)
        && !is_chan_admin (source_p, channel_p) && (!IsULine (source_p)))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0], channel_p->chname);
      return -1;
    }
    else if (!IsMember (source_p, channel_p) && !IsULine (source_p))
    {
      sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                  me.name, parv[0],
                  ((channel_p) ? (channel_p->chname) : parv[2]));
      return -1;
    }
  }
  /*
   * Check if user being invited is banned on channel
   */
  if (channel_p && is_banned (target_p, channel_p)
      && !is_chan_op (source_p, channel_p)
      && !is_chan_admin (source_p, channel_p) && !IsULine (source_p))
  {
    sendto_one (source_p, err_str (ERR_CHANOPRIVSNEEDED),
                me.name, parv[0], channel_p->chname);
    return -1;
  }
  /*
   * all is good.. invite is ok..
   */
  if (MyConnect (source_p))
  {
    sendto_one (source_p, rpl_str (RPL_INVITING), me.name, parv[0],
                target_p->name,
                ((channel_p) ? (channel_p->chname) : parv[2]));
    if (target_p->user->away)
      sendto_one (source_p, rpl_str (RPL_AWAY), me.name, parv[0],
                  target_p->name, target_p->user->away);
  }

  if (MyConnect (target_p))
  {
    if (channel_p->mode.mode & MODE_SECURE && !IsSSLClient (target_p))
    {
      sendto_one (source_p, err_str (ERR_NOTSSLCLIENT),
                  me.name, parv[0], channel_p->chname);
      return -1;
    }
    else if ((channel_p->mode.mode & MODE_NOINVITE)
             || (channel_p->mode.mode & MODE_INVITEONLY)
             || (channel_p->mode.mode & MODE_OPERONLY)
             || (channel_p->mode.mode & MODE_ADMINONLY)
             || (channel_p->mode.mode & MODE_SECURE)
             || (channel_p->mode.mode & MODE_REGONLY)
             || is_banned (target_p, channel_p))
    {
      /*
       * Let the rest of the staff know an invite have happened if invite is somehow restricted
       */
      sendto_allchannelops_butone (NULL, &me, channel_p,
                                   ":%s NOTICE !@%%%s :[To %s Staff] -- %s invited %s into channel %s.",
                                   me.name, channel_p->chname,
                                   channel_p->chname, source_p->name,
                                   target_p->name, channel_p->chname);
      /*
       * Add the actual invite
       */
      add_invite (target_p, channel_p);
    }
  }
  sendto_prefix_one (target_p, source_p, ":%s INVITE %s :%s", parv[0],
                     target_p->name,
                     ((channel_p) ? (channel_p->chname) : parv[2]));

  return 0;
}


/*
 * The function which sends the actual channel list back to the user.
 * Operates by stepping through the hashtable, sending the entries back if
 * they match the criteria.
 * client_p = Local client to send the output back to.
 * numsend = Number (roughly) of lines to send back. Once this number has
 * been exceeded, send_list will finish with the current hash bucket,
 * and record that number as the number to start next time send_list
 * is called for this user. So, this function will almost always send
 * back more lines than specified by numsend (though not by much,
 * assuming CH_MAX is was well picked). So be conservative in your choice
 * of numsend. -Rak
 */

void
send_list (aClient * client_p, int numsend)
{
  aChannel *channel_p;
  LOpts *lopt = client_p->user->lopt;
  int hashnum;
  char tempbuff[KEYLEN + 8 + 3 + 1 + 3 + 6];
  char modestuff[TOPICLEN + 3 + KEYLEN + 8 + 3 + 1 + 6];

  /*
   * Step trough the hashtable.
   */
  for (hashnum = lopt->starthash; hashnum < CH_MAX; hashnum++)
  {
    if (numsend > 0)
      /*
       * Step trough the channels
       */
      for (channel_p = (aChannel *) hash_get_chan_bucket (hashnum);
           channel_p; channel_p = channel_p->hnextch)
      {
        /*
         * Slight temporary cludge to allow opers to see secret and private channels in a /LIST
         * We need to make a define for this to be able to turn it off - ShadowMaster
         */

        /*
         *** FIXME ***
         *
         * This needs to have its own flag added, secret channels should NOT be shown
         * by default.
         */
        if (SecretChannel (channel_p) && !IsMember (client_p, channel_p)
            && !IsAnOper (client_p))
          continue;

        /*
         * Skip channels that dont match the criteria.
         */
        if ((!lopt->showall) && ((channel_p->users < lopt->usermin) ||
                                 ((lopt->usermax >= 0)
                                  && (channel_p->users > lopt->usermax))
                                 || ((channel_p->channelts || 1) <
                                     lopt->chantimemin)
                                 || (channel_p->topic_time <
                                     lopt->topictimemin)
                                 || (channel_p->channelts >
                                     lopt->chantimemax)
                                 || (channel_p->topic_time >
                                     lopt->topictimemax)
                                 || (lopt->nolist
                                     && find_str_link (lopt->nolist,
                                                       channel_p->chname))
                                 || (lopt->yeslist
                                     && !find_str_link (lopt->yeslist,
                                                        channel_p->chname))))
          continue;

        /*
         * Show modes in the /list for operators.
         */
        if (IsAnOper (client_p))
        {
          *modebuf = *parabuf = '\0';
          modebuf[1] = '\0';
          channel_modes (client_p, modebuf, parabuf, channel_p);
          if (SecretChannel (channel_p) || HiddenChannel (channel_p))
          {
            ircsprintf (tempbuff, "\002[%s %s]\002", modebuf, parabuf);
          }
          else
          {
            ircsprintf (tempbuff, "[%s %s]", modebuf, parabuf);
          }
          ircsprintf (modestuff, "%-20s %s", tempbuff, channel_p->topic);
          sendto_one (client_p, rpl_str (RPL_LIST), me.name,
                      client_p->name, channel_p->chname,
                      channel_p->users, modestuff);
        }
        else
        {
          sendto_one (client_p, rpl_str (RPL_LIST), me.name,
                      client_p->name, ShowChannel (client_p,
                                                   channel_p) ?
                      channel_p->chname : "*", channel_p->users,
                      ShowChannel (client_p,
                                   channel_p) ? channel_p->topic : "");
        }
        numsend--;
      }
    else
      break;
  }

  /* All done */
  if (hashnum == CH_MAX)
  {
    Link *lp, *next;
    sendto_one (client_p, rpl_str (RPL_LISTEND), me.name, client_p->name);
    for (lp = lopt->yeslist; lp; lp = next)
    {
      next = lp->next;
      free_link (lp);
    }
    for (lp = lopt->nolist; lp; lp = next)
    {
      next = lp->next;
      free_link (lp);
    }

    MyFree (client_p->user->lopt);
    client_p->user->lopt = NULL;
    remove_from_list (&listing_clients, client_p, NULL);
    return;
  }

  /*
   * We've exceeded the limit on the number of channels to send back
   * at once.
   */
  lopt->starthash = hashnum;
  return;
}


/*
 * m_list
 *
 * parv[0] = sender prefix
 * parv[1] = channel
 */
int
m_list (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p;
  time_t currenttime = time (NULL);
  char *name, *p = NULL;
  LOpts *lopt = NULL;
  Link *lp, *next;
  int usermax, usermin, error = 0, doall = 0;
  int x;
  time_t chantimemin, chantimemax;
  ts_val topictimemin, topictimemax;
  Link *yeslist = NULL, *nolist = NULL;
  static char *usage[] = {
    "   Usage: /raw LIST options (on mirc) or /quote LIST options (ircII)",
    "",
    "If you don't include any options, the default is to send you the",
    "entire unfiltered list of channels. Below are the options you can",
    "use, and what channels LIST will return when you use them.",
    ">number  List channels with more than <number> people.",
    "<number  List channels with less than <number> people.",
    "C>number List channels created between now and <number> minutes ago.",
    "C<number List channels created earlier than <number> minutes ago.",
    "T>number List channels whose topics are older than <number> minutes",
    "         (Ie, they have not changed in the last <number> minutes.",
    "T<number List channels whose topics are not older than <number> minutes.",
    "*mask*   List channels that match *mask*",
    "!*mask*  List channels that do not match *mask*", NULL
  };
  /* Some starting san checks -- No interserver lists allowed. */
  if (client_p != source_p || !source_p->user)
    return 0;
  /* If a /list is in progress, then another one will cancel it */
  if ((lopt = source_p->user->lopt) != NULL)
  {
    sendto_one (source_p, rpl_str (RPL_LISTEND), me.name, parv[0]);
    for (lp = lopt->yeslist; lp; lp = next)
    {
      next = lp->next;
      free_link (lp);
    }
    for (lp = lopt->nolist; lp; lp = next)
    {
      next = lp->next;
      free_link (lp);
    }
    MyFree (source_p->user->lopt);
    source_p->user->lopt = NULL;
    remove_from_list (&listing_clients, source_p, NULL);
    return 0;
  }

  /* if HTM, drop this too */
  if (lifesux)
  {
    sendto_one (source_p, rpl_str (RPL_LOAD2HI), me.name, source_p->name);
    return 0;
  }

  if (parc < 2 || BadPtr (parv[1]))
  {

    sendto_one (source_p, rpl_str (RPL_LISTSTART), me.name, parv[0]);
    lopt = source_p->user->lopt = (LOpts *) MyMalloc (sizeof (LOpts));
    memset (lopt, '\0', sizeof (LOpts));
    lopt->showall = 1;

    add_to_list (&listing_clients, source_p);

    if (DBufLength (&client_p->sendQ) < 2048)
      send_list (client_p, 64);
    return 0;
  }

  if ((parc == 2) && (parv[1][0] == '?') && (parv[1][1] == '\0'))
  {
    char **ptr = usage;
    for (; *ptr; ptr++)
      sendto_one (source_p, rpl_str (RPL_COMMANDSYNTAX), me.name,
                  client_p->name, *ptr);
    return 0;
  }

  sendto_one (source_p, rpl_str (RPL_LISTSTART), me.name, parv[0]);
  chantimemax = topictimemax = currenttime + 86400;
  chantimemin = topictimemin = 0;
  usermin = 1;                  /* By default, set the minimum to 1 users */
  usermax = -1;                 /* No maximum */
  for (name = strtoken (&p, parv[1], ","); name && !error;
       name = strtoken (&p, (char *) NULL, ","))
  {

    switch (*name)
    {
       case '<':
         usermax = atoi (name + 1) - 1;
         doall = 1;
         break;
       case '>':
         usermin = atoi (name + 1) + 1;
         doall = 1;
         break;
       case 'C':
       case 'c':               /* Channel TS time -- creation time? */
         ++name;
         switch (*name++)
         {
            case '<':
              chantimemax = currenttime - 60 * atoi (name);
              doall = 1;
              break;
            case '>':
              chantimemin = currenttime - 60 * atoi (name);
              doall = 1;
              break;
            default:
              sendto_one (source_p, err_str (ERR_LISTSYNTAX), me.name,
                          client_p->name);
              error = 1;
         }
         break;
       case 'T':
       case 't':
         ++name;
         switch (*name++)
         {
            case '<':
              topictimemax = currenttime - 60 * atoi (name);
              doall = 1;
              break;
            case '>':
              topictimemin = currenttime - 60 * atoi (name);
              doall = 1;
              break;
            default:
              sendto_one (source_p, err_str (ERR_LISTSYNTAX), me.name,
                          client_p->name);
              error = 1;
         }
         break;
       default:                /* A channel, possibly with wildcards.
                                 * Thought for the future: Consider turning wildcard
                                 * processing on the fly.
                                 * new syntax: !channelmask will tell ircd to ignore
                                 * any channels matching that mask, and then
                                 * channelmask will tell ircd to send us a list of
                                 * channels only masking channelmask. Note: Specifying
                                 * a channel without wildcards will return that
                                 * channel even if any of the !channelmask masks
                                 * matches it.
                                 */
         if (*name == '!')
         {
           doall = 1;
           lp = make_link ();
           lp->next = nolist;
           nolist = lp;
           DupString (lp->value.cp, name + 1);
         }
         else if (strchr (name, '*') || strchr (name, '*'))
         {
           doall = 1;
           lp = make_link ();
           lp->next = yeslist;
           yeslist = lp;
           DupString (lp->value.cp, name);
         }
         else                   /* Just a normal channel */
         {
           channel_p = find_channel (name, NullChn);
           if (channel_p
               && ((x = ShowChannel (source_p, channel_p))
                   || IsServerAdmin (source_p)))
           {
             char *nameptr = name;
             char channame[CHANNELLEN + 2];
             if (!x && IsServerAdmin (source_p))
             {
               channame[0] = '%';
               strcpy (&channame[1], channel_p->chname);
               nameptr = channame;
             }

             sendto_one (source_p, rpl_str (RPL_LIST), me.name, parv[0],
                         nameptr, channel_p->users, channel_p->topic);
           }
         }
    }                           /* switch */
  }                             /* while */

  if (doall)
  {
    lopt = source_p->user->lopt = (LOpts *) MyMalloc (sizeof (LOpts));
    memset (lopt, '\0', sizeof (LOpts));
    lopt->usermin = usermin;
    lopt->usermax = usermax;
    lopt->topictimemax = topictimemax;
    lopt->topictimemin = topictimemin;
    lopt->chantimemax = chantimemax;
    lopt->chantimemin = chantimemin;
    lopt->nolist = nolist;
    lopt->yeslist = yeslist;

    add_to_list (&listing_clients, source_p);

    if (DBufLength (&client_p->sendQ) < 2048)
      send_list (client_p, 64);
    return 0;
  }

  sendto_one (source_p, rpl_str (RPL_LISTEND), me.name, parv[0]);
  return 0;
}



/************************************************************************
 * m_names() - Added by Jto 27 Apr 1989
 * 12 Feb 2000 - geesh, time for a rewrite -lucas
 ************************************************************************/
/*
 * m_names 
 * parv[0] = sender prefix 
 * parv[1] = channel
 */

/* maximum names para to show to opers when abuse occurs */
#define TRUNCATED_NAMES 64

int
m_names (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int mlen = strlen (me.name) + NICKLEN + 7;
  aChannel *channel_p;
  aClient *target_p;
  int member;
  chanMember *cm;
  int idx, flag = 1, spos;
  char *s, *para = parv[1];
  if (parc < 2 || !MyConnect (source_p))
  {
    sendto_one (source_p, rpl_str (RPL_ENDOFNAMES), me.name, parv[0], "*");
    return 0;
  }

  for (s = para; *s; s++)
  {
    if (*s == ',')
    {
      if (strlen (para) > TRUNCATED_NAMES)
        para[TRUNCATED_NAMES] = '\0';
      sendto_realops ("names abuser %s %s",
                      get_client_name (source_p, FALSE), para);
      sendto_one (source_p, err_str (ERR_TOOMANYTARGETS), me.name,
                  source_p->name, "NAMES");
      return 0;
    }
  }

  if (!check_channelname (source_p, (unsigned char *) para))
    return 0;
  channel_p = find_channel (para, (aChannel *) NULL);
  if (!channel_p
      || (!ShowChannel (source_p, channel_p) && !IsAnOper (source_p)))
  {
    sendto_one (source_p, rpl_str (RPL_ENDOFNAMES), me.name, parv[0], para);
    return 0;
  }

  /* cache whether this user is a member of this channel or not */
  member = IsMember (source_p, channel_p);
  if (PubChannel (channel_p))
    buf[0] = '=';
  else if (SecretChannel (channel_p))
    buf[0] = '@';
  else
    buf[0] = '*';
  idx = 1;
  buf[idx++] = ' ';
  for (s = channel_p->chname; *s; s++)
    buf[idx++] = *s;
  buf[idx++] = ' ';
  buf[idx++] = ':';
  /* If we go through the following loop and never add anything,
     we need this to be empty, otherwise spurious things from the
     LAST /names call get stuck in there.. - lucas */
  buf[idx] = '\0';
  spos = idx;                   /* starting point in buffer for names! */
  for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    /* Opers always see invisible users */
    if (IsInvisible (target_p) && !member && !IsAnOper (source_p))
      continue;

    /* We mark users that are +i as such for opers outside the channel */
    if (!member && IsAnOper (source_p) && IsInvisible (target_p))
    {
      buf[idx++] = '=';
    }

    /* While access modes are not exclusive modes (yet) the RCF
     * documents names to only show the highest mode.
     * mIRC and XChat are known to cope fine with listing all prefixes
     * in names but some clients break horribly on this, so we stick to
     * showing only one prefix.
     * Hopefully we end up with exclusive access modes soon.
     */
    if (cm->flags & CHFL_CHANADMIN)
    {
      buf[idx++] = '!';
    }
    else if (cm->flags & CHFL_CHANOP)
    {
      buf[idx++] = '@';
    }
    else if (cm->flags & CHFL_HALFOP)
    {
      buf[idx++] = '%';
    }
    else if (cm->flags & CHFL_VOICE)
    {
      buf[idx++] = '+';
    }

    for (s = target_p->name; *s; s++)
      buf[idx++] = *s;
    buf[idx++] = ' ';
    buf[idx] = '\0';
    flag = 1;
    if (mlen + idx + NICKLEN > BUFSIZE - 3)
    {
      sendto_one (source_p, rpl_str (RPL_NAMREPLY), me.name, parv[0], buf);
      idx = spos;
      flag = 0;
    }
  }

  if (flag)
    sendto_one (source_p, rpl_str (RPL_NAMREPLY), me.name, parv[0], buf);
  sendto_one (source_p, rpl_str (RPL_ENDOFNAMES), me.name, parv[0], para);
  return 0;
}

void
send_user_joins (aClient * client_p, aClient * user)
{
  Link *lp;
  aChannel *channel_p;
  int cnt = 0, len = 0, clen;
  char *mask;
  *buf = ':';
  (void) strcpy (buf + 1, user->name);
  (void) strcat (buf, " JOIN ");
  len = strlen (user->name) + 7;
  for (lp = user->user->channel; lp; lp = lp->next)
  {
    channel_p = lp->value.channel_p;
    if (*channel_p->chname == '&')
      continue;
    if ((mask = strchr (channel_p->chname, ':')))
      if (match (++mask, client_p->name))
        continue;
    clen = strlen (channel_p->chname);
    if (clen > (size_t) BUFSIZE - 7 - len)
    {
      if (cnt)
        sendto_one (client_p, "%s", buf);
      *buf = ':';
      (void) strcpy (buf + 1, user->name);
      (void) strcat (buf, " JOIN ");
      len = strlen (user->name) + 7;
      cnt = 0;
    }
    (void) strcpy (buf + len, channel_p->chname);
    cnt++;
    len += clen;
    if (lp->next)
    {
      len++;
      (void) strcat (buf, ",");
    }
  }
  if (*buf && cnt)
    sendto_one (client_p, "%s", buf);
  return;
}

#if 0
void
kill_ban_list (aClient * client_p, aChannel * channel_p)
{
  chanMember *cm;
  aBan *bp, *bpn;
  char *cp;
  int count = 0, mysend = 0;
  cp = modebuf;
  *cp++ = '-';
  *cp = '\0';
  *parabuf = '\0';
  for (bp = channel_p->banlist; bp; bp = bp->next)
  {
    if (strlen (parabuf) + strlen (bp->banstr) + 10 < (size_t) MODEBUFLEN)
    {
      if (*parabuf)
        strcat (parabuf, " ");
      strcat (parabuf, bp->banstr);
      count++;
      *cp++ = 'b';
      *cp = '\0';
    }
    else if (*parabuf)
      mysend = 1;
    if (count == MAXMODEPARAMS)
      mysend = 1;
    if (mysend)
    {
      sendto_channel_butserv (channel_p, &me, ":%s MODE %s %s %s",
                              client_p->name, channel_p->chname, modebuf,
                              parabuf);
      mysend = 0;
      *parabuf = '\0';
      cp = modebuf;
      *cp++ = '-';
      if (count != MAXMODEPARAMS)
      {
        strcpy (parabuf, bp->banstr);
        *cp++ = 'b';
        count = 1;
      }
      else
        count = 0;
      *cp = '\0';
    }
  }

  if (*parabuf)
  {
    sendto_channel_butserv (channel_p, &me, ":%s MODE %s %s %s",
                            client_p->name, channel_p->chname, modebuf,
                            parabuf);
  }

  /* physically destroy channel ban list */

  bp = channel_p->banlist;
  while (bp)
  {
    bpn = bp->next;
    MyFree (bp->banstr);
    MyFree (bp->who);
    MyFree (bp);
    bp = bpn;
  }

  channel_p->banlist = NULL;
  /* reset bquiet on all channel members */
  for (cm = channel_p->members; cm; cm = cm->next)
  {
    if (MyConnect (cm->client_p))
      cm->bans = 0;
  }
}

void
kill_exempt_list (aClient * client_p, aChannel * channel_p)
{
  anExempt *bp, *bpn;
  char *cp;
  int count = 0, mysend = 0;
  cp = modebuf;
  *cp++ = '-';
  *cp = '\0';
  *parabuf = '\0';
  for (bp = channel_p->exemptlist; bp; bp = bp->next)
  {
    if (strlen (parabuf) + strlen (bp->exemptstr) + 10 < (size_t) MODEBUFLEN)
    {
      if (*parabuf)
        strcat (parabuf, " ");
      strcat (parabuf, bp->exemptstr);
      count++;
      *cp++ = 'e';
      *cp = '\0';
    }
    else if (*parabuf)
      mysend = 1;
    if (count == MAXMODEPARAMS)
      mysend = 1;
    if (mysend)
    {
      sendto_channel_butserv (channel_p, &me, ":%s MODE %s %s %s",
                              client_p->name, channel_p->chname, modebuf,
                              parabuf);
      mysend = 0;
      *parabuf = '\0';
      cp = modebuf;
      *cp++ = '-';
      if (count != MAXMODEPARAMS)
      {
        strcpy (parabuf, bp->exemptstr);
        *cp++ = 'e';
        count = 1;
      }
      else
        count = 0;
      *cp = '\0';
    }
  }

  if (*parabuf)
  {
    sendto_channel_butserv (channel_p, &me, ":%s MODE %s %s %s",
                            client_p->name, channel_p->chname, modebuf,
                            parabuf);
  }

  /* physically destroy channel exempt list */

  bp = channel_p->exemptlist;
  while (bp)
  {
    bpn = bp->next;
    MyFree (bp->exemptstr);
    MyFree (bp->who);
    MyFree (bp);
    bp = bpn;
  }

  channel_p->exemptlist = NULL;
}
#endif


/* m_resynch
 *
* parv[0] = sender
 * parv[1] = #channel
 *
 * Sent from a server I am directly connected to that is requesting I resend
 * EVERYTHING I know about #channel.
 */
int
m_resynch (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p;

  if (!MyConnect (source_p) || !IsServer (source_p) || parc < 2)
    return 0;

  channel_p = find_channel (parv[1], NullChn);

  sendto_realops_lev (DEBUG_LEV, "%s is requesting a resynch of %s%s",
                      parv[0], parv[1],
                      (channel_p == NullChn) ? " [failed]" : "");

  if (channel_p != NullChn)
    send_channel_modes (source_p, channel_p);
  return 0;
}

static inline void
sjoin_sendit (aClient * client_p, aClient * source_p, aChannel * channel_p,
              char *from)
{
  sendto_channel_butserv (channel_p, source_p, ":%s MODE %s %s %s", from,
                          channel_p->chname, modebuf, parabuf);
}

/*
 * m_sjoin
 * parv[0] - sender
 * parv[1] - TS
 * parv[2] - channel
 * parv[3] - modes + n arguments (key and/or limit)
 * parv[4+n] - flags+nick list (all in one parameter)
 *
 *
 * process a SJOIN, taking the TS's into account to either ignore the
 * incoming modes or undo the existing ones or merge them, and JOIN all
 * the specified users while sending JOIN/MODEs to non-TS servers and
 * to clients
 */

#define INSERTSIGN(x,y) \
if (what != x) { \
*mbuf++=y; \
what = x; \
}

#define ADD_PARA(p) para = p; if(pbpos) parabuf[pbpos++] = ' '; while(*para) parabuf[pbpos++] = *para++;
#define ADD_SJBUF(p) para = p; if(sjbufpos) sjbuf[sjbufpos++] = ' '; while(*para) sjbuf[sjbufpos++] = *para++;

#define ADD_PARA2(p) para2 = p; if(pbpos2) ssjpara[pbpos2++] = ' '; while(*para2) ssjpara[pbpos2++] = *para2++;
#define ADD_SJBUF2(p) para2 = p; if(sjbufpos2) sjbuf2[sjbufpos2++] = ' '; while(*para2) sjbuf2[sjbufpos2++] = *para2++;

#define SJ_MODEPLUS(x, y) \
  if(((y) & mode.mode) && !((y) & oldmode->mode)) \
  { \
    INSERTSIGN(1, '+') \
    *mbuf++ = (x); \
  }

#define SJ_MODEMINUS(x, y) \
  if(((y) & oldmode->mode) && !((y) & mode.mode)) \
 { \
   INSERTSIGN(-1, '-') \
   *mbuf++ = (x); \
 }

#define SJ_MODEADD(x, y) case (x): mode.mode |= (y); break

int
m_sjoin (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p;
  aClient *target_p;
  ts_val newts, oldts, tstosend;
  static Mode mode, *oldmode;
  chanMember *cm;
  int args = 0, haveops = 0, keepourmodes = 1, keepnewmodes = 1,
    doesop = 0, what = 0, pargs = 0, pargs2 = 0, fl, people = 0,
    isnew, clientjoin = 0, pbpos, pbpos2, sjbufpos, sjbufpos2, created = 0;
  char *s, *s0, *s2, *para, *para2;
  static char numeric[16], sjbuf[BUFSIZE];
  char keep_modebuf[REALMODEBUFLEN], keep_parabuf[REALMODEBUFLEN];
  char *mbuf = modebuf, *p;
  char sjbuf2[BUFSIZE], ssjmode[BUFSIZE], ssjpara[BUFSIZE];

  /* if my client is SJOINing, it's just a local user being a dufus. 
   *  Ignore him.
   * parc >= 5 (new serv<->serv SJOIN format)
   * parc == 3 (new serv<->serv cliSJOIN format)
   */
  if (MyClient (source_p) || (parc < 5 && IsServer (source_p))
      || (parc < 3 && IsPerson (source_p)))
  {
    return 0;
  }

  if (parc == 3 && IsPerson (source_p))
  {
    clientjoin = 1;
  }
  else if (MyIsDigit (parv[2][0]))
  {
    int i;
    if (parc < 6)
    {
      return 0;
    }
    for (i = 2; i < (parc - 1); i++)
    {
      parv[i] = parv[i + 1];
    }
    parc--;
  }

  if (!IsChannelName (parv[2]))
  {
    return 0;
  }

  newts = atol (parv[1]);
  isnew = ChannelExists (parv[2]) ? 0 : 1;
  channel_p = get_channel (source_p, parv[2], CREATE, &created);
  oldts = channel_p->channelts;
  for (cm = channel_p->members; cm; cm = cm->next)
  {
    if (cm->flags & (MODE_CHANADMIN | MODE_CHANOP | MODE_HALFOP))
    {
      haveops++;
      break;
    }
  }

  if (clientjoin)               /* we have a good old (new :) client sjoin, with timestamp */
  {
    if (isnew)
    {
      channel_p->channelts = tstosend = newts;
    }
    else if (newts == 0 || oldts == 0)
    {
      channel_p->channelts = tstosend = 0;
    }
    else if (newts == oldts)
    {
      tstosend = oldts;
    }
    else if (newts < oldts)
    {
      if (haveops)
      {
        tstosend = oldts;
      }
      else
      {
        channel_p->channelts = tstosend = newts;
      }
    }
    else
    {
      tstosend = oldts;
    }

    /* parv[0] is the client that is joining. parv[0] == source_p->name */
    if (!IsMember (source_p, channel_p))
    {
      add_user_to_channel (channel_p, source_p, 0);
      sendto_channel_butserv (channel_p, source_p, ":%s JOIN :%s",
                              parv[0], parv[2]);
    }

    sendto_ssjoin5_servs (channel_p, client_p, newCliSJOINFmt, parv[0],
                          tstosend, parv[2]);

    sendto_pressjoin3_servs (channel_p, client_p, newCliSJOINFmt, parv[0],
                             tstosend, parv[2]);

    /* if the channel is created in client sjoin, we lost some channel modes. */
    if (created)
    {
      sendto_realops_lev (DEBUG_LEV, "Requesting resynch of %s from %s",
                          channel_p->chname, client_p->name);
      sendto_one (client_p, "RESYNCH %s", channel_p->chname);
    }

    return 0;
  }

  memset ((char *) &mode, '\0', sizeof (mode));
  s = parv[3];
  while (*s)
  {
    switch (*(s++))
    {
         SJ_MODEADD ('i', MODE_INVITEONLY);
         SJ_MODEADD ('n', MODE_NOPRIVMSGS);
         SJ_MODEADD ('p', MODE_PRIVATE);
         SJ_MODEADD ('s', MODE_SECRET);
         SJ_MODEADD ('m', MODE_MODERATED);
         SJ_MODEADD ('t', MODE_TOPICLIMIT);
         SJ_MODEADD ('r', MODE_REGISTERED);
         SJ_MODEADD ('R', MODE_REGONLY);
         SJ_MODEADD ('c', MODE_NOCOLOR);
         SJ_MODEADD ('O', MODE_OPERONLY);
         SJ_MODEADD ('N', MODE_NOINVITE);
         SJ_MODEADD ('K', MODE_NOKNOCK);
         SJ_MODEADD ('S', MODE_SECURE);
         SJ_MODEADD ('A', MODE_ADMINONLY);
         SJ_MODEADD ('M', MODE_MODREG);
         SJ_MODEADD ('q', MODE_NOQUITREASON);
       case 'k':
         strncpyzt (mode.key, parv[4 + args], KEYLEN + 1);
         args++;
         if (parc < 5 + args)
         {
           return 0;
         }
         break;
       case 'l':
         mode.limit = atoi (parv[4 + args]);
         args++;
         if (parc < 5 + args)
         {
           return 0;
         }
         break;
    }
  }

  oldmode = &channel_p->mode;
  doesop =
    ((parv[4 + args][0] == '!' || parv[4 + args][1] == '!'
      || parv[4 + args][2] == '!' || parv[4 + args][3] == '!')
     || (parv[4 + args][0] == '@' || parv[4 + args][1] == '@'
         || parv[4 + args][2] == '@' || parv[4 + args][3] == '@')
     || (parv[4 + args][0] == '%' || parv[4 + args][1] == '%'
         || parv[4 + args][2] == '%' || parv[4 + args][3] == '%'));

  if (isnew)
  {
    channel_p->channelts = tstosend = newts;
  }
  else if (newts == 0 || oldts == 0)
  {
    channel_p->channelts = tstosend = 0;
  }
  else if (newts == oldts)
  {
    tstosend = oldts;
  }
  else if (newts < oldts)
  {
#if 0
    kill_ban_list (source_p, channel_p);
    kill_exempt_list (source_p, channel_p);
#endif
    keepourmodes = 0;
    channel_p->channelts = tstosend = newts;
  }
  else
  {                             /* if our TS is older, and we have ops, don't keep their modes */
    keepnewmodes = 0;
    tstosend = oldts;
  }

  if (!keepnewmodes)
  {
    mode = *oldmode;
  }
  else if (keepourmodes)
  {
    mode.mode |= oldmode->mode;
    if (oldmode->limit > mode.limit)
    {
      mode.limit = oldmode->limit;
    }
    if (*oldmode->key && *mode.key && strcmp (mode.key, oldmode->key) > 0)
    {
      /* sketchy: keep the key that's lexographically greater
         if we both have a differing key. */
      strcpy (mode.key, oldmode->key);
    }
    else if (*oldmode->key && *mode.key == '\0')
    {
      strcpy (mode.key, oldmode->key);
    }
  }

  pbpos = 0;
  /*
   * since the most common case is that the modes are exactly the same,
   *  this if will skip over the most common case... :)
   * 
   * this would look prettier in a for loop, but it's unrolled here
   *  so it's a bit faster.   - lucas
   * 
   * pass +: go through and add new modes that are in mode and not oldmode
   * pass -: go through and delete old modes that are in oldmode and not mode
   */
  if (mode.mode != oldmode->mode)
  {
    /* plus modes */
    SJ_MODEPLUS ('p', MODE_PRIVATE);
    SJ_MODEPLUS ('s', MODE_SECRET);
    SJ_MODEPLUS ('m', MODE_MODERATED);
    SJ_MODEPLUS ('n', MODE_NOPRIVMSGS);
    SJ_MODEPLUS ('t', MODE_TOPICLIMIT);
    SJ_MODEPLUS ('i', MODE_INVITEONLY);
    SJ_MODEPLUS ('r', MODE_REGISTERED);
    SJ_MODEPLUS ('R', MODE_REGONLY);
    SJ_MODEPLUS ('c', MODE_NOCOLOR);
    SJ_MODEPLUS ('O', MODE_OPERONLY);
    SJ_MODEPLUS ('N', MODE_NOINVITE);
    SJ_MODEPLUS ('K', MODE_NOKNOCK);
    SJ_MODEPLUS ('S', MODE_SECURE);
    SJ_MODEPLUS ('A', MODE_ADMINONLY);
    SJ_MODEPLUS ('M', MODE_MODREG);
    SJ_MODEPLUS ('q', MODE_NOQUITREASON);

    /* minus modes */
    SJ_MODEMINUS ('p', MODE_PRIVATE);
    SJ_MODEMINUS ('s', MODE_SECRET);
    SJ_MODEMINUS ('m', MODE_MODERATED);
    SJ_MODEMINUS ('n', MODE_NOPRIVMSGS);
    SJ_MODEMINUS ('t', MODE_TOPICLIMIT);
    SJ_MODEMINUS ('i', MODE_INVITEONLY);
    SJ_MODEMINUS ('r', MODE_REGISTERED);
    SJ_MODEMINUS ('R', MODE_REGONLY);
    SJ_MODEMINUS ('c', MODE_NOCOLOR);
    SJ_MODEMINUS ('O', MODE_OPERONLY);
    SJ_MODEMINUS ('N', MODE_NOINVITE);
    SJ_MODEMINUS ('K', MODE_NOKNOCK);
    SJ_MODEMINUS ('S', MODE_SECURE);
    SJ_MODEMINUS ('A', MODE_ADMINONLY);
    SJ_MODEMINUS ('M', MODE_MODREG);
    SJ_MODEMINUS ('q', MODE_NOQUITREASON);
  }

  if (oldmode->limit && !mode.limit)
  {
    INSERTSIGN (-1, '-') * mbuf++ = 'l';
  }

  if (oldmode->key[0] && !mode.key[0])
  {
    INSERTSIGN (-1, '-') * mbuf++ = 'k';
    ADD_PARA (oldmode->key) pargs++;
  }

  if (mode.limit && oldmode->limit != mode.limit)
  {
    INSERTSIGN (1, '+') * mbuf++ = 'l';
    sprintf (numeric, "%-15d", mode.limit);
    if ((s = strchr (numeric, ' ')))
      *s = '\0';
    ADD_PARA (numeric);
    pargs++;
  }

  if (mode.key[0] && strcmp (oldmode->key, mode.key))
  {
    INSERTSIGN (1, '+') * mbuf++ = 'k';
    ADD_PARA (mode.key) pargs++;
  }

  channel_p->mode = mode;
  if (!keepourmodes)            /* deop and devoice everyone! */
  {
    what = 0;
    for (cm = channel_p->members; cm; cm = cm->next)
    {

      if (cm->flags & MODE_CHANADMIN)
      {
        INSERTSIGN (-1, '-') * mbuf++ = 'a';
        ADD_PARA (cm->client_p->name) pargs++;
        if (pargs >= MAXMODEPARAMS)
        {
          *mbuf = '\0';
          parabuf[pbpos] = '\0';
          sjoin_sendit (client_p, source_p, channel_p, parv[0]);
          mbuf = modebuf;
          *mbuf = '\0';
          pargs = pbpos = what = 0;
        }
        cm->flags &= ~MODE_CHANADMIN;
      }

      if (cm->flags & MODE_CHANOP)
      {
        INSERTSIGN (-1, '-') * mbuf++ = 'o';
        ADD_PARA (cm->client_p->name) pargs++;
        if (pargs >= MAXMODEPARAMS)
        {
          *mbuf = '\0';
          parabuf[pbpos] = '\0';
          sjoin_sendit (client_p, source_p, channel_p, parv[0]);
          mbuf = modebuf;
          *mbuf = '\0';
          pargs = pbpos = what = 0;
        }
        cm->flags &= ~MODE_CHANOP;
      }

      if (cm->flags & MODE_HALFOP)
      {
        INSERTSIGN (-1, '-') * mbuf++ = 'h';
        ADD_PARA (cm->client_p->name) pargs++;
        if (pargs >= MAXMODEPARAMS)
        {
          *mbuf = '\0';
          parabuf[pbpos] = '\0';
          sjoin_sendit (client_p, source_p, channel_p, parv[0]);
          mbuf = modebuf;
          *mbuf = '\0';
          pargs = pbpos = what = 0;
        }
        cm->flags &= ~MODE_HALFOP;
      }

      if (cm->flags & MODE_VOICE)
      {
        INSERTSIGN (-1, '-') * mbuf++ = 'v';
        ADD_PARA (cm->client_p->name) pargs++;
        if (pargs >= MAXMODEPARAMS)
        {
          *mbuf = '\0';
          parabuf[pbpos] = '\0';
          sjoin_sendit (client_p, source_p, channel_p, parv[0]);
          mbuf = modebuf;
          *mbuf = '\0';
          pargs = pbpos = what = 0;
        }
        cm->flags &= ~MODE_VOICE;
      }
    }
    if (channel_p->topic[0])
    {
      channel_p->topic[0] = '\0';
      sendto_channel_butserv (channel_p, source_p, ":%s TOPIC %s :%s",
                              me.name, channel_p->chname, channel_p->topic);
    }
    sendto_channel_butserv (channel_p, &me,
                            ":%s NOTICE %s :*** Notice -- TS for %s changed from %ld to %ld",
                            me.name, channel_p->chname, channel_p->chname,
                            oldts, newts);
  }

  if (mbuf != modebuf)
  {
    *mbuf = '\0';
    parabuf[pbpos] = '\0';
    sjoin_sendit (client_p, source_p, channel_p, parv[0]);
  }

  *modebuf = '\0';
  parabuf[0] = '\0';
  if (parv[3][0] != '0' && keepnewmodes)
    channel_modes (source_p, modebuf, parabuf, channel_p);
  else
  {
    modebuf[0] = '0';
    modebuf[1] = '\0';
  }

  /* We do this down below now, so we can send out for two sjoin formats.      
   * sprintf(t, ":%s SJOIN %ld %ld %s %s %s :", parv[0], tstosend, tstosend,
   *                     parv[2], modebuf, parabuf);
   * t += strlen(t);
   * the pointer "t" has been removed and is now replaced with an 
   * index into sjbuf for faster appending
   *
   * With the SSJ3 we now do 3 sjoin formats to maintain compactibility
   * with non SSJ3 severs like services *sigh* - ShadowMaster
   */

  strcpy (keep_modebuf, modebuf);
  strcpy (keep_parabuf, parabuf);
  sjbufpos = 0;
  sjbufpos2 = 0;
  mbuf = modebuf;
  pbpos = 0;
  pbpos2 = 0;
  pargs = 0;
  pargs2 = 0;
  *mbuf++ = '+';
  *ssjmode = '+';
  ssjmode[1] = '\0';
  for (s = s0 = s2 = strtoken (&p, parv[args + 4], " "); s;
       s = s0 = s2 = strtoken (&p, (char *) NULL, " "))
  {
    fl = 0;
    if (*s == '!' || s[1] == '!' || s[2] == '!' || s[3] == '!')
      fl |= MODE_CHANADMIN;
    if (*s == '%' || s[1] == '%' || s[2] == '%' || s[3] == '%')
      fl |= MODE_HALFOP;
    if (*s == '@' || s[1] == '@' || s[2] == '@' || s[3] == '@')
      fl |= MODE_CHANOP;
    if (*s == '+' || s[1] == '+' || s[2] == '+' || s[3] == '+')
      fl |= MODE_VOICE;
    if (!keepnewmodes)
    {
      if (fl & MODE_CHANADMIN)
        fl = MODE_DEADMINED;
      if (fl & MODE_HALFOP)
        fl = MODE_DEHALFOPPED;
      if (fl & MODE_CHANOP)
        fl = MODE_DEOPPED;
      else
        fl = 0;
    }


    while (*s == '!' || *s == '%' || *s == '@' || *s == '+')
      s++;
    if (!(target_p = find_chasing (source_p, s, NULL)))
      continue;
    if (target_p->from != client_p)
      continue;
    people++;
    if (!IsMember (target_p, channel_p))
    {
      add_user_to_channel (channel_p, target_p, fl);
      sendto_channel_butserv (channel_p, target_p, ":%s JOIN :%s", s,
                              parv[2]);
    }


    while (*s2 == '!' || *s2 == '%')
      s2++;

    if (keepnewmodes)
    {
      ADD_SJBUF (s0);
      ADD_SJBUF2 (s0);
    }
    else
    {
      ADD_SJBUF (s);
      ADD_SJBUF2 (s2);
    }

    if (fl & MODE_CHANADMIN)
    {
      ADD_PARA2 (s) pargs2++;
      strcat (ssjmode, "a");
      *mbuf++ = 'a';
      ADD_PARA (s) pargs++;
      if (pargs >= MAXMODEPARAMS)
      {
        *mbuf = '\0';
        parabuf[pbpos] = '\0';
        sjoin_sendit (client_p, source_p, channel_p, parv[0]);
        mbuf = modebuf;
        *mbuf++ = '+';
        pargs = pbpos = 0;
      }
    }

    if (fl & MODE_HALFOP)
    {
      ADD_PARA2 (s) pargs2++;
      strcat (ssjmode, "h");
      *mbuf++ = 'h';
      ADD_PARA (s) pargs++;
      if (pargs >= MAXMODEPARAMS)
      {
        *mbuf = '\0';
        parabuf[pbpos] = '\0';
        sjoin_sendit (client_p, source_p, channel_p, parv[0]);
        mbuf = modebuf;
        *mbuf++ = '+';
        pargs = pbpos = 0;
      }
    }

    if (fl & MODE_CHANOP)
    {
      *mbuf++ = 'o';
      ADD_PARA (s) pargs++;
      if (pargs >= MAXMODEPARAMS)
      {
        *mbuf = '\0';
        parabuf[pbpos] = '\0';
        sjoin_sendit (client_p, source_p, channel_p, parv[0]);
        mbuf = modebuf;
        *mbuf++ = '+';
        pargs = pbpos = 0;
      }
    }


    if (fl & MODE_VOICE)
    {
      *mbuf++ = 'v';
      ADD_PARA (s) pargs++;
      if (pargs >= MAXMODEPARAMS)
      {
        *mbuf = '\0';
        parabuf[pbpos] = '\0';
        sjoin_sendit (client_p, source_p, channel_p, parv[0]);
        mbuf = modebuf;
        *mbuf++ = '+';
        pargs = pbpos = 0;
      }
    }
  }


  parabuf[pbpos] = '\0';
  ssjpara[pbpos2] = '\0';
  *mbuf = '\0';
  if (pargs)
    sjoin_sendit (client_p, source_p, channel_p, parv[0]);
  if (people)
  {
    sjbuf[sjbufpos] = '\0';
    sjbuf2[sjbufpos2] = '\0';
    if (keep_parabuf[0] != '\0')
    {
      sendto_ssjoin5_servs (channel_p, client_p, newSJOINFmt, parv[0],
                            tstosend, parv[2], keep_modebuf, keep_parabuf,
                            sjbuf);

      sendto_pressjoin3_servs (channel_p, client_p, newSJOINFmt, parv[0],
                               tstosend, parv[2], keep_modebuf,
                               keep_parabuf, sjbuf2);
      /*
       *** FIXME ***
       *
       * This is starting to get ugly. This is mostly intended for
       * backwards services compactibility, so unless services starts supporting TSMODE
       * its safe to leave. We mite need to write yet another sendto_pressjoin_tsmode_servs (ugly)
       * to deal with both possibilities however.
       */
      sendto_pressjoin3_servs (channel_p, client_p,
                               ":%s MODE %s %s %s", me.name, parv[2],
                               ssjmode, ssjpara);
    }
    else
    {
      sendto_ssjoin5_servs (channel_p, client_p, newSJOINFmtNP, parv[0],
                            tstosend, parv[2], keep_modebuf, sjbuf);

      sendto_pressjoin3_servs (channel_p, client_p, newSJOINFmtNP,
                               parv[0], tstosend, parv[2], keep_modebuf,
                               sjbuf2);
      /*
       *** FIXME ***
       *
       * This is starting to get ugly. This is mostly intended for
       * backwards services compactibility, so unless services starts supporting TSMODE
       * its safe to leave. We mite need to write yet another sendto_pressjoin_tsmode_servs (ugly)
       * to deal with both possibilities however.
       */
      sendto_pressjoin3_servs (channel_p, client_p,
                               ":%s MODE %s %s %s", me.name, parv[2],
                               ssjmode, ssjpara);
    }
  }
  else if (created && channel_p->users == 0)
  {
    /* I created the channel, but there were no people! */
    sub1_from_channel (channel_p);
  }
  return 0;
}

#undef INSERTSIGN
#undef ADD_PARA
#undef ADD_SJBUF


char *
pretty_mask (char *mask)
{
  char *cp, *user, *host;
  if ((user = strchr ((cp = mask), '!')))
    *user++ = '\0';
  if ((host = strrchr (user ? user : cp, '@')))
  {
    *host++ = '\0';
    if (!user)
      return make_nick_user_host (NULL, cp, host);
  }
  else if (!user && (strchr (cp, '.') || strchr (cp, ':')))
    return make_nick_user_host (NULL, NULL, cp);
  return make_nick_user_host (cp, user, host);
}

/*
** m_knock
**
** Based on m_knock by Stskeeps and Codemastr
**
** Rewritten for UltimateIRCd by ShadowMaster
**
**  parv[0] - sender prefix
**  parv[1] - channel
**  parv[2] - reason
*/
int
m_knock (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p;
  int chanlen = 0;
  char *name;
  if (!(source_p->user))
    return 0;                   /* Something is wrong. Bail out */
  if (parc < 3 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "KNOCK");
    return 0;
  }

  name = parv[1];
  if (!check_channelname (source_p, (unsigned char *) name))
    return 0;
  chanlen = strlen (name);
  if (chanlen > CHANNELLEN)
  {
    name[CHANNELLEN] = '\0';
    chanlen = CHANNELLEN;
  }

  if (!(channel_p = find_channel (parv[1], NullChn)))
  {
    sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL), me.name, parv[0],
                name);
    return 0;
  }

  if (IsMember (source_p, channel_p) == 1)
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: You are already in channel %s",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  if (is_banned (source_p, channel_p))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: You are banned from %s",
                me.name, parv[0], channel_p->chname);
    return 0;
  }


  if (channel_p->mode.mode & MODE_REGONLY && !IsRegNick (source_p))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: You need a Registered nickname to join %s",
                me.name, parv[0], channel_p->chname);
/*      return (ERR_NEEDREGGEDNICK);*/
    return 0;
  }

  if (channel_p->mode.mode & MODE_SECURE && !IsSSLClient (source_p))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: You need to be a secure client (SSL) to join %s",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  if (channel_p->mode.mode & MODE_ADMINONLY && !IsOper (source_p))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: Channel %s is Admin Only (+A)",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  if (channel_p->mode.mode & MODE_OPERONLY && !IsOper (source_p))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: Channel %s is Oper Only (+O)",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  if (channel_p->mode.mode & MODE_NOKNOCK)
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: Channel %s is does not allow KNOCK's (+K)",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  if (!(channel_p->mode.mode & MODE_INVITEONLY))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: Channel %s is not Invite Only (+i)",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  if (channel_p->mode.mode & MODE_NOINVITE)
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [KNOCK] -- Error: Channel %s does not allow invites (+N)",
                me.name, parv[0], channel_p->chname);
    return 0;
  }

  sendto_allchannelops_butone (NULL, &me, channel_p,
                               ":%s NOTICE !@%%%s :[To %s Staff] -- %s (%s@%s) knocked on the channel [%s]",
                               me.name, channel_p->chname, channel_p->chname,
                               parv[0], source_p->user->username,
                               (IsHidden (source_p) ? source_p->user->
                                virthost : source_p->user->host), parv[2]);
  sendto_one (source_p, ":%s NOTICE %s :*** [KNOCK] -- Knocked on to %s",
              me.name, source_p->name, channel_p->chname);
  return 0;
}

/*
 * m_chanfix()
 *
 * Copyright (C) 2003, Thomas. J. Stens� and ShadowRealm Creations
 *
 * parv[0] - sender
 * parv[1] - channel
 * parv[2] - TS (remote only)
 *
 * Allows Administrators to forcibly reset the timestamp on a given channel.
 */
int
m_chanfix (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aChannel *channel_p;
  ts_val newts, oldts;

  /*
   * Administrators only
   */
  if (!IsSkoAdmin (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
    return 0;
  }

  /*
   * Make sure we have enough parameters
   * 1 for local calls
   * 2 for remote
   */

  if (MyClient (source_p))
  {
    if ((parc < 2) || (*parv[1] == '\0'))
    {
      sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                  me.name, parv[0], "CHANFIX");
      return 0;
    }
  }
  else
  {
    if ((parc < 3) || (*parv[1] == '\0'))
    {
      sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                  me.name, parv[0], "CHANFIX");
      return 0;
    }
  }

  /*
   * Make sure the channel given is valid
   */
  channel_p = get_channel (source_p, parv[1], 0, NULL);
  if (!channel_p)
  {
    sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                me.name, parv[0], parv[1]);
    return 0;
  }

  /*
   * Set the new timestamp
   */
  if (MyClient (source_p))
  {
    newts = NOW;
  }
  else
  {
    newts = atol (parv[2]);
  }

  oldts = channel_p->channelts;
  channel_p->channelts = newts;

  /*
   * Tell the admin and inform the channel.
   */
  if (MyClient (source_p))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** \2[CHANFIX]\2 -- Forced TS change on %s. Old: %ld New: %ld",
                me.name, parv[0], parv[1], oldts, newts);
  }


  /*
   * Tell the channel.
   */
  sendto_channel_butserv (channel_p, &me,
                          ":%s NOTICE %s :*** Notice -- TS for %s changed from %ld to %ld",
                          me.name, channel_p->chname, channel_p->chname,
                          oldts, newts);

  /*
   * Tell the local opers.
   * Each server creates its own notice to opers as its wastefull to send
   * the notice across the network as the information needed to generate it
   * is passed on in the CHANFIX command.
   */
  sendto_netinfo
    ("from %s: \2[CHANFIX]\2 %s forced a TS change on %s. Old: %ld New: %ld",
     me.name, parv[0], parv[1], oldts, newts);

  /*
   * Propogate
   */
  sendto_match_servs (channel_p, client_p, ":%s CHANFIX %s %ld", parv[0],
                      parv[1], newts);

  return 0;
}
