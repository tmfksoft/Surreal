/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/socketengine_poll.c
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
 *  $Id: socketengine_poll.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "config.h"

#ifdef USE_POLL

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "fds.h"
#include "sock.h"
#include <sys/poll.h>

static void engine_get_pollfds (struct pollfd **, int *);


struct pollfd poll_fds[MAXCONNECTIONS];
int last_pfd = -1;

void
engine_init ()
{
}

void
engine_add_fd (int fd)
{
  struct pollfd *pfd = &poll_fds[++last_pfd];

  /* sanity check */
  if (last_pfd >= MAXCONNECTIONS)
    abort ();

  set_fd_internal (fd, (void *) last_pfd);

  pfd->fd = fd;
  pfd->events = 0;
  pfd->revents = 0;
}

void
engine_del_fd (int fd)
{
  int arrayidx = (int) get_fd_internal (fd);

  /* If it's at the end of the array, just chop it off */
  if (arrayidx == last_pfd)
  {
    fdfprintf (stderr, "Removing %d[%d] from end of pollfds\n", last_pfd, fd);
    last_pfd--;
    return;
  }

  /* Otherwise, move the last array member to where the old one was */
  fdfprintf (stderr,
             "Moving pfd %d[%d] to vacated spot %d[%d] -- now %d[%d]\n",
             last_pfd, poll_fds[last_pfd].fd, arrayidx, fd, last_pfd, fd);
  memcpy (&poll_fds[arrayidx], &poll_fds[last_pfd], sizeof (struct pollfd));
  last_pfd--;
  set_fd_internal (poll_fds[arrayidx].fd, (void *) arrayidx);
}

void
engine_change_fd_state (int fd, unsigned int stateplus)
{
  int arrayidx = (int) get_fd_internal (fd);
  struct pollfd *pfd = &poll_fds[arrayidx];

  pfd->events = 0;
  if (stateplus & FDF_WANTREAD)
    pfd->events |= POLLIN | POLLHUP | POLLERR;
  if (stateplus & FDF_WANTWRITE)
    pfd->events |= POLLOUT;
}

void
engine_get_pollfds (struct pollfd **pfds, int *numpfds)
{
  *pfds = poll_fds;
  *numpfds = (last_pfd + 1);
}

int
engine_read_message (time_t delay)
{
  static struct pollfd poll_fdarray[MAXCONNECTIONS];

  struct pollfd *pfd;
  int nfds, nbr_pfds, length, i;
  unsigned int fdflags;
  int fdtype;
  void *fdvalue;
  aClient *client_p;

  engine_get_pollfds (&pfd, &nbr_pfds);
  memcpy (poll_fdarray, pfd, sizeof (struct pollfd) * nbr_pfds);

  nfds = poll (poll_fdarray, nbr_pfds, delay * 1000);
  if (nfds == -1)
  {
    if (((errno == EINTR) || (errno == EAGAIN)))
      return -1;
    report_error ("poll %s:%s", &me);
    sleep (5);
    return -1;
  }

  if (delay)
    NOW = timeofday = time (NULL);

  for (pfd = poll_fdarray, i = 0; nfds && (i < nbr_pfds); i++, pfd++)
  {
    get_fd_info (pfd->fd, &fdtype, &fdflags, &fdvalue);

    client_p = NULL;
    length = -1;

    if (nfds && pfd->revents)
    {
      int rr = pfd->revents & (POLLIN | POLLHUP | POLLERR);
      int rw = pfd->revents & (POLLOUT);

      fdfprintf (stderr, "fd %d: %s%s\n", pfd->fd, rr ? "read " : "",
                 rw ? "write" : "");

      nfds--;

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
#ifdef USE_ADNS
           if (rr)
             dns_readable (pfd->fd, NULL);
           if (rw)
             dns_writeable (pfd->fd, NULL);
#else
           do_dns_async ();
#endif
           break;

         case FDT_CLIENT:
           client_p = (aClient *) fdvalue;
           readwrite_client (client_p, rr, rw);
           break;

         default:
           abort ();            /* unknown client type? bail! */
      }
    }
  }                             /* end of for() loop for testing polled sockets */

  return 0;
}
#endif /* USE_POLL */
