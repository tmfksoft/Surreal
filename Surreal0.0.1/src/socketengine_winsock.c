/*
 *UltimateIRCd - an Internet Relay Chat Daemon, src/socketengine_winsock.c
 *
 *Copyright (C) 2003-2007 by the past and present ircd coders, and others.
 *Refer to the documentation within doc/authors/ for full credits and copyrights.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation; either version 2 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program; if not, write to the Free Software
 *Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307
 *USA
 *
 *$Id: socketengine_winsock.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

/*
 * Winsock socketengine, based on the select socketengine
 * (to avoid cluttering the select_socketengine.c file with #ifdef's)
 */

#include "config.h"

#ifdef USE_WINSOCK

#include "common.h"
#include "sys.h"
#include "h.h"
#include "fds.h"
#include "sock.h"

#pragma comment(lib, "ws2_32.lib")

static fd_set g_read_set, g_write_set, g_excp_set;

void
engine_init ()
{
  FD_ZERO (&g_read_set);
  FD_ZERO (&g_write_set);
  FD_ZERO (&g_excp_set);
}

void
engine_add_fd (int fd)
{
  set_fd_internal (fd, (void *) 0);
}

void
engine_del_fd (int fd)
{
  FD_CLR (fd, &g_read_set);
  FD_CLR (fd, &g_write_set);
  FD_CLR (fd, &g_excp_set);
}

void
engine_change_fd_state (int fd, unsigned int stateplus)
{
  int prevstate = (int) get_fd_internal (fd);

  if ((stateplus & FDF_WANTREAD) && !(prevstate & FDF_WANTREAD))
  {
    FD_SET (fd, &g_read_set);
    FD_SET (fd, &g_excp_set);
    prevstate |= FDF_WANTREAD;
  }

  else if (!(stateplus & FDF_WANTREAD) && (prevstate & FDF_WANTREAD))
  {
    FD_CLR (fd, &g_read_set);
    FD_CLR (fd, &g_excp_set);
    prevstate &= ~(FDF_WANTREAD);
  }

  if ((stateplus & FDF_WANTWRITE) && !(prevstate & FDF_WANTWRITE))
  {
    FD_SET (fd, &g_write_set);
    prevstate |= FDF_WANTWRITE;
  }

  else if (!(stateplus & FDF_WANTWRITE) && (prevstate & FDF_WANTWRITE))
  {
    FD_CLR (fd, &g_write_set);
    prevstate &= ~(FDF_WANTWRITE);
  }

  set_fd_internal (fd, (void *) prevstate);
}

static void
engine_get_fdsets (fd_set * r, fd_set * w, fd_set * e)
{
  memcpy (r, &g_read_set, sizeof (fd_set));
  memcpy (w, &g_write_set, sizeof (fd_set));
  memcpy (e, &g_excp_set, sizeof (fd_set));
}

int
engine_read_message (time_t delay)
{
  fd_set read_set, write_set, excp_set;
  struct timeval wt;
  int nfds, length, i;
  unsigned int fdflags;
  int fdtype;
  void *fdvalue;
  aClient *client_p;

  engine_get_fdsets (&read_set, &write_set, &excp_set);
  wt.tv_sec = delay;
  wt.tv_usec = 0;
  nfds = select (MAXCONNECTIONS, &read_set, &write_set, &excp_set, &wt);

  if (nfds == SOCKET_ERROR)
  {
    fdfprintf (stderr, "socket error occured: %d\n", WSAGetLastError ());
    if (WSAGetLastError () == WSAEINTR)
      return -1;

    report_error ("select %s:%s", &me);
    Sleep (5000);
    return -1;
  }
  else if (nfds == 0)
    return 0;

  if (delay)
    NOW = timeofday = time (NULL);

  for (i = 0; i < MAXCONNECTIONS; i++)
  {
    get_fd_info (i, &fdtype, &fdflags, &fdvalue);
    client_p = NULL;
    length = -1;

    if (nfds)
    {
      int rr = FD_ISSET (i, &read_set);
      int rw = FD_ISSET (i, &write_set);
      int ex = FD_ISSET (i, &excp_set);

      if (rr || rw || ex)
        nfds--;
      else
        continue;

      fdfprintf (stderr, "fd %d: %s%s%s\n", i, rr ? "read " : "",
                 rw ? "write" : "", ex ? "exception" : "");

      switch (fdtype)
      {
         case FDT_NONE:
           break;

         case FDT_AUTH:
           if (FD_ISSET (client_p->authfd, &excp_set))
           {
             int err, len = sizeof (err);

             if (getsockopt
                 (client_p->authfd, SOL_SOCKET, SO_ERROR, &err, &len) || err)
             {
               closesocket (client_p->authfd);
               if (client_p->authfd == highest_fd)
                 while (!local[highest_fd])
                   highest_fd--;
               client_p->authfd = -1;
               client_p->flags &= ~(FLAGS_AUTH | FLAGS_WRAUTH);
               check_client (client_p);
               continue;
             }
           }

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
           dns_do_callbacks ();
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
    else
      break;                    /* no more fds? break out of the loop */
  }                             /* end of for() loop for testing selected sockets */
  return 0;
}


#endif /* USE_WINSOCK */
