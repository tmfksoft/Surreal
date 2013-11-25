/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/socketengine_kqueue.c
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
 *  $Id: socketengine_kqueue.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "config.h"

#ifdef USE_KQUEUE

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "fds.h"
#include "sock.h"
#include <sys/event.h>
#include <sys/time.h>

#define MAX_EVENT_QUEUE 64

static int kqueue_id = -1;
static struct kevent eventQs[2][MAX_EVENT_QUEUE + 1];
static struct kevent *eventQ = eventQs[0];
static int eventQi = 0;
static int numEvents = 0;

static void
kevent_add (struct kevent *e)
{
  if (kqueue_id == -1)
    abort ();

  if (numEvents == MAX_EVENT_QUEUE)
  {
    if (kevent (kqueue_id, eventQ, numEvents, NULL, 0, NULL) < 0)
      sendto_realops_lev (DEBUG_LEV, "kevent() returned error: %s",
                          strerror (errno));
    numEvents = 0;
  }
  memcpy (&eventQ[numEvents++], e, sizeof (struct kevent));
}

void
engine_init ()
{
  kqueue_id = kqueue ();
  numEvents = 0;
}

void
engine_add_fd (int fd)
{
  struct kevent e;

  e.ident = fd;
  e.filter = EVFILT_READ;
  e.flags = EV_ADD | EV_DISABLE;
  e.fflags = 0;
  e.data = 0;
  e.udata = NULL;
  kevent_add (&e);

  e.ident = fd;
  e.filter = EVFILT_WRITE;
  e.flags = EV_ADD | EV_DISABLE;
  e.fflags = 0;
  e.data = 0;
  e.udata = NULL;
  kevent_add (&e);

  set_fd_internal (fd, 0);
}

void
engine_del_fd (int fd)
{
  struct kevent e;
  int i, j;

  e.ident = fd;
  e.filter = EVFILT_READ;
  e.flags = EV_DELETE;
  e.fflags = 0;
  e.data = 0;
  e.udata = NULL;
  kevent_add (&e);

  e.ident = fd;
  e.filter = EVFILT_WRITE;
  e.flags = EV_DELETE;
  e.fflags = 0;
  e.data = 0;
  e.udata = NULL;
  kevent_add (&e);

  /* But we should remove this fd from the change queue -- if it was closed  
   * and we have a change pending, kevent() will fail later.  What's worse   
   * is that when the queue is flushed due to being full, a kevent() failure 
   * may leave some changes unprocessed.  Reordering the change queue is not 
   * safe, hence the gymnastics below.    -Quension  
   */

  if (!numEvents)
    return;

  /* optimal case: fd isn't in the change queue */
  for (i = 0; i < numEvents; i++)
    if (eventQ[i].ident == fd)
      break;

  /* second optimal case: fd is last, truncate the queue */
  if (i == numEvents - 1)
    numEvents--;

  if (i == numEvents)
    return;

  /* swap array index, copy all fds before this one */
  eventQi ^= 1;
  memcpy (eventQs[eventQi], eventQ, sizeof (struct kevent) * i);

  /* selectively copy remaining fds, skip bad one */
  for (j = i++; i < numEvents; i++)
    if (eventQ[i].ident != fd)
      memcpy (&eventQs[eventQi][j++], &eventQ[i], sizeof (struct kevent));

  /* swap active array */
  numEvents = j;
  eventQ = eventQs[eventQi];

}

void
engine_change_fd_state (int fd, unsigned int stateplus)
{
  unsigned int oldflags = (unsigned int) get_fd_internal (fd);
  struct kevent e;

  /* Something changed with our read state? */
  if ((oldflags ^ stateplus) & FDF_WANTREAD)
  {
    e.ident = fd;
    e.filter = EVFILT_READ;
    e.flags = EV_ADD | ((stateplus & FDF_WANTREAD) ? EV_ENABLE : EV_DISABLE);
    e.fflags = 0;
    e.data = 0;
    e.udata = 0;
    kevent_add (&e);
  }

  /* Something changed with our write state? */
  if ((oldflags ^ stateplus) & FDF_WANTWRITE)
  {
    e.ident = fd;
    e.filter = EVFILT_WRITE;
    e.flags = EV_ADD | ((stateplus & FDF_WANTWRITE) ? EV_ENABLE : EV_DISABLE);
    e.fflags = 0;
    e.data = 0;
    e.udata = 0;
    kevent_add (&e);
  }

  set_fd_internal (fd, (void *) stateplus);
}

#define ENGINE_MAX_EVENTS 512
#define ENGINE_MAX_LOOPS (2 * (MAXCONNECTIONS / 512))

int
engine_read_message (time_t delay)
{
  static struct kevent events[ENGINE_MAX_EVENTS];

  int nevs, length, i, numloops, eventsfull;
  unsigned int fdflags;
  int fdtype;
  void *fdvalue;
  aClient *client_p;
  struct timespec wait;

  numloops = 0;
  wait.tv_sec = delay;
  wait.tv_nsec = 0;

  do
  {
    nevs =
      kevent (kqueue_id, eventQ, numEvents, events, ENGINE_MAX_EVENTS, &wait);
    numEvents = 0;

    if (nevs == 0)
      return 0;

    if (nevs == -1)
    {
      if ((errno == EINTR) || (errno == EAGAIN))
        return -1;

      report_error ("kevent %s:%s", &me);
      sleep (5);
      return -1;
    }

    eventsfull = (nevs == ENGINE_MAX_EVENTS) ? 1 : 0;
    if (delay || numloops)
      NOW = timeofday = time (NULL);
    numloops++;

    for (i = 0; i < nevs; i++)
    {
      int rr = 0, rw = 0;

      if (events[i].flags & EV_ERROR)
      {
        errno = events[i].data;
        /* this should be handled later i suppose */
        continue;
      }

      get_fd_info (events[i].ident, &fdtype, &fdflags, &fdvalue);

      if (events[i].filter == EVFILT_READ)
        rr = 1;
      else if (events[i].filter == EVFILT_WRITE)
        rw = 1;

      client_p = NULL;
      length = -1;

      switch (fdtype)
      {
         case FDT_NONE:
           break;

         case FDT_AUTH:
           client_p = (aClient *) fdvalue;
           if (rr)
             read_authports (client_p);
           else if (rw && client_p->authfd >= 0)
             send_authports (client_p);
           check_client_fd (client_p);
           break;

         case FDT_LISTENER:
           client_p = (aClient *) fdvalue;
           if (rr)
             accept_connection (client_p);
           break;

         case FDT_RESOLVER:
           do_dns_async ();
           break;

         case FDT_CLIENT:
           client_p = (aClient *) fdvalue;
           readwrite_client (client_p, rr, rw);
           break;

         default:
           abort ();            /* unknown client type? bail! */
      }
    }
  }
  while (eventsfull && (numloops < ENGINE_MAX_LOOPS));

  return 0;
}
#endif /* USE_KQUEUE */
