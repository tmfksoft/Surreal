/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/send.c
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
 *  $Id: send.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include <stdio.h>
#include "numeric.h"
#include "dh.h"
#include "zlink.h"
#include "va_copy.h"
#include "fds.h"



/*
 * STOP_SENDING_ON_SHORT_SEND:
 * Treat a short send as a blocked socket
 */
#define STOP_SENDING_ON_SHORT_SEND

#ifdef ALWAYS_SEND_DURING_SPLIT
extern int currently_processing_netsplit;
#endif

static char sendbuf[2048];
static char remotebuf[2048];
static int send_message (aClient *, char *, int);

#ifdef HAVE_ENCRYPTION_ON
/*
 * WARNING:
 * Please be aware that if you are using both encryption
 * and ziplinks, rc4buf in send.c MUST be the same size
 * as zipOutBuf in zlink.c!
 */
static char rc4buf[16384];
#endif

static int sentalong[MAXCONNECTIONS];
static int sent_serial;
#ifdef UNUSED
#ifdef FLUD
static void sendto_channel_butlocal (aClient *, aClient *, aChannel *, char *,
                                     ...);
#endif
#endif


void
init_send ()
{
  memset (sentalong, 0, sizeof (int) * MAXCONNECTIONS);
  sent_serial = 0;
}

/* This routine increments our serial number so it will
 * be unique from anything in sentalong, no need for a memset
 * except for every MAXINT calls - lucas
 */

/* This should work on any OS where an int is 32 bit, I hope.. */

#define HIGHEST_SERIAL INT_MAX

#define INC_SERIAL if(sent_serial == HIGHEST_SERIAL) \
   { memset(sentalong, 0, sizeof(sentalong)); sent_serial = 0; } \
   sent_serial++;


/*
 * dead_link
 *
 * somewhere along the lines of sending out, there was an error.
 * we can't close it from the send loop, so mark it as dead
 * and close it from the main loop.
 *
 * if this link is a server, tell routing people.
 */

static int
dead_link (aClient * to, char *notice, int sockerr)
{

  int errtmp = errno;           /* so we don't munge this later */

  to->sockerr = sockerr;
  to->flags |= FLAGS_DEADSOCKET;
  /*
   * If because of BUFFERPOOL problem then clean dbuf's now so that
   * notices don't hurt operators below.
   */
  DBufClear (&to->recvQ);
  DBufClear (&to->sendQ);
  /* Ok, if the link we're dropping is a server, send a routing
   * notice..
   */
  if (IsServer (to) && !(to->flags & FLAGS_CLOSING))
  {
    char fbuf[512];

    ircsprintf (fbuf, "from %s: %s", me.name, notice);
    sendto_gnotice (fbuf, get_client_name (to, HIDEME), strerror (errtmp));
    ircsprintf (fbuf, ":%s GNOTICE :%s", me.name, notice);
    sendto_serv_butone (to, fbuf, get_client_name (to, HIDEME),
                        strerror (errtmp));
  }

  return -1;
}

/*
 * * send_message *   Internal utility which delivers one message
 * buffer to the *      socket. Takes care of the error handling and
 * buffering, if *      needed.
 */
static int
send_message (aClient * to, char *msg, int len)
{
  static int SQinK;

#ifdef DUMP_DEBUG
  fprintf (dumpfp, "-> %s: %s\n", (to->name ? to->name : "*"), msg);
#endif

  if (to->from)
    to = to->from;              /* shouldn't be necessary */

  if (IsServer (to) || IsNegoServer (to))
  {
    if (len > 510)
    {
      msg[511] = '\n';
      msg[512] = '\0';
      len = 512;
    }
    else
    {
      msg[len] = '\n';
      msg[len + 1] = '\0';
      len++;
    }
  }
  else
  {
    if (len > 509)
    {
      msg[510] = '\r';
      msg[511] = '\n';
      msg[512] = '\0';
      len = 512;
    }
    else
    {
      msg[len] = '\r';
      msg[len + 1] = '\n';
      msg[len + 2] = '\0';
      len += 2;
    }
  }

  if (IsMe (to))
  {
    sendto_realops ("Trying to send to myself! [%s]", msg);
    return 0;
  }

  if (IsDead (to))
    return 0;
  if (DBufLength (&to->sendQ) > to->sendqlen)
  {
    /* this would be a duplicate notice, but it contains some useful information that
       would be spamming the rest of the network. Kept in. - lucas */
    if (IsServer (to))
      sendto_realops ("Max SendQ limit exceeded for %s: %d > %d",
                      get_client_name (to, HIDEME), DBufLength (&to->sendQ),
                      get_sendq (to));
    to->flags |= FLAGS_SENDQEX;
    return dead_link (to, "Max Sendq exceeded for %s, closing link", 0);
  }

  /*
   * * Update statistics. The following is slightly incorrect *
   * because it counts messages even if queued, but bytes * only
   * really sent. Queued bytes get updated in SendQueued.
   */
  to->sendM += 1;
  me.sendM += 1;
  if (to->acpt != &me)
  {
    to->acpt->sendM += 1;
  }

  if (ZipOut (to))
  {
    int ldata = (to->flags & FLAGS_BURST);

    msg = zip_output (to->serv->zip_out, msg, &len, 0, &ldata);
    if (len == -1)
    {
      sendto_realops ("Zipout error for %s: (%d) %s\n", to->name, ldata, msg);
      return dead_link (to, "Zip output error for %s", IRCERR_ZIP);
    }

    if (len == 0)
      return 0;
  }

#ifdef HAVE_ENCRYPTION_ON
  if (IsRC4OUT (to))
  {
    /* don't destroy the data in 'msg' */
    rc4_process_stream_to_buf (to->serv->rc4_out, msg, rc4buf, len);
    msg = rc4buf;
  }
#endif

  if (dbuf_put (&to->sendQ, msg, len) < 0)
    return dead_link (to, "Buffer allocation error for %s, closing link",
                      IRCERR_BUFALLOC);

  /*
   * * This little bit is to stop the sendQ from growing too large
   * when * there is no need for it to. Thus we call send_queued()
   * every time * 2k has been added to the queue since the last
   * non-fatal write. * Also stops us from deliberately building a
   * large sendQ and then * trying to flood that link with data
   * (possible during the net * relinking done by servers with a large
   * load).
   */
  /*
   * Well, let's try every 4k for clients, and immediately for servers
   * -Taner
   */
  /*
   * Let's not waste time trying this on anyone who has a blocking socket.
   * Also, let's send every 8k for servers, since there's lots of traffic
   * there and we'd like to make it more efficient. - lucas
   */

  if (to->flags & FLAGS_BLOCKED)
    return 0;

#ifdef ALWAYS_SEND_DURING_SPLIT
  if (currently_processing_netsplit)
  {
    send_queued (to);
    return 0;
  }
#endif

  SQinK = (DBufLength (&to->sendQ) >> 10);
  if (IsServer (to))
  {
    if (SQinK > (to->lastsq + 8))
      send_queued (to);
  }
  else
  {
    if (SQinK > (to->lastsq + 4))
      send_queued (to);
  }
  return 0;
}


/*
 * * send_queued *    This function is called from the main
 * select-loop (or whatever) *  when there is a chance the some output
 * would be possible. This *    attempts to empty the send queue as far
 * as possible...
 */
int
send_queued (aClient * to)
{
  char *msg;
  int len, rlen;
  int more_data = 0;            /* the hybrid approach.. */

  /*
   * * Once socket is marked dead, we cannot start writing to it, *
   * even if the error is removed...
   */
  if (IsDead (to))
  {
    /*
     * * Actually, we should *NEVER* get here--something is * not
     * working correct if send_queued is called for a * dead
     * socket... --msa
     */
    return -1;
  }

  if (ZipOut (to) && zip_is_data_out (to->serv->zip_out))
  {
    if (DBufLength (&to->sendQ))
      more_data = 1;
    else
    {
      int ldata = (to->flags & FLAGS_BURST);

      msg = zip_output (to->serv->zip_out, NULL, &len, 1, &ldata);
      if (len == -1)
      {
        sendto_realops ("Zipout error for %s: (%d) %s\n", to->name,
                        ldata, msg);
        return dead_link (to, "Zip output error for %s", IRCERR_ZIP);
      }

#ifdef HAVE_ENCRYPTION_ON
      if (IsRC4OUT (to))
        rc4_process_stream (to->serv->rc4_out, msg, len);
#endif
      /* silently stick this on the sendq... */
      if (!dbuf_put (&to->sendQ, msg, len))
        return dead_link (to, "Buffer allocation error for %s",
                          IRCERR_BUFALLOC);
    }
  }

  while (DBufLength (&to->sendQ) > 0)
  {
    msg = dbuf_map (&to->sendQ, (size_t *) & len);

    if ((rlen = deliver_it (to, msg, len)) < 0)
    {
      return dead_link (to, "Write error to %s, closing link (%s)", errno);
    }
    dbuf_delete (&to->sendQ, rlen);
    to->lastsq = (DBufLength (&to->sendQ) >> 10);

#ifdef STOP_SENDING_ON_SHORT_SEND
    if (rlen < len)
    {
      /* Treat this socket as blocking */
      to->flags |= FLAGS_BLOCKED;
      set_fd_flags (to->fd, FDF_WANTWRITE);
      break;
    }
#else
    if (rlen == 0)
    {
      /* Socket is blocking... */
      break;
    }

#endif
    if (more_data && DBufLength (&to->sendQ) == 0)
    {
      int ldata = (to->flags & FLAGS_BURST);

      more_data = 0;

      msg = zip_output (to->serv->zip_out, NULL, &len, 1, &ldata);
      if (len == -1)
      {
        sendto_realops ("Zipout error for %s: (%d) %s\n", to->name,
                        ldata, msg);
        return dead_link (to, "Zip output error for %s", IRCERR_ZIP);
      }

#ifdef HAVE_ENCRYPTION_ON
      if (IsRC4OUT (to))
        rc4_process_stream (to->serv->rc4_out, msg, len);
#endif
      /* silently stick this on the sendq... */
      if (!dbuf_put (&to->sendQ, msg, len))
        return dead_link (to, "Buffer allocation error for %s",
                          IRCERR_BUFALLOC);
    }
  }

  if ((to->flags & FLAGS_SOBSENT) && IsBurst (to)
      && DBufLength (&to->sendQ) < 20480)
  {                             /* 20k */
    if (!(to->flags & FLAGS_BURST))
    {
      to->flags &= (~FLAGS_SOBSENT);
      sendto_one (to, "BURST %d", DBufLength (&to->sendQ));
      /*
       * Tell all servers on the other side of the link that we have completed bursting.
       */
/*	  sendto_one (to, "EOBURST");*/
      if (!(to->flags & FLAGS_EOBRECV))
      {                         /* hey we're the last to synch.. */
#ifdef HTM_LOCK_ON_NETBURST
        HTMLOCK = NO;
#endif
      }
    }
  }

  return (IsDead (to)) ? -1 : 0;
}

/*
 * * send message to single client
 */
void
sendto_one (aClient * to, char *pattern, ...)
{
  va_list vl;
  int len;                      /* used for the length of the current message */

  va_start (vl, pattern);
  len = ircvsprintf (sendbuf, pattern, vl);

  if (to->from)
    to = to->from;
  if (IsMe (to))
  {
    sendto_realops ("Trying to send [%s] to myself!", sendbuf);
    return;
  }
  send_message (to, sendbuf, len);
  va_end (vl);
}

void
vsendto_one (aClient * to, char *pattern, va_list vl)
{
  int len;                      /* used for the length of the current message */

  len = ircvsprintf (sendbuf, pattern, vl);

  if (to->from)
    to = to->from;
  if (IsMe (to) && to->fd >= 0)
  {
    sendto_realops ("Trying to send [%s] to myself!", sendbuf);
    return;
  }
  send_message (to, sendbuf, len);
}

/* prefix_buffer
 *
 * take varargs and dump prefixed message into a buffer
 * remote: 1 if client is remote, 0 if local
 * from: the client sending the message
 * prefix: the prefix as specified (parv[0] usually)
 * buffer: the buffer to dump this into (NO BOUNDS CHECKING!)
 * pattern: varargs pattern
 * vl: varargs variable list with one arg taken already
 */
static inline int
prefix_buffer (int remote, aClient * from, char *prefix, char *buffer,
               char *pattern, va_list vl)
{
  char *p;                      /* temp pointer */
  int msglen;                   /* the length of the message we end up with */
  int sidx = 1;                 /* start at offset 1 */
  *buffer = ':';

  if (!remote && IsPerson (from))
  {
    int flag = 0;
    anUser *user = from->user;

    for (p = from->name; *p; p++)
      buffer[sidx++] = *p;

    if (user)
    {
      if (*user->username)
      {
        buffer[sidx++] = '!';
        for (p = user->username; *p; p++)
          buffer[sidx++] = *p;
      }
      if (*user->host && !MyConnect (from))
      {
        buffer[sidx++] = '@';
        for (p = (IsHidden (from) ? user->virthost : user->host); *p; p++)
          buffer[sidx++] = *p;
        flag = 1;
      }
    }

    if (!flag && MyConnect (from) && *user->host)
    {
      buffer[sidx++] = '@';
      for (p = (IsHidden (from) ? from->user->virthost : from->sockhost);
           *p; p++)
        buffer[sidx++] = *p;
    }
  }
  else
  {
    for (p = prefix; *p; p++)
      buffer[sidx++] = *p;
  }

  msglen = ircvsprintf (&buffer[sidx], pattern + 3, vl);
  msglen += sidx;

  return msglen;
}

static inline int
check_fake_direction (aClient * from, aClient * to)
{
  if (!MyClient (from) && IsPerson (to) && (to->from == from->from))
  {
    if (IsServer (from))
    {
      sendto_realops
        ("Message to %s[%s] dropped from %s (Fake Direction)", to->name,
         to->from->name, from->name);
      return -1;
    }

    sendto_realops ("Ghosted: %s[%s@%s] from %s[%s@%s] (%s)", to->name,
                    to->user->username, to->user->host, from->name,
                    from->user->username, from->user->host, to->from->name);
    sendto_serv_butone (NULL, ":%s KILL %s :%s (%s[%s@%s] Ghosted %s)",
                        me.name, to->name, me.name, to->name,
                        to->user->username, to->user->host, to->from->name);

    to->flags |= FLAGS_KILLED;
    exit_client (NULL, to, &me, "Ghosted client");

    if (IsPerson (from))
      sendto_one (from, err_str (ERR_GHOSTEDCLIENT), me.name, from->name,
                  to->name, to->user->username, to->user->host, to->from);
    return -1;
  }

  return 0;
}


void
sendto_channel_butone (aClient * one, aClient * from, aChannel * channel_p,
                       char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  int didlocal = 0, didremote = 0;
  va_list vl;
  char *pfix;

  va_start (vl, pattern);

  pfix = va_arg (vl, char *);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one)
    {
      continue;                 /* Skip one */
    }
    if (IsDeaf (target_p))
    {
      continue;                 /* Skip deaf clients */
    }
    i = target_p->from->fd;
    if (MyClient (target_p))
    {
      if (!didlocal)
      {
        didlocal = prefix_buffer (0, from, pfix, sendbuf, pattern, vl);
      }
      if (check_fake_direction (from, target_p))
      {
        continue;
      }
      send_message (target_p, sendbuf, didlocal);
      sentalong[i] = sent_serial;
    }
    else
    {
      /*
       * Now check whether a message has been sent to this remote
       * link already
       */
      if (!didremote)
      {
        didremote = prefix_buffer (1, from, pfix, remotebuf, pattern, vl);
      }
      if (check_fake_direction (from, target_p))
      {
        continue;
      }
      if (sentalong[i] != sent_serial)
      {
        send_message (target_p, remotebuf, didremote);
        sentalong[i] = sent_serial;
      }
    }
  }

  va_end (vl);
  return;
}

/*
 * sendto_server_butone
 * 
 * Send a message to all connected servers except the client 'one'.
 */
void
sendto_serv_butone (aClient * one, char *pattern, ...)
{
  aClient *client_p;
  int k = 0;
  fdlist send_fdlist;
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (one && client_p == one->from)
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}

/*
 * sendto_noquit_servs_butone
 * 
 * Send a message to all noquit servs if noquit = 1,
 * or all non-noquit servs if noquit = 0
 * we omit "one", too.
 */
void
sendto_noquit_servs_butone (int noquit, aClient * one, char *pattern, ...)
{
  aClient *client_p;
  int k = 0;
  fdlist send_fdlist;
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if ((noquit && !IsNoQuit (client_p)) ||
        (!noquit && IsNoQuit (client_p)) || one == client_p)
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}

/*
 * sendto_clientcapab_servs_butone
 *
 * Send a message to all client capable servers if clientcapab = 1,
 * or all non-client capable servs if clientcapab = 0
 * we omit "one", too.
 */
void
sendto_clientcapab_servs_butone (int clientcapab, aClient * one,
                                 char *pattern, ...)
{
  aClient *client_p;
  int k = 0;
  fdlist send_fdlist;
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if ((clientcapab && !IsClientCapable (client_p)) ||
        (!clientcapab && IsClientCapable (client_p)) || one == client_p)
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}


/*
 * sendto_common_channels()
 * 
 * Sends a message to all people (inclusing user) on local server who are
 * in same channel with user.
 */
void
sendto_common_channels (aClient * from, char *pattern, ...)
{
  Link *channels;
  chanMember *users;
  aClient *client_p;
  va_list vl;
  char *pfix;
  int msglen = 0;

  va_start (vl, pattern);

  pfix = va_arg (vl, char *);

  INC_SERIAL if (from->fd >= 0)
      sentalong[from->fd] = sent_serial;

  if (from->user)
  {
    for (channels = from->user->channel; channels; channels = channels->next)
    {
      for (users = channels->value.channel_p->members; users;
           users = users->next)
      {
        client_p = users->client_p;

        if (!MyConnect (client_p) || sentalong[client_p->fd] == sent_serial)
          continue;

        sentalong[client_p->fd] = sent_serial;
        if (!msglen)
          msglen = prefix_buffer (0, from, pfix, sendbuf, pattern, vl);
        if (check_fake_direction (from, client_p))
          continue;
        send_message (client_p, sendbuf, msglen);
      }
    }
  }

  if (MyConnect (from))
  {
    if (!msglen)
      msglen = prefix_buffer (0, from, pfix, sendbuf, pattern, vl);
    send_message (from, sendbuf, msglen);
  }

  va_end (vl);
  return;
}

/*
 * send_quit_to_common_channels()
 * 
 * Sends a message to all people (inclusing user) on local server who are
 * in same channel with user if the user can send to this channel.
 *
 */
void
send_quit_to_common_channels (aClient * from, char *reason)
{
  Link *channels;
  chanMember *users;
  aClient *client_p;
  int msglen;
  INC_SERIAL
    msglen = sprintf (sendbuf, ":%s!%s@%s QUIT :%s", from->name,
                      from->user->username,
                      (IsHidden (from) ? from->user->virthost : from->user->
                       host), reason);

  if (from->fd >= 0)
    sentalong[from->fd] = sent_serial;
  for (channels = from->user->channel; channels; channels = channels->next)
  {
    if (!NoQuitReasonChan (channels->value.channel_p))
    {
      if (can_send (from, channels->value.channel_p, reason) == 0)
      {
        for (users = channels->value.channel_p->members;
             users; users = users->next)
        {
          client_p = users->client_p;

          if (!MyConnect (client_p) || sentalong[client_p->fd] == sent_serial)
            continue;

          sentalong[client_p->fd] = sent_serial;
          if (check_fake_direction (from, client_p))
            continue;
          send_message (client_p, sendbuf, msglen);
        }
      }
    }
  }
  return;
}

/*
 * send_part_to_common_channels()
 * 
 * Sends a message to all people (inclusing user) on local server who are
 * in same channel with user if the user cannot send to the channel.
 */
void
send_part_to_common_channels (aClient * from, char *reason)
{
  Link *channels;
  chanMember *users;
  aClient *client_p;
  int msglen = 0;

  for (channels = from->user->channel; channels; channels = channels->next)
  {
    if ((can_send (from, channels->value.channel_p, reason) != 0)
        || (NoQuitReasonChan (channels->value.channel_p)))
    {
      msglen = sprintf (sendbuf, ":%s!%s@%s PART %s",
                        from->name, from->user->username,
                        (IsHidden (from) ? from->user->
                         virthost : from->user->host),
                        channels->value.channel_p->chname);

      INC_SERIAL if (from->fd >= 0)
          sentalong[from->fd] = sent_serial;

      for (users = channels->value.channel_p->members; users;
           users = users->next)
      {
        client_p = users->client_p;

        if (!MyConnect (client_p) || sentalong[client_p->fd] == sent_serial)
          continue;

        sentalong[client_p->fd] = sent_serial;
        if (check_fake_direction (from, client_p))
          continue;
        send_message (client_p, sendbuf, msglen);
      }
    }
  }
  return;
}

#ifdef UNUSED
/* this function isn't called from anywhere
** Fish (23/08/03)
*/
#ifdef FLUD
void
sendto_channel_butlocal (aClient * one, aClient * from, aChannel * channel_p,
                         char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one)
      continue;                 /* ...was the one I should skip */
    i = target_p->from->fd;
    if (!MyFludConnect (target_p))
    {
      /*
       * Now check whether a message has been sent to this remote
       * link already
       */
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);
        sentalong[i] = sent_serial;
      }
    }
  }
  va_end (vl);
  return;
}
#endif /* FLUD */
#endif
/*
 * sendto_channel_butserv
 * 
 * Send a message to all members of a channel that are connected to this
 * server.
 */
void
sendto_channel_butserv (aChannel * channel_p, aClient * from, char *pattern,
                        ...)
{
  chanMember *cm;
  aClient *target_p;
  va_list vl;
  int didlocal = 0;
  char *pfix;

  va_start (vl, pattern);

  pfix = va_arg (vl, char *);

  for (cm = channel_p->members; cm; cm = cm->next)
  {
    if (MyConnect (target_p = cm->client_p))
    {
      if (!didlocal)
        didlocal = prefix_buffer (0, from, pfix, sendbuf, pattern, vl);

      if (check_fake_direction (from, target_p))
        continue;

      send_message (target_p, sendbuf, didlocal);

      /* vsendto_prefix_one(target_p, from, pattern, vl); */
    }
  }
  va_end (vl);
  return;
}


/*
 * sendto_oldssjoin_servs
 *
 * send to all servers with pre bahamut SSJOIN
 *
 */
void
sendto_oldssjoin_servs (aChannel * channel_p, aClient * from, char *pattern,
                        ...)
{
  int k = 0;
  fdlist send_fdlist;

  aClient *client_p;
  va_list vl;
  DLink *lp;

  if (channel_p)
  {
    if (*channel_p->chname == '&')
      return;
  }
  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if ((client_p == from) || (IsSSJoin (client_p)))
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}

/*
 * sendto_ssjoin5_servs
 *
 * send to all servers with ssjoin5 capability (or not)
 *
 */
void
sendto_ssjoin5_servs (aChannel * channel_p, aClient * from, char *pattern,
                      ...)
{
  int k = 0;
  fdlist send_fdlist;
  aClient *client_p;
  va_list vl;
  DLink *lp;

  if (channel_p)
  {
    if (*channel_p->chname == '&')
      return;
  }
  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if ((client_p == from) || (!IsSSJoin5 (client_p)))
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}


/*
 * sendto_pressjoin3_servs
 *
 * send to all servers that dont have ssjoin3, ssjoin4 or ssjoin5 capability
 *
 */
void
sendto_pressjoin3_servs (aChannel * channel_p, aClient * from, char *pattern,
                         ...)
{
  int k = 0;
  fdlist send_fdlist;
  aClient *client_p;
  va_list vl;
  DLink *lp;

  if (channel_p)
  {
    if (*channel_p->chname == '&')
      return;
  }
  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if ((client_p == from) || (!IsSSJoin (client_p)))
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}

/*
 * sendto_tsmode_servs
  *
  * send to all servers with tsmode capability (or not)
  *
  */
void
sendto_tsmode_servs (int tsmode, aChannel * channel_p, aClient * from,
                     char *pattern, ...)
{
  int k = 0;
  fdlist send_fdlist;
  aClient *client_p;
  va_list vl;
  DLink *lp;

  if (channel_p)
  {
    if (*channel_p->chname == '&')
      return;
  }
  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if ((client_p == from) ||
        (tsmode && !IsTSMODE (client_p)) || (!tsmode && IsTSMODE (client_p)))
      continue;

    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}


/*
 * * send a msg to all ppl on servers/hosts that match a specified mask *
 * (used for enhanced PRIVMSGs) *
 * 
 * addition -- Armin, 8jun90 (gruner@informatik.tu-muenchen.de)
 */

static int
match_it (aClient * one, char *mask, int what)
{
  if (what == MATCH_HOST)
    return (match (mask, one->user->host) == 0);
  else
    return (match (mask, one->user->server) == 0);
}

/*
 * sendto_match_servs
 * 
 * send to all servers which match the mask at the end of a channel name
 * (if there is a mask present) or to all if no mask.
 */
void
sendto_match_servs (aChannel * channel_p, aClient * from, char *pattern, ...)
{
  int k = 0;
  fdlist send_fdlist;
  aClient *client_p;
  va_list vl;
  DLink *lp;

  if (channel_p)
  {
    if (*channel_p->chname == '&')
      return;
  }
  va_start (vl, pattern);
  for (lp = server_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (client_p == from)
      continue;
    send_fdlist.entry[++k] = client_p->fd;
  }
  send_fdlist.last_entry = k;
  if (k)
    vsendto_fdlist (&send_fdlist, pattern, vl);
  va_end (vl);
  return;
}

/*
 * sendto_match_butone
 * 
 * Send to all clients which match the mask in a way defined on 'what';
 * either by user hostname or user servername.
 */
void
sendto_match_butone (aClient * one, aClient * from, char *mask, int what,
                     char *pattern, ...)
{
  int i;
  aClient *client_p, *target_p;
  char cansendlocal, cansendglobal;
  va_list vl;

  va_start (vl, pattern);
  if (MyConnect (from))
  {
    cansendlocal = (OPCanLNotice (from)) ? 1 : 0;
    cansendglobal = (OPCanGNotice (from)) ? 1 : 0;
  }
  else
    cansendlocal = cansendglobal = 1;
  for (i = 0; i <= highest_fd; i++)
  {
    if (!(client_p = local[i]))
      continue;                 /* that clients are not mine */
    if (client_p == one)        /* must skip the origin !! */
      continue;
    if (IsServer (client_p))
    {
      if (!cansendglobal)
        continue;
      for (target_p = client; target_p; target_p = target_p->next)
        if (IsRegisteredUser (target_p)
            && match_it (target_p, mask, what) && target_p->from == client_p)
          break;
      /*
       * a person on that server matches the mask, so we * send *one*
       * msg to that server ...
       */
      if (target_p == NULL)
        continue;
      /*
       * ... but only if there *IS* a matching person 
       */
    }
    /*
     * my client, does he match ? 
     */
    else if (!cansendlocal || !(IsRegisteredUser (client_p) &&
                                match_it (client_p, mask, what)))
      continue;
    vsendto_prefix_one (client_p, from, pattern, vl);
  }
  va_end (vl);
  return;
}

/*
 * sendto_all_butone.
 * 
 * Send a message to all connections except 'one'. The basic wall type
 * message generator.
 */
void
sendto_all_butone (aClient * one, aClient * from, char *pattern, ...)
{
  int i;
  aClient *client_p;
  va_list vl;

  va_start (vl, pattern);
  for (i = 0; i <= highest_fd; i++)
    if ((client_p = local[i]) && !IsMe (client_p) && one != client_p)
      vsendto_prefix_one (client_p, from, pattern, vl);
  va_end (vl);
  return;
}


/*
 * sendto_realops_lev
 * 
 * Send to *local* ops only
 */
void
sendto_realops_lev (int lev, char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  char tmpbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && !IsMe (client_p))
    {
      switch (lev)
      {
         case CCONN_LEV:
           if (!SendConnectNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Connect-Notice");
           break;
         case REJ_LEV:
           if (!SendRejectNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Reject-Notice");
           break;
         case SKILL_LEV:
           if (!SendSkillNotice (client_p))
             continue;
           ircsprintf (tmpbuf, "ServerKill-Notice");
           break;
         case SPY_LEV:
           if (!SendSpyNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Spy-Notice");
           break;
         case DCCSEND_LEV:
           if (!SendDCCNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "DCCSend-Notice");
           break;
         case FLOOD_LEV:
           if (!SendFloodNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Flood-Notice");
           break;
         case SPAM_LEV:
           if (!SendSpamNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Spam-Notice");
           break;
         case DEBUG_LEV:
           if (!SendDebugNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Debug-Notice");
           break;

         default:              /* this is stupid, but oh well */
           if (!SendServerNotice (client_p) || !IsAnOper (client_p))
             continue;
           ircsprintf (tmpbuf, "Realops-Notice");
      }
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2%s\2 -- ",
                  me.name, client_p->name, tmpbuf);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

/*
 * sendto_ops
 * 
 * send to all +s users on this server only.
 */
void
sendto_ops (char *pattern, ...)
{
  aClient *client_p;
  int i;
  char nbuf[1024];
  va_list vl;

  va_start (vl, pattern);
  for (i = 0; i <= highest_fd; i++)
    if ((client_p = local[i]) && !IsServer (client_p) && !IsMe (client_p) &&
        SendServerNotice (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** Server-Notice -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  va_end (vl);
  return;
}

/*
 * * sendto_ops_butone *      Send message to all operators. * one -
 * client not to send message to * from- client which message is from
 * *NEVER* NULL!!
 */
void
sendto_ops_butone (aClient * one, aClient * from, char *pattern, ...)
{
  int i;
  aClient *client_p;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (client_p = client; client_p; client_p = client_p->next)
  {
    if (!SendWallops (client_p))
      continue;
    /*
     * we want wallops if (MyClient(client_p) && !(IsServer(from) ||
     * IsMe(from))) continue;
     */
    i = client_p->from->fd;     /*
                                 * find connection oper is on 
                                 */
    if (sentalong[i] == sent_serial)    /*
                                         * sent message along it already ? 
                                         */
      continue;
    if (client_p->from == one)
      continue;                 /*
                                 * ...was the one I should skip 
                                 */
    sentalong[i] = sent_serial;
    vsendto_prefix_one (client_p->from, from, pattern, vl);
  }
  va_end (vl);
  return;
}

/*
 * * sendto_wallops_butone *      Send message to all operators. * one
 * - client not to send message to * from- client which message is from
 * *NEVER* NULL!!
 */
void
sendto_wallops_butone (aClient * one, aClient * from, char *pattern, ...)
{
  int i;
  aClient *client_p;
  va_list vl;

  va_start (vl, pattern);
  for (i = 0; i <= highest_fd; i++)
  {
    if ((client_p = local[i]) != NULL)
    {
      if (!
          (IsRegistered (client_p)
           && (SendWallops (client_p) || IsServer (client_p)))
          || client_p == one)
        continue;
      vsendto_prefix_one (client_p, from, pattern, vl);
    }
  }
  va_end (vl);
  return;
}

void
send_globops (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsAnOper (client_p)
        && !IsMe (client_p) && SendGlobops (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2Global\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
send_chatops (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsAnOper (client_p)
        && !IsMe (client_p) && SendChatops (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2ChatOps\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
sendto_netinfo (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsAnOper (client_p)
        && !IsMe (client_p) && SendNetInfo (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2Network-Info\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
sendto_netglobal (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsAnOper (client_p) && !IsMe (client_p)
        && SendNetGlobal (client_p))
    {
      ircsprintf (nbuf,
                  ":%s NOTICE %s :*** \2Network-Global\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
sendto_adminchat (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsSkoAdmin (client_p) && !IsMe (client_p)
        && SendChatops (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2Admin-Chat\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
sendto_cschat (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p)
        && (IsSkoServicesStaff (client_p) || IsNetAdmin (client_p)
            || IsNetCoAdmin (client_p)) && !IsMe (client_p)
        && SendChatops (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2CS-Chat\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}


void
sendto_connectnotice (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsAnOper (client_p)
        && SendConnectNotice (client_p) && !IsMe (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2Connect/Exit\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
sendto_globalconnectnotice (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && IsAnOper (client_p)
        && SendGConnectNotice (client_p) && !IsMe (client_p))
    {
      ircsprintf (nbuf,
                  ":%s NOTICE %s :*** \2Global-Connect/Exit\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}


/*
 * to - destination client from - client which message is from
 * 
 * NOTE: NEITHER OF THESE SHOULD *EVER* BE NULL!! -avalon
 */
void
sendto_prefix_one (aClient * to, aClient * from, char *pattern, ...)
{
  static char sender[HOSTLEN + NICKLEN + USERLEN + 5];
  static char temp[1024];
  anUser *user;
  char *idx;
  char *par;
  int flag = 0, sidx = 0;
  va_list vl, vl2;

  va_start (vl, pattern);
  VA_COPY (vl2, vl);


  par = va_arg (vl, char *);
  /*
   * Optimize by checking if (from && to) before everything 
   * uhh, there's _always_ going to be a to!
   */
  if (from)
  {
    if (!MyClient (from) && IsPerson (to) && (to->from == from->from))
    {
      if (IsServer (from))
      {
        ircvsprintf (temp, pattern, vl2);
        sendto_realops
          ("Send message (%s) to %s[%s] dropped from %s(Fake Dir)",
           temp, to->name, to->from->name, from->name);
        va_end (vl);
        return;
      }

      sendto_realops ("Ghosted: %s[%s@%s] from %s[%s@%s] (%s)", to->name,
                      to->user->username, to->user->host, from->name,
                      from->user->username, from->user->host, to->from->name);
      sendto_serv_butone (NULL, ":%s KILL %s :%s (%s[%s@%s] Ghosted %s)",
                          me.name, to->name, me.name, to->name,
                          to->user->username, to->user->host, to->from->name);

      to->flags |= FLAGS_KILLED;
      exit_client (NULL, to, &me, "Ghosted client");
      if (IsPerson (from))
        sendto_one (from, err_str (ERR_GHOSTEDCLIENT), me.name,
                    from->name, to->name, to->user->username,
                    (IsHidden (to) ? to->user->virthost : to->user->host),
                    to->from);
      va_end (vl);
      return;
    }

    if (MyClient (to) && IsPerson (from) && (irccmp (par, from->name) == 0))
    {
      user = from->user;

      for (idx = from->name; *idx; idx++)
        sender[sidx++] = *idx;

      if (user)
      {
        if (*user->username)
        {
          sender[sidx++] = '!';
          for (idx = user->username; *idx; idx++)
            sender[sidx++] = *idx;
        }
        if (*user->host && !MyConnect (from))
        {
          sender[sidx++] = '@';
          for (idx = (IsHidden (from) ? user->virthost : user->host);
               *idx; idx++)
            sender[sidx++] = *idx;
          flag = 1;
        }
      }

      /*
       * flag is used instead of index(sender, '@') for speed and
       * also since username/nick may have had a '@' in them.
       * -avalon
       */

      if (!flag && MyConnect (from) && *user->host)
      {
        sender[sidx++] = '@';
        for (idx =
             (IsHidden (from) ? from->user->virthost : from->sockhost);
             *idx; idx++)
          sender[sidx++] = *idx;
      }

      sender[sidx] = '\0';
      par = sender;

    }
  }

  temp[0] = ':';
  sidx = 1;

  /* okay, we more or less know that our sendto_prefix crap is going to be :%s <blah>,
   * so it's easy to fix these lame problems...joy */

  for (idx = par; *idx; idx++)
    temp[sidx++] = *idx;
  for (idx = (pattern + 3); *idx; idx++)
    temp[sidx++] = *idx;

  temp[sidx] = '\0';

  vsendto_one (to, temp, vl);
  va_end (vl);
}

/* this is an incredibly expensive function. 
 * removed all strcat() calls. - lucas */

void
vsendto_prefix_one (aClient * to, aClient * from, char *pattern, va_list vl)
{
  static char sender[HOSTLEN + NICKLEN + USERLEN + 5];
  static char temp[1024];
  anUser *user;
  char *idx;
  char *par;
  int flag = 0, sidx = 0;
  va_list vl2;
  VA_COPY (vl2, vl);


  par = va_arg (vl2, char *);
  /*
   * Optimize by checking if (from && to) before everything 
   * uhh, there's _always_ going to be a to!
   */
  if (from)
  {
    if (!MyClient (from) && IsPerson (to) && (to->from == from->from))
    {
      if (IsServer (from))
      {
        ircvsprintf (temp, pattern, vl);
        sendto_realops
          ("Send message (%s) to %s[%s] dropped from %s(Fake Dir)",
           temp, to->name, to->from->name, from->name);
        return;
      }

      sendto_realops ("Ghosted: %s[%s@%s] from %s[%s@%s] (%s)", to->name,
                      to->user->username, to->user->host, from->name,
                      from->user->username, from->user->host, to->from->name);
      sendto_serv_butone (NULL, ":%s KILL %s :%s (%s[%s@%s] Ghosted %s)",
                          me.name, to->name, me.name, to->name,
                          to->user->username, to->user->host, to->from->name);

      to->flags |= FLAGS_KILLED;
      exit_client (NULL, to, &me, "Ghosted client");
      if (IsPerson (from))
        sendto_one (from, err_str (ERR_GHOSTEDCLIENT), me.name,
                    from->name, to->name, to->user->username,
                    to->user->host, to->from);
      return;
    }

    if (MyClient (to) && IsPerson (from) && (irccmp (par, from->name) == 0))
    {
      user = from->user;

      for (idx = from->name; *idx; idx++)
        sender[sidx++] = *idx;

      if (user)
      {
        if (*user->username)
        {
          sender[sidx++] = '!';
          for (idx = user->username; *idx; idx++)
            sender[sidx++] = *idx;
        }
        if (*user->host && !MyConnect (from))
        {
          sender[sidx++] = '@';
          for (idx = (IsHidden (from) ? user->virthost : user->host);
               *idx; idx++)
            sender[sidx++] = *idx;
          flag = 1;
        }
      }

      /*
       * flag is used instead of index(sender, '@') for speed and
       * also since username/nick may have had a '@' in them.
       * -avalon
       */

      if (!flag && MyConnect (from) && *user->host)
      {
        sender[sidx++] = '@';
        for (idx =
             (IsHidden (from) ? from->user->virthost : from->sockhost);
             *idx; idx++)
          sender[sidx++] = *idx;
      }

      sender[sidx] = '\0';
      par = sender;

    }
  }

  temp[0] = ':';
  sidx = 1;

  /* okay, we more or less know that our sendto_prefix crap is going to be :%s <blah>,
   * so it's easy to fix these lame problems...joy */

  for (idx = par; *idx; idx++)
    temp[sidx++] = *idx;
  for (idx = (pattern + 3); *idx; idx++)
    temp[sidx++] = *idx;

  temp[sidx] = '\0';

  vsendto_one (to, temp, vl2);
}

void
sendto_fdlist (fdlist * listp, char *pattern, ...)
{
  int len, j, fd;
  va_list vl;

  va_start (vl, pattern);
  len = ircvsprintf (sendbuf, pattern, vl);

  for (fd = listp->entry[j = 1]; j <= listp->last_entry;
       fd = listp->entry[++j])
    send_message (local[fd], sendbuf, len);
  va_end (vl);
}

void
vsendto_fdlist (fdlist * listp, char *pattern, va_list vl)
{
  int len, j, fd;
  len = ircvsprintf (sendbuf, pattern, vl);

  for (fd = listp->entry[j = 1]; j <= listp->last_entry;
       fd = listp->entry[++j])
    send_message (local[fd], sendbuf, len);
}


/*
 * sendto_realops
 * 
 * Send to *local* ops only but NOT +s nonopers.
 * If it's to local ops only and not +s nonopers, then SendServNotice is
 * wrong. Changed to IsAnOper. -mjs
 */
void
sendto_realops (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (IsAnOper (client_p))
    {
      ircsprintf (nbuf,
                  ":%s NOTICE %s :*** \2Realops-Notice\2 -- %s",
                  me.name, client_p->name, pattern);
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

void
vsendto_realops (char *pattern, va_list vl)
{
  aClient *client_p;
  char nbuf[1024];
  DLink *lp;

  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (IsAnOper (client_p))
    {
      ircsprintf (nbuf,
                  ":%s NOTICE %s :*** \2Server Notice\2 -- %s",
                  me.name, client_p->name, pattern);
      vsendto_one (client_p, nbuf, vl);
    }
  }
  return;
}

/*
 * * ts_warn *      Call sendto_realops, with some flood checking (at most
 * 5 warnings *      every 5 seconds)
 */

void
ts_warn (char *pattern, ...)
{
  static ts_val last = 0;
  static int warnings = 0;
  ts_val now;
  va_list vl;

  va_start (vl, pattern);
  /*
   * * if we're running with TS_WARNINGS enabled and someone does *
   * something silly like (remotely) connecting a nonTS server, *
   * we'll get a ton of warnings, so we make sure we don't send * more
   * than 5 every 5 seconds.  -orabidoo
   */
  /*
   * th+hybrid servers always do TS_WARNINGS -Dianora
   */
  now = time (NULL);
  if (now - last < 5)
  {
    if (++warnings > 5)
      return;
  }
  else
  {
    last = now;
    warnings = 0;
  }

  vsendto_realops (pattern, vl);
  va_end (vl);
  return;
}

/*
 * sendto_locops
 */
void
sendto_locops (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (SendGlobops (client_p))
    {
      ircsprintf (nbuf, ":%s NOTICE %s :*** \2LocOps\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }
  va_end (vl);
  return;
}

/*
 * sendto_gnotice - send a routing notice to all local +n users.
 */
void
sendto_gnotice (char *pattern, ...)
{
  aClient *client_p;
  char nbuf[1024];
  va_list vl;
  DLink *lp;

  va_start (vl, pattern);
  for (lp = oper_list; lp; lp = lp->next)
  {
    client_p = lp->value.client_p;

    if (!IsServer (client_p) && !IsMe (client_p)
        && SendRoutingNotice (client_p))
    {

      ircsprintf (nbuf, ":%s NOTICE %s :*** \2Routing\2 -- ",
                  me.name, client_p->name);
      strncat (nbuf, pattern, sizeof (nbuf) - strlen (nbuf));
      vsendto_one (client_p, nbuf, vl);
    }
  }

  vsnprintf (nbuf, 1024, pattern, vl);
  ilog (LOGF_ROUTING, "%s", nbuf);

  va_end (vl);
  return;
}

/*
 * sendto_allchannelops_butone
 *   Send a message to all Channel Admins, ChanOps and HalfOps in channel channel_p that
 *   are directly on this server and sends the message
 *   on to the next server if it has any aswell.
 */
void
sendto_allchannelops_butone (aClient * one, aClient * from,
                             aChannel * channel_p, char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one
        || !((cm->flags & CHFL_CHANADMIN) || (cm->flags & CHFL_CHANOP)
             || (cm->flags & CHFL_HALFOP)))
      continue;
    i = target_p->from->fd;
    if (MyConnect (target_p) && IsRegisteredUser (target_p))
    {
      vsendto_prefix_one (target_p, from, pattern, vl);
      sentalong[i] = sent_serial;
    }
    else
    {
      /*
       * Now check whether a message has been sent to this
       * *      * remote link already 
       */
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);

        sentalong[i] = sent_serial;
      }
    }
  }
  va_end (vl);
  return;
}


/*
 * sendto_channeladmins_butone
 *   Send a message to all Channel Admins in channel channel_p that
 *   are directly on this server and sends the message
 *   on to the next server if it has any Channel Admins.
 */
void
sendto_channeladmins_butone (aClient * one, aClient * from,
                             aChannel * channel_p, char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one || !(cm->flags & CHFL_CHANADMIN))
      continue;
    i = target_p->from->fd;
    if (MyConnect (target_p) && IsRegisteredUser (target_p))
    {
      vsendto_prefix_one (target_p, from, pattern, vl);
      sentalong[i] = sent_serial;
    }
    else
    {
      /*
       * Now check whether a message has been sent to this
       * *      * remote link already 
       */
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);

        sentalong[i] = sent_serial;
      }
    }
  }
  va_end (vl);
  return;
}

/*
 * sendto_channelops_butone
 *   Send a message to all OPs in channel channel_p that
 *   are directly on this server and sends the message
 *   on to the next server if it has any OPs.
 */
void
sendto_channelops_butone (aClient * one, aClient * from, aChannel * channel_p,
                          char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one || !(cm->flags & CHFL_CHANOP))
      continue;
    i = target_p->from->fd;
    if (MyConnect (target_p) && IsRegisteredUser (target_p))
    {
      vsendto_prefix_one (target_p, from, pattern, vl);
      sentalong[i] = sent_serial;
    }
    else
    {
      /*
       * Now check whether a message has been sent to this
       * *      * remote link already 
       */
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);

        sentalong[i] = sent_serial;
      }
    }
  }
  va_end (vl);
  return;
}

/*
 * sendto_channelhalfops_butone
 *   Send a message to all HalfOps in channel channel_p that
 *   are directly on this server and sends the message
 *   on to the next server if it has any HalfOps.
 */
void
sendto_channelhalfops_butone (aClient * one, aClient * from,
                              aChannel * channel_p, char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one || !(cm->flags & CHFL_HALFOP))
      continue;
    i = target_p->from->fd;
    if (MyConnect (target_p) && IsRegisteredUser (target_p))
    {
      vsendto_prefix_one (target_p, from, pattern, vl);
      sentalong[i] = sent_serial;
    }
    else
    {
      /*
       * Now check whether a message has been sent to this
       * *      * remote link already 
       */
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);

        sentalong[i] = sent_serial;
      }
    }
  }
  va_end (vl);
  return;
}

/*
 * sendto_channelvoice_butone
 *   Send a message to all voiced users in channel channel_p that
 *   are directly on this server and sends the message
 *   on to the next server if it has any voiced users.
 */
void
sendto_channelvoice_butone (aClient * one, aClient * from,
                            aChannel * channel_p, char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one || !(cm->flags & CHFL_VOICE))
      continue;
    i = target_p->from->fd;
    if (MyConnect (target_p) && IsRegisteredUser (target_p))
    {
      vsendto_prefix_one (target_p, from, pattern, vl);
      sentalong[i] = sent_serial;
    }
    else
    {
      /*
       * Now check whether a message has been sent to this
       * *      * remote link already 
       */
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);
        sentalong[i] = sent_serial;
      }
    }
  }
  va_end (vl);
  return;
}

/*
 * sendto_channelvoiceops_butone
 *   Send a message to all OPs or voiced users in channel channel_p that
 *   are directly on this server and sends the message
 *   on to the next server if it has any OPs or voiced users.
 */
void
sendto_channelvoiceops_butone (aClient * one, aClient * from, aChannel
                               * channel_p, char *pattern, ...)
{
  chanMember *cm;
  aClient *target_p;
  int i;
  va_list vl;

  va_start (vl, pattern);

  INC_SERIAL for (cm = channel_p->members; cm; cm = cm->next)
  {
    target_p = cm->client_p;
    if (target_p->from == one
        || !((cm->flags & CHFL_VOICE) || (cm->flags & CHFL_CHANOP)))
      continue;
    i = target_p->from->fd;
    if (MyConnect (target_p) && IsRegisteredUser (target_p))
    {
      vsendto_prefix_one (target_p, from, pattern, vl);
      sentalong[i] = sent_serial;
    }
    else                        /* remote link */
    {
      if (sentalong[i] != sent_serial)
      {
        vsendto_prefix_one (target_p, from, pattern, vl);
        sentalong[i] = sent_serial;
      }
    }
  }
  return;
}

/*******************************************
 * Flushing functions (empty queues)
 *******************************************/

/*
 * flush_connections
 * Empty only buffers for clients without FLAGS_BLOCKED
 * dump_connections 
 * Unintelligently try to empty all buffers.
 */
void
flush_connections (int fd)
{
  int i;
  aClient *client_p;

  if (fd == me.fd)
  {
    for (i = highest_fd; i >= 0; i--)
    {
      if (!(client_p = local[i]))
      {
        continue;
      }

      if (!(client_p->flags & FLAGS_BLOCKED) &&
          (DBufLength (&client_p->sendQ) > 0 ||
           (ZipOut (client_p) && zip_is_data_out (client_p->serv->zip_out))))
      {
        send_queued (client_p);
      }
    }
  }
  else if (fd >= 0 && (client_p = local[fd])
           &&
           !(client_p->flags & FLAGS_BLOCKED) &&
           (DBufLength (&client_p->sendQ) > 0 ||
            (ZipOut (client_p) && zip_is_data_out (client_p->serv->zip_out))))
  {
    send_queued (client_p);
  }
}


void
dump_connections (int fd)
{
  int i;
  aClient *client_p;

  if (fd == me.fd)
  {
    for (i = highest_fd; i >= 0; i--)
    {
      if ((client_p = local[i]) &&
          (DBufLength (&client_p->sendQ) > 0 ||
           (ZipOut (client_p) && zip_is_data_out (client_p->serv->zip_out))))
      {
        send_queued (client_p);
      }
    }
  }
  else if (fd >= 0 && (client_p = local[fd]) &&
           (DBufLength (&client_p->sendQ) > 0 ||
            (ZipOut (client_p) && zip_is_data_out (client_p->serv->zip_out))))
  {
    send_queued (client_p);
  }
}

/* flush an fdlist intelligently */
void
flush_fdlist_connections (fdlist * listp)
{
  int i, fd;
  aClient *client_p;

  for (fd = listp->entry[i = 1]; i <= listp->last_entry;
       fd = listp->entry[++i])
  {
    if ((client_p = local[fd]) && !(client_p->flags & FLAGS_BLOCKED)
        &&
        (DBufLength (&client_p->sendQ) > 0 ||
         (ZipOut (client_p) && zip_is_data_out (client_p->serv->zip_out))))
    {
      send_queued (client_p);
    }
  }
}
