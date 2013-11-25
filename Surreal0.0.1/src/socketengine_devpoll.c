/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/socketengine_devpoll.c
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
 *  $Id: socketengine_devpoll.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "config.h"

#ifdef USE_DEVPOLL

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "fds.h"
#include "sock.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/devpoll.h>
#include <sys/poll.h>

static int devpoll_id = -1, numfds = 0;

void
engine_init ()
{
  devpoll_id = open ("/dev/poll", O_RDWR);
}

void
engine_add_fd (int fd)
{
  struct pollfd dev_fd;

  if (numfds >= MAXCONNECTIONS)
    abort ();

  dev_fd.events = 0;
  dev_fd.revents = 0;
  dev_fd.fd = fd;
  if (write (devpoll_id, &dev_fd, sizeof (struct pollfd)) !=
      sizeof (struct pollfd))
    abort ();

  set_fd_internal (fd, 0);
  ++numfds;
}

void
engine_del_fd (int fd)
{
  struct pollfd dev_fd;

  dev_fd.events = POLLREMOVE;
  dev_fd.revents = 0;
  dev_fd.fd = fd;
  if (write (devpoll_id, &dev_fd, sizeof (struct pollfd)) !=
      sizeof (struct pollfd))
    abort ();

  --numfds;
}

void
engine_change_fd_state (int fd, unsigned int stateplus)
{
  unsigned int events = 0;
  struct pollfd dev_fd;

  if (stateplus & FDF_WANTWRITE)
    events |= POLLOUT;
  if (stateplus & FDF_WANTREAD)
    events |= POLLIN | POLLHUP | POLLERR;

  dev_fd.events = events;
  dev_fd.revents = 0;
  dev_fd.fd = fd;

  if (write (devpoll_id, &dev_fd, sizeof (struct pollfd)) !=
      sizeof (struct pollfd))
    abort ();

  set_fd_internal (fd, (void *) events);
}

#define ENGINE_MAX_EVENTS 512
#define ENGINE_MAX_LOOPS (2 * (MAXCONNECTIONS / 512))

int
engine_read_message (time_t delay)
{
  struct pollfd events[ENGINE_MAX_EVENTS], *pevent;
  struct dvpoll dopoll;
  int nfds, i, numloops = 0, eventsfull;
  unsigned int fdflags, fdevents;
  int fdtype;
  void *fdvalue;
  aClient *client_p;

  dopoll.dp_fds = events;
  dopoll.dp_nfds = ENGINE_MAX_EVENTS;
  dopoll.dp_timeout = delay;
  do
  {
    nfds = ioctl (devpoll_id, DP_POLL, &dopoll);

    if (nfds < 0)
    {
      if (errno == EINTR || errno == EAGAIN)
        return -1;

      report_error ("ioctl(devpoll): %s:%s", &me);
      sleep (5);
      return -1;
    }
    eventsfull = nfds == ENGINE_MAX_EVENTS;

    if (delay || numloops)
      NOW = timeofday = time (NULL);
    numloops++;

    for (i = 0, pevent = events; i < nfds; i++, pevent++)
    {
      fdevents = (unsigned int) get_fd_internal (pevent->fd);
      if (pevent->fd != -1)
      {
        int rr = (pevent->revents & (POLLIN | POLLHUP | POLLERR))
          && (fdevents & (POLLIN | POLLHUP | POLLERR));
        int rw = (pevent->revents & POLLOUT) && (fdevents & POLLOUT);

        get_fd_info (pevent->fd, &fdtype, &fdflags, &fdvalue);

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
#endif /* USE_DEVPOLL */
