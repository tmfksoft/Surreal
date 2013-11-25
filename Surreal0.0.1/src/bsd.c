/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/bsd.c
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
 *  $Id: bsd.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include "fds.h"
#include <signal.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

extern int errno;               /* ...seems that errno.h doesn't define this everywhere */

#if defined(DEBUGMODE) || defined (DNS_DEBUG)
int writecalls = 0, writeb[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int readcalls = 0;
#endif
void
dummy ()
{
#ifndef _WIN32
  struct sigaction act;

  act.sa_handler = dummy;
  act.sa_flags = 0;
  (void) sigemptyset (&act.sa_mask);
  (void) sigaddset (&act.sa_mask, SIGALRM);
  (void) sigaddset (&act.sa_mask, SIGPIPE);
# ifdef SIGWINCH
  (void) sigaddset (&act.sa_mask, SIGWINCH);
# endif
  (void) sigaction (SIGALRM, &act, (struct sigaction *) NULL);
  (void) sigaction (SIGPIPE, &act, (struct sigaction *) NULL);
# ifdef SIGWINCH
  (void) sigaction (SIGWINCH, &act, (struct sigaction *) NULL);
# endif
#endif
}

/*
 * deliver_it 
 *
 * Attempt to send a sequence of bytes to the connection. 
 * Returns 
 *  < 0     Some fatal error occurred, (but not EWOULDBLOCK). 
 *    his return is a request to close the socket and clean up the link.
 *  >= 0    No real error occurred, returns the number of bytes actually
 *    transferred. EWOULDBLOCK and other similar conditions should be mapped
 *    to zero return.
 *    Upper level routine will have to decide what to do with
 *    those unwritten bytes... 
 * *NOTE*  alarm calls have been preserved, so this should work equally 
 *  well whether blocking or non-blocking mode is used...
 */
int
deliver_it (aClient * client_p, char *str, int len)
{
  int retval;
  aClient *acpt = client_p->acpt;
#ifdef	DEBUGMODE
  writecalls++;
#endif
#ifdef USE_SSL
  if (IsSSL (client_p) && client_p->ssl)
    retval = safe_SSL_write (client_p, str, len);
  else
#endif
    retval = send (client_p->fd, str, len, 0);

  /*
   * Convert WOULDBLOCK to a return of "0 bytes moved". This 
   * should occur only if socket was non-blocking. Note, that all is
   * Ok, if the 'write' just returns '0' instead of an error and
   * errno=EWOULDBLOCK. 
   */
  if (retval < 0 &&
#ifdef _WIN32
      (WSAGetLastError () == WSAEWOULDBLOCK ||
       WSAGetLastError () == WSAENOBUFS)
#else
      (errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOBUFS)
#endif
    )
  {
    retval = 0;
    client_p->flags |= FLAGS_BLOCKED;
    set_fd_flags (client_p->fd, FDF_WANTWRITE);
    return (retval);            /* Just get out now... */
  }
  else if (retval > 0)
  {
    if (client_p->flags & FLAGS_BLOCKED)
    {
      client_p->flags &= ~FLAGS_BLOCKED;
      unset_fd_flags (client_p->fd, FDF_WANTWRITE);
    }
  }

#ifdef DEBUGMODE
  if (retval < 0)
  {
    writeb[0]++;
    Debug ((DEBUG_ERROR, "write error (%s) to %s",
            strerror (errno), client_p->name));
  }
  else if (retval == 0)
    writeb[1]++;
  else if (retval < 16)
    writeb[2]++;
  else if (retval < 32)
    writeb[3]++;
  else if (retval < 64)
    writeb[4]++;
  else if (retval < 128)
    writeb[5]++;
  else if (retval < 256)
    writeb[6]++;
  else if (retval < 512)
    writeb[7]++;
  else if (retval < 1024)
    writeb[8]++;
  else
    writeb[9]++;
#endif
  if (retval > 0)
  {
    client_p->sendB += retval;
    me.sendB += retval;
    if (client_p->sendB > 1023)
    {
      client_p->sendK += (client_p->sendB >> 10);
      client_p->sendB &= 0x03ff;        /* 2^10 = 1024, 3ff = 1023 */
    }
    if (acpt != &me)
    {
      acpt->sendB += retval;
      if (acpt->sendB > 1023)
      {
        acpt->sendK += (acpt->sendB >> 10);
        acpt->sendB &= 0x03ff;
      }
    }
    if (me.sendB > 1023)
    {
      me.sendK += (me.sendB >> 10);
      me.sendB &= 0x03ff;
    }
  }
  return (retval);
}
