/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_auth.c
 *
 *  Copyright (C) 1992 Darren Reed
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
 *  $Id: s_auth.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "res.h"
#include "numeric.h"
#include "version.h"
#include <sys/types.h>
#ifndef _WIN32
# include <sys/socket.h>
# include <sys/file.h>
# include <sys/ioctl.h>
#else
# include <winsock2.h>
#endif
#include "inet.h"
#include <fcntl.h>
#include "sock.h"               /* If FD_ZERO isn't define up to this point */
/* define it (BSD4.2 needs this) */
#include "h.h"
#include "fdlist.h"
#include "fds.h"

static void authsenderr (aClient *);

/*
 * start_auth
 *
 * Flag the client to show that an attempt to contact the ident server on
 * the client's host.  The connect and subsequently the socket are all
 * put into 'non-blocking' mode.  Should the connect or any later phase
 * of the identifing process fail, it is aborted and the user is given
 * a username of "unknown".
 */
void
start_auth (aClient * client_p)
{
  struct SOCKADDR_IN sock;
  struct SOCKADDR_IN localaddr;
  u3_socklen_t locallen;

  Debug ((DEBUG_NOTICE, "start_auth(%x) fd %d status %d",
          client_p, client_p->fd, client_p->status));

  if ((client_p->authfd = socket (AFINET, SOCK_STREAM, 0)) == -1)
  {
#ifdef	USE_SYSLOG
    syslog (LOG_ERR, "Unable to create auth socket for %s:%m",
            get_client_name (client_p, TRUE));
#endif
    ircstp->is_abad++;
    return;
  }
  if (client_p->authfd >= MAXCONNECTIONS)
  {
    sendto_realops_lev (DEBUG_LEV, "Can't allocate fd for auth on %s",
                        get_client_name (client_p,
                                         (IsServer (client_p) ? HIDEME :
                                          TRUE)));
#ifdef _WIN32
    closesocket (client_p->authfd);
#else
    close (client_p->authfd);
#endif
    client_p->authfd = -1;
    return;
  }
#ifdef SHOW_HEADERS
#ifdef USE_SSL
  if (!IsSSL (client_p))
  {
#endif
    sendto_one (client_p, REPORT_DO_ID);
#ifdef USE_SSL
  }
#endif
#endif
  set_non_blocking (client_p->authfd, client_p);
  /*
   * get the local address of the client and bind to that to make the
   * auth request.  This used to be done only for ifdef VIRTTUAL_HOST,
   * but needs to be done for all clients since the ident request must
   * originate from that same address-- and machines with multiple IP
   * addresses are common now
   */
  locallen = sizeof (struct SOCKADDR_IN);

  memset (&localaddr, '\0', locallen);
  getsockname (client_p->fd, (struct SOCKADDR *) &localaddr,
               (u3_socklen_t *) & locallen);
  localaddr.SIN_PORT = htons (0);

  if (bind (client_p->authfd, (struct SOCKADDR *) &localaddr,
            sizeof (localaddr)) == -1)
  {
    report_error ("binding auth stream socket %s:%s", client_p);
#ifdef _WIN32
    closesocket (client_p->authfd);
#else
    close (client_p->authfd);
#endif
    client_p->authfd = -1;
    return;
  }

  memcpy ((char *) &sock.SIN_ADDR, (char *) &client_p->ip,
          sizeof (struct IN_ADDR));

  sock.SIN_PORT = htons (113);
  sock.SIN_FAMILY = AFINET;

#ifdef INET6                    /* skip identd check for outbound v4 connections. */
  if (IN6_IS_ADDR_V4MAPPED (&sock.sin6_addr))
  {
    ircstp->is_abad++;
#ifdef _WIN32
    closesocket (client_p->authfd);
#else
    close (client_p->authfd);
#endif
    client_p->authfd = -1;
#ifdef SHOW_HEADERS
#ifdef USE_SSL
    if (!IsSSL (client_p))
    {
#endif
      sendto_one (client_p, REPORT_FAIL_ID);
#ifdef USE_SSL
    }
#endif
#endif
    return;
  }
#endif

  if (connect (client_p->authfd, (struct SOCKADDR *) &sock,
               sizeof (sock)) == -1 &&
#ifndef _WIN32
      errno != EINPROGRESS
#else
      WSAGetLastError () != WSAEINPROGRESS
#endif
    )
  {
    ircstp->is_abad++;
    /* No error report from this... */
#ifdef _WIN32
    closesocket (client_p->authfd);
#else
    close (client_p->authfd);
#endif
    client_p->authfd = -1;
#ifdef SHOW_HEADERS
#ifdef USE_SSL
    if (!IsSSL (client_p))
    {
#endif
      sendto_one (client_p, REPORT_FAIL_ID);
#ifdef USE_SSL
    }
#endif
#endif
    return;
  }

  client_p->flags |= (FLAGS_WRAUTH | FLAGS_AUTH);
  if (client_p->authfd > highest_fd)
    highest_fd = client_p->authfd;

  add_fd (client_p->authfd, FDT_AUTH, client_p);
  return;
}


/*
 * send_authports
 *
 * Send the ident server a query giving "theirport , ourport". The write
 * is only attempted *once* so it is deemed to be a fail if the entire
 * write doesn't write all the data given.  This shouldnt be a problem
 * since the socket should have a write buffer far greater than this
 * message to store it in should problems arise. -avalon
 */
void
send_authports (aClient * client_p)
{
  struct SOCKADDR_IN us, them;
  char authbuf[32];
  u3_socklen_t ulen, tlen;

  Debug ((DEBUG_NOTICE, "write_authports(%x) fd %d authfd %d stat %d",
          client_p, client_p->fd, client_p->authfd, client_p->status));
  tlen = ulen = sizeof (us);

  if (getsockname
      (client_p->fd, (struct SOCKADDR *) &us, (u3_socklen_t *) & ulen)
      || getpeername (client_p->fd, (struct SOCKADDR *) &them,
                      (u3_socklen_t *) & tlen))
  {
#ifdef	USE_SYSLOG
    syslog (LOG_DEBUG, "auth get{sock,peer}name error for %s:%m",
            get_client_name (client_p, TRUE));
#endif
    authsenderr (client_p);
    return;
  }

  (void) ircsprintf (authbuf, "%u , %u\r\n",
                     (unsigned int) ntohs (them.SIN_PORT),
                     (unsigned int) ntohs (us.SIN_PORT));

  Debug ((DEBUG_SEND, "sending [%s] to auth port %s.113",
          authbuf, inet_ntop (AFINET, (char *) &them.SIN_ADDR, mydummy,
                              sizeof (mydummy))));

  if (write (client_p->authfd, authbuf, strlen (authbuf)) != strlen (authbuf))
  {
    authsenderr (client_p);
    return;
  }

  client_p->flags &= ~FLAGS_WRAUTH;

  return;
}

/*
 * authsenderr() *
 * input - pointer to aClient output
 */
static void
authsenderr (aClient * client_p)
{
  ircstp->is_abad++;

  del_fd (client_p->authfd);
#ifdef _WIN32
  closesocket (client_p->authfd);
#else
  close (client_p->authfd);
#endif

  if (client_p->authfd == highest_fd)
  {
    while (!local[highest_fd])
    {
      highest_fd--;
    }
  }
  client_p->authfd = -1;
  client_p->flags &= ~(FLAGS_AUTH | FLAGS_WRAUTH);
#ifdef SHOW_HEADERS
#ifdef USE_SSL
  if (!IsSSL (client_p))
  {
#endif
    sendto_one (client_p, REPORT_FAIL_ID);
#ifdef USE_SSL
  }
#endif
#endif


  return;
}

/*
 * read_authports
 *
 * read the reply (if any) from the ident server we connected to. The
 * actual read processing here is pretty weak - no handling of the
 * reply if it is fragmented by IP.
 *
 * Whoever wrote this code should be shot.
 * Looks like it's trouncing on memory it shouldn't be.
 * Rewriting, some credit goes to wd for saving me time with his code.
 * - lucas
 * 
 */

#define AUTHBUFLEN 128

void
read_authports (aClient * client_p)
{
  char buf[AUTHBUFLEN], usern[USERLEN + 1];
  int len, userncnt;
  char *userid = "", *s, *reply, *os, *tmp;

  len = RECV (client_p->authfd, buf, AUTHBUFLEN);

  if (len > 0)
  {
    do
    {
      if (buf[len - 1] != '\n')
        break;

      buf[--len] = '\0';

      if (len == 0)
        break;

      if (buf[len - 1] == '\r')
        buf[--len] = '\0';

      if (len == 0)
        break;

      s = strchr (buf, ':');
      if (!s)
        break;
      s++;

      while (MyIsSpace (*s))
        s++;

      reply = s;
      if (strncmp (reply, "USERID", 6))
        break;

      s = strchr (reply, ':');
      if (!s)
        break;
      s++;

      while (MyIsSpace (*s))
        s++;

      os = s;

#ifdef NO_IDENT_SYSTYPE_OTHER
      if (strncmp (os, "OTHER", 5) == 0)
        break;
#endif

      s = strchr (os, ':');
      if (!s)
        break;
      s++;

      while (MyIsSpace (*s))
        s++;

      userid = tmp = usern;
      /* s is the pointer to the beginning of the userid field */
      for (userncnt = USERLEN; *s && userncnt; s++)
      {
        if (*s == '@')
          break;

        if (!MyIsSpace (*s) && *s != ':')
        {
          *tmp++ = *s;
          userncnt--;
        }
      }
      *tmp = '\0';

    }
    while (0);
  }

  del_fd (client_p->authfd);
#ifdef _WIN32
  closesocket (client_p->authfd);
#else
  close (client_p->authfd);
#endif
  if (client_p->authfd == highest_fd)
    while (!local[highest_fd])
      highest_fd--;
  client_p->authfd = -1;
  ClearAuth (client_p);

  if (!*userid)
  {
    ircstp->is_abad++;
    strcpy (client_p->username, "unknown");
#ifdef SHOW_HEADERS
#ifdef USE_SSL
    if (!IsSSL (client_p))
    {
#endif
      sendto_one (client_p, REPORT_FAIL_ID);
#ifdef USE_SSL
    }
#endif
#endif
    return;
  }
#ifdef SHOW_HEADERS
  else
  {
#ifdef USE_SSL
    if (!IsSSL (client_p))
    {
#endif
      sendto_one (client_p, REPORT_FIN_ID);
#ifdef USE_SSL
    }
#endif
  }
#endif

  ircstp->is_asuc++;
  strncpyzt (client_p->username, userid, USERLEN + 1);
  client_p->flags |= FLAGS_GOTID;
  return;
}
