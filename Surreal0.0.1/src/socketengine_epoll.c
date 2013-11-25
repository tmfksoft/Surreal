/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/socketengine_epoll.c
 *
 *  Copyright (C) 2004 David Parton
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
 *  $Id: socketengine_epoll.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


#include "config.h"

#ifdef USE_EPOLL

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "fds.h"
#include "sock.h"
#include <stdint.h>
#include <errno.h>
#include <sys/epoll.h>

#ifdef NEED_EPOLL_DEFS
#include <asm/unistd.h>

_syscall1 (int, epoll_create, int, size)
_syscall4 (int, epoll_ctl, int, epfd, int, op, int, fd, struct epoll_event *,
           event)
_syscall4 (int, epoll_wait, int, epfd, struct epoll_event *, pevents, int,
           maxevents, int, timeout)
#endif
     static int epoll_id = -1, numfds = 0;
     static struct epoll_fd
     {
       int fd;
       unsigned int events;
     } epoll_fds[MAXCONNECTIONS];


     void engine_init ()
{
  epoll_id = epoll_create (MAXCONNECTIONS);
  memset (epoll_fds, 0, sizeof (epoll_fds));
}

void
engine_add_fd (int fd)
{
  struct epoll_event ev;

  if (numfds >= MAXCONNECTIONS)
    abort ();

  ev.events = 0;
  ev.data.ptr = &epoll_fds[numfds];
  if (epoll_ctl (epoll_id, EPOLL_CTL_ADD, fd, &ev) < 0)
    abort ();

  epoll_fds[numfds].fd = fd;
  epoll_fds[numfds].events = 0;
  set_fd_internal (fd, (void *) &epoll_fds[numfds]);
  ++numfds;
}

void
engine_del_fd (int fd)
{
  struct epoll_event ev;
  struct epoll_fd *epfd = (struct epoll_fd *) get_fd_internal (fd);

  if (epoll_ctl (epoll_id, EPOLL_CTL_DEL, fd, &ev) < 0)
    abort ();

  if (epfd - epoll_fds != numfds - 1)
  {
    *epfd = epoll_fds[numfds - 1];
    set_fd_internal (epfd->fd, (void *) epfd);

    /* update the epoll internal pointer as well */
    ev.events = epfd->events;
    ev.data.ptr = epfd;
    if (epoll_ctl (epoll_id, EPOLL_CTL_MOD, epfd->fd, &ev) < 0)
      abort ();
  }

  --numfds;
}

void
engine_change_fd_state (int fd, unsigned int stateplus)
{
  struct epoll_event ev;
  struct epoll_fd *epfd = (struct epoll_fd *) get_fd_internal (fd);

  ev.events = 0;
  ev.data.ptr = epfd;
  if (stateplus & FDF_WANTWRITE)
    ev.events |= EPOLLOUT;
  if (stateplus & FDF_WANTREAD)
    ev.events |= EPOLLIN | EPOLLHUP | EPOLLERR;

  if (ev.events != epfd->events)
  {
    epfd->events = ev.events;
    if (epoll_ctl (epoll_id, EPOLL_CTL_MOD, fd, &ev) < 0)
      abort ();
  }
}

#define ENGINE_MAX_EVENTS 512
#define ENGINE_MAX_LOOPS (2 * (MAXCONNECTIONS / 512))

int
engine_read_message (time_t delay)
{
  struct epoll_event events[ENGINE_MAX_EVENTS], *pevent;
  struct epoll_fd *epfd;
  int nfds, i, numloops = 0, eventsfull;
  unsigned int fdflags;
  int fdtype;
  void *fdvalue;
  aClient *client_p;

  do
  {
    nfds = epoll_wait (epoll_id, events, ENGINE_MAX_EVENTS, delay * 1000);

    if (nfds == -1)
    {
      if (errno == EINTR || errno == EAGAIN)
        return -1;

      report_error ("epoll_wait: %s:%s", &me);
      sleep (5);
      return -1;
    }
    eventsfull = nfds == ENGINE_MAX_EVENTS;

    if (delay || numloops)
      NOW = timeofday = time (NULL);
    numloops++;

    for (i = 0, pevent = events; i < nfds; i++, pevent++)
    {
      epfd = pevent->data.ptr;
      if (epfd->fd != -1)
      {
        int rr = pevent->events & (EPOLLIN | EPOLLHUP | EPOLLERR);
        int rw = pevent->events & EPOLLOUT;

        get_fd_info (epfd->fd, &fdtype, &fdflags, &fdvalue);

        switch (fdtype)
        {
           case FDT_NONE:
             break;

           case FDT_AUTH:
             client_p = (aClient *) fdvalue;
             if (rr)
               read_authports (client_p);
             if (rw && client_p->authfd >= 0)
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
             abort ();
        }
      }
    }
  }
  while (eventsfull && numloops < ENGINE_MAX_LOOPS);

  return 0;
}
#endif /* USE_EPOLL */
