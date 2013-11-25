/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_bsd.c
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
 *  $Id: s_bsd.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "res.h"
#include "numeric.h"
#include "version.h"
#include "zlink.h"
#include "throttle.h"
#include "userban.h"
#include <sys/types.h>
#ifndef _WIN32
# include <sys/socket.h>
# include <sys/file.h>
# include <sys/ioctl.h>
# include <signal.h>
# include <utmp.h>
# include <sys/resource.h>
#else
# include <winsock2.h>
#endif
#if defined(SOL20)
# include <sys/filio.h>
# include <sys/select.h>
# include <unistd.h>
#endif
#include "inet.h"
#include <stdio.h>
#include <fcntl.h>

#ifdef USE_SSL
#include "ssl.h"
#endif

#ifndef USE_ADNS
# ifdef  AIX
#  include <time.h>
#  include <arpa/nameser.h>
# else
#  include "nameser.h"
# endif
#include "resolv.h"
#endif

#include "sock.h"

/* If FD_ZERO isn't define up to this point,
 * define it (BSD4.2 needs this)
 */

#include "h.h"
#include "fdlist.h"
#include "fds.h"

extern fdlist default_fdlist;

#ifndef IN_LOOPBACKNET
# define IN_LOOPBACKNET	0x7f
#endif

#if defined(MAXBUFFERS)
int rcvbufmax = 0, sndbufmax = 0;
#endif

#ifdef MAXBUFFERS
void reset_sock_opts (int, int);
#endif

aClient *local[MAXCONNECTIONS];
int highest_fd = 0, resfd = -1;
time_t timeofday;
static struct SOCKADDR_IN mysk;
static struct SOCKADDR *connect_inet (aConfItem *, aClient *, int *);
static int check_init (aClient *, char *);
static void set_sock_opts (int, aClient *);

#ifdef USE_ADNS
static void got_client_dns (void *, adns_answer *);
static void got_async_connect_dns (void *, adns_answer *);
#endif

struct SOCKADDR_IN vserv;

char specific_virtual_host;

#if defined(MAXBUFFERS)
static char *readbuf;
#else
static char readbuf[8192];
#endif

int find_fline (aClient *);

#ifdef INET6
static unsigned char minus_one[] =
  { 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 0
};

void
ip6_expand (char *host, size_t len)
{
  if (*host == ':')
  {
    char *b;
    DupString (b, host);
    *host = '0';
    strncpyzt ((host + 1), b, len - 1);
    MyFree (b);
  }
}
#endif

/* Silly macro to ignore certain report error statements */
#define silent_report_error(x,y)

/*
 * Try and find the correct name to use with getrlimit() for setting
 * the max. number of files allowed to be open by this process.
 */

#ifdef RLIMIT_FDMAX
#define RLIMIT_FD_MAX   RLIMIT_FDMAX
#else
#ifdef RLIMIT_NOFILE
#define RLIMIT_FD_MAX RLIMIT_NOFILE
#else
#ifdef RLIMIT_OPEN_MAX
#define RLIMIT_FD_MAX RLIMIT_OPEN_MAX
#else
#undef RLIMIT_FD_MAX
#endif
#endif
#endif

/*
 * add_local_domain() 
 * Add the domain to hostname, if it is missing
 * (as suggested by eps@TOASTER.SFSU.EDU)
 */

void
add_local_domain (char *hname, int size)
{
/* this is now a dummy (it used to return the local domain name?!
 * but we could return something better? - Fish
 */
  if (!strchr (hname, '.'))
  {
    (void) strncat (hname, ".", size - 1);
    (void) strncat (hname, "localnet", size - 1);
  }
  return;
}

/*
 * Cannot use perror() within daemon. stderr is closed in
 * ircd and cannot be used. And, worse yet, it might have
 * been reassigned to a normal connection...
 */

/*
 * report_error
 * This a replacement for perror(). Record error to log and 
 * also send a copy to all *LOCAL* opers online. 
 * text    is a *format* string for outputting error. It must
 * contain only two '%s', the first will be replaced by the
 * sockhost from the client_p, and the latter will be taken from 
 * sys_errlist[errno].
 * 
 * client_p, if not NULL, is the *LOCAL* client associated with
 * the error.
 */

void
report_error (char *text, aClient * client_p)
{
  int errtmp = errno;           /* debug may change 'errno' */
  char *host;
  int err;
  u3_socklen_t len = sizeof (err);
  extern char *strerror ();

  host =
    (client_p) ? get_client_name (client_p,
                                  (IsServer (client_p) ? HIDEME : FALSE)) :
    "";

  Debug ((DEBUG_ERROR, text, host, strerror (errtmp)));
  /*
   * Get the *real* error from the socket (well try to anyway..). This
   * may only work when SO_DEBUG is enabled but its worth the gamble
   * anyway.
   */

#ifdef	SO_ERROR
  if (!IsMe (client_p) && client_p->fd >= 0)
    if (!getsockopt
        (client_p->fd, SOL_SOCKET, SO_ERROR, (char *) &err,
         (u3_socklen_t *) & len))
      if (err)
        errtmp = err;
#endif
  sendto_realops_lev (DEBUG_LEV, text, host, strerror (errtmp));
#ifdef USE_SYSLOG
  syslog (LOG_WARNING, text, host, strerror (errtmp));
  if (bootopt & BOOT_STDERR)
  {
    fprintf (stderr, text, host, strerror (errtmp));
    fprintf (stderr, "\n");
  }
#endif
  return;
}

/* inetport
 *
 * Create a socket in the AFINET domain, bind it to the port given in
 * 'port' and listen to it.  Connections are accepted to this socket
 * depending on the IP# mask given by 'name'.  Returns the fd of the
 * socket created or -1 on error.
 */
#ifndef INET6
int
inetport (aClient * client_p, char *name, int port, u_long bind_addr)
#else
int
inetport (aClient * client_p, char *name, int port, char *bind_addr)
#endif
{
  static struct SOCKADDR_IN server;
  int ad[4];
  u3_socklen_t len = sizeof (server);
  char ipname[20];

  ad[0] = ad[1] = ad[2] = ad[3] = 0;
  /*
   * do it this way because building ip# from separate values for each
   * byte requires endian knowledge or some nasty messing. Also means
   * easy conversion of "*" 0.0.0.0 or 134.* to 134.0.0.0 :-)
   */
#ifndef INET6
  (void) sscanf (name, "%d.%d.%d.%d", &ad[0], &ad[1], &ad[2], &ad[3]);
  (void) ircsprintf (ipname, "%d.%d.%d.%d", ad[0], ad[1], ad[2], ad[3]);
#else
  if (*name == '*')
    ircsprintf (ipname, "::");
  else
    ircsprintf (ipname, "%s", name);
#endif

  if (client_p != &me)
  {
    (void) ircsprintf (client_p->sockhost, "%-.42s.%.u",
                       name, (unsigned int) port);
    (void) strcpy (client_p->name, me.name);
  }

  /* At first, open a new socket */

  if (client_p->fd == -1)
  {
    client_p->fd = socket (AFINET, SOCK_STREAM, 0);
    if (client_p->fd < 0 &&
#ifndef _WIN32
        errno == EAGAIN
#else
        WSAGetLastError () == WSAEMFILE
#endif
      )
    {
      sendto_realops ("opening stream socket %s: No more sockets",
                      get_client_name (client_p, HIDEME));
      return -1;
    }
  }
  if (client_p->fd < 0)
  {
    report_error ("opening stream socket %s:%s", client_p);
    return -1;
  }
  else if (client_p->fd >= (HARD_FDLIMIT - 10))
  {
    sendto_realops ("No more connections allowed (%s)", client_p->name);
#ifdef _WIN32
    closesocket (client_p->fd);
#else
    (void) close (client_p->fd);
#endif
    return -1;
  }
  set_sock_opts (client_p->fd, client_p);

  /*
   * Bind a port to listen for new connections if port is non-null,
   * else assume it is already open and try get something from it.
   */
  if (port)
  {
    memset ((char *) &server, '\0', sizeof (server));
    server.SIN_FAMILY = AFINET;

    if (bind_addr)
#ifndef INET6
      server.SIN_ADDR.S_ADDR = bind_addr;
    else
      server.SIN_ADDR.S_ADDR = INADDR_ANY;
#else
      memcpy (server.SIN_ADDR.S_ADDR, bind_addr, sizeof (struct IN_ADDR));
    else
      memset (server.SIN_ADDR.S_ADDR, 0x0, sizeof (struct IN_ADDR));
#endif
    server.SIN_PORT = htons ((short) port);
    /* 
     * Try 10 times to bind the socket with an interval of 20
     * seconds. Do this so we dont have to keepp trying manually to
     * bind. Why ? Because a port that has closed often lingers
     * around for a short time. This used to be the case.  Now it no
     * longer is. Could cause the server to hang for too long -
     * avalon
     */
    if (bind (client_p->fd, (struct SOCKADDR *) &server, sizeof (server)) ==
        -1)
    {
      report_error ("binding stream socket %s:%s", client_p);
#ifdef _WIN32
      closesocket (client_p->fd);
#else
      (void) close (client_p->fd);
#endif
      return -1;
    }
  }
  if (getsockname
      (client_p->fd, (struct SOCKADDR *) &server, (u3_socklen_t *) & len))
  {
    report_error ("getsockname failed for %s:%s", client_p);
#ifdef _WIN32
    closesocket (client_p->fd);
#else
    (void) close (client_p->fd);
#endif
    return -1;
  }

  if (client_p == &me)
  {
    /* KLUDGE to get it work... */
    char buf[1024];

    (void) ircsprintf (buf, rpl_str (RPL_MYPORTIS), me.name, "*",
                       ntohs (server.SIN_PORT));
    (void) write (0, buf, strlen (buf));
  }
  if (client_p->fd > highest_fd)
    highest_fd = client_p->fd;
#ifndef INET6
  /*
     client_p->ip.S_ADDR = inet_addr(ipname);
   */
  inet_pton (AFINET, ipname, (struct IN_ADDR *) &client_p->ip.S_ADDR);
#else
  inet_pton (AFINET, ipname, client_p->ip.S_ADDR);
#endif
  client_p->port = (int) ntohs (server.SIN_PORT);
  /*
   * If the operating system has a define for SOMAXCONN, use it,
   * otherwise use HYBRID_SOMAXCONN -Dianora
   */

#ifdef SOMAXCONN
  (void) listen (client_p->fd, SOMAXCONN);
#else
  (void) listen (client_p->fd, HYBRID_SOMAXCONN);
#endif
  local[client_p->fd] = client_p;

  return 0;
}

/*
 * add_listener
 *
 * Create a new client which is essentially the stub like 'me' to be used
 * for a socket that is passive (listen'ing for connections to be
 * accepted).
 */
int
add_listener (aConfItem * aconf)
{
  aClient *client_p;
#ifndef INET6
  u_long vaddr;
#else
  char vaddr[sizeof (struct IN_ADDR)];
#endif

#ifdef USE_SSL
  extern int ssl_capable;
#endif

  client_p = make_client (NULL, NULL);
  client_p->flags = FLAGS_LISTEN;
  client_p->acpt = client_p;
  client_p->from = client_p;
  SetMe (client_p);
  strncpyzt (client_p->name, aconf->host, sizeof (client_p->name));

  if ((aconf->passwd[0] != '\0') && (aconf->passwd[0] != '*'))
#ifndef INET6
    /*
       vaddr = inet_addr(aconf->passwd);
       else
       vaddr = (u_long) NULL;
     */
    inet_pton (AFINET, aconf->passwd, &vaddr);
  else
    memset (&vaddr, 0x0, sizeof (struct IN_ADDR));
#else
    inet_pton (AFINET, aconf->passwd, vaddr);
  else
    memset (vaddr, 0x0, sizeof (struct IN_ADDR));
#endif

  if (inetport (client_p, aconf->host, aconf->port, vaddr))
    client_p->fd = -2;

  if (client_p->fd >= 0)
  {
    client_p->confs = make_link ();
    client_p->confs->next = NULL;
    client_p->confs->value.aconf = aconf;
    set_non_blocking (client_p->fd, client_p);

    add_fd (client_p->fd, FDT_LISTENER, client_p);
    set_fd_flags (client_p->fd, FDF_WANTREAD);
#ifdef USE_SSL
    if ((strcmp (aconf->name, "SSL")) == 0 && ssl_capable)
    {
      SetSSL (client_p);
      client_p->ssl = NULL;
      client_p->client_cert = NULL;
    }
#endif
  }
  else
    free_client (client_p);
  return 0;
}

/*
 * close_listeners
 *
 * Close and free all clients which are marked as having their socket open
 * and in a state where they can accept connections.  Unix sockets have
 * the path to the socket unlinked for cleanliness.
 */
void
close_listeners ()
{
  aClient *client_p;
  int i;
  aConfItem *aconf;

  /*
   * close all 'extra' listening ports we have and unlink the file
   * name if it was a unix socket.
   */
  for (i = highest_fd; i >= 0; i--)
  {
    if (!(client_p = local[i]))
      continue;
    if (client_p == &me || !IsListening (client_p))
      continue;
    aconf = client_p->confs->value.aconf;

    if (IsIllegal (aconf) && aconf->clients == 0)
    {
      close_connection (client_p);
    }
  }
}

#ifdef HAVE_FD_ALLOC
fd_set *write_set, *read_set;

#endif

/* init_sys */
void
init_sys ()
{
  int fd;

#ifdef RLIMIT_FD_MAX
  struct rlimit limit;

  if (!getrlimit (RLIMIT_FD_MAX, &limit))
  {
    if (limit.rlim_max < MAXCONNECTIONS)
    {
      fprintf (stderr, "Error fd table too big\n");
      fprintf (stderr, "Hard Limit: %ld IRC max: %d\n",
               (long) limit.rlim_max, MAXCONNECTIONS);
      fprintf (stderr,
               "Reconfigure using --with-maxclients=%ld and recompile\n",
               (long) limit.rlim_max);
      fprintf (stderr,
               "========================================================\n");
      exit (-1);
    }
    limit.rlim_cur = limit.rlim_max;    /* make soft limit the max */
    if (setrlimit (RLIMIT_FD_MAX, &limit) == -1)
    {
      fprintf (stderr, "error setting max fd's to %ld\n",
               (long) limit.rlim_cur);
      fprintf (stderr,
               "========================================================\n");
      exit (-1);
    }

    printf ("Value of FD_SETSIZE is %d\n", FD_SETSIZE);
    printf ("Value of NOFILE is %d\n", NOFILE);
  }
#endif

# if !defined(SOL20) && !defined(_WIN32)
  setlinebuf (stderr);
# endif

  fprintf (stderr, "IRCd initialized, launching into background.\n");
  fprintf (stderr,
           "========================================================\n");



  for (fd = 3; fd < MAXCONNECTIONS; fd++)
  {
    (void) close (fd);
    local[fd] = NULL;
  }
  local[1] = NULL;

  if (bootopt & BOOT_TTY)
  {
    engine_init ();

#ifdef USE_ADNS
    /* start up adns! */
    init_resolver ();
#else
    /* debugging is going to a tty */
    resfd = init_resolver (0x1f);

# ifndef _WIN32
    add_fd (resfd, FDT_RESOLVER, NULL);
    set_fd_flags (resfd, FDF_WANTREAD);
# endif
#endif
    return;
  }
#ifndef _WIN32                  /* Closing stdout freezes the window on windows */
  close (1);
  if (!(bootopt & BOOT_DEBUG) && !(bootopt & BOOT_STDERR))
  {
    close (2);
  }
#endif

  if (((bootopt & BOOT_CONSOLE) || isatty (0)) &&
      !(bootopt & (BOOT_INETD | BOOT_OPER)) && !(bootopt & BOOT_STDERR))
  {
#ifndef _WIN32
    int pid;

    if ((pid = fork ()) < 0)
    {
      if ((fd = open ("/dev/tty", O_RDWR)) >= 0)
      {
        write (fd, "Couldn't fork!\n", 15);     /* crude, but effective */
      }
      exit (0);
    }
    else if (pid > 0)
    {
      exit (0);
    }
# ifdef TIOCNOTTY
    if ((fd = open ("/dev/tty", O_RDWR)) >= 0)
    {
      ioctl (fd, TIOCNOTTY, (char *) NULL);
      close (fd);
    }
# endif
# if defined(SOL20) || defined(DYNIXPTX) || \
    defined(_POSIX_SOURCE) || defined(SVR4)
    setsid ();
# else
    setpgrp (0, (int) getpid ());
# endif
    close (0);                  /* fd 0 opened by inetd */
#endif /* _WIN32 */
    local[0] = NULL;
  }

  engine_init ();

#ifdef USE_ADNS
  /* ok, start adns up */
  init_resolver ();
#else
# ifndef _WIN32
  resfd = init_resolver (0x1f);
  add_fd (resfd, FDT_RESOLVER, NULL);
  set_fd_flags (resfd, FDF_WANTREAD);
# endif
#endif
  return;
}

void
write_pidfile ()
{
#ifdef IRCD_PIDFILE
  int fd;
  char buff[20];

  if ((fd = open (IRCD_PIDFILE, O_CREAT | O_WRONLY, 0600)) >= 0)
  {
    ircsprintf (buff, "%5d\n", (int) getpid ());
    if (write (fd, buff, strlen (buff)) == -1)
    {
      Debug ((DEBUG_NOTICE, "Error writing to pid file %s", IRCD_PIDFILE));
    }
    close (fd);
    return;
  }
# ifdef	DEBUGMODE
  else
    Debug ((DEBUG_NOTICE, "Error opening pid file %s", IRCD_PIDFILE));
# endif
#endif
}

#ifdef INET6
/* check whether this is a loopback address */
int
in6_is_addr_loopback (uint32_t * f)
{
  /* ipv6 loopback */
  if (((*f == 0) && (*(f + 1) == 0) &&
       (*(f + 2) == 0) && (*(f + 3) == htonl (1))) ||
      /* ipv4 mapped 127.0.0.1 */
      ((*(f + 2) == htonl (0x0000ffff)) && (*(f + 3) == htonl (0x7f000001))))
  {
    return 1;
  }
  return 0;
}
#endif

/*
 * Initialize the various name strings used to store hostnames. This is
 * set from either the server's sockhost (if client fd is a tty or
 * localhost) or from the ip# converted into a string. 0 = success, -1
 * = fail.
 */
static int
check_init (aClient * client_p, char *sockn)
{
  struct SOCKADDR_IN sk;
  u3_socklen_t len = sizeof (struct SOCKADDR_IN);

  /* If descriptor is a tty, special checking... * IT can't EVER be a tty */

  if (getpeername
      (client_p->fd, (struct SOCKADDR *) &sk, (u3_socklen_t *) & len) == -1)
  {

/*
 * This fills syslog, if on, and is just annoying. Nobody needs it. -lucas
 *    report_error("connect failure: %s %s", client_p);
 */
    return -1;
  }

  inet_ntop (AFINET, (char *) &sk.SIN_ADDR, sockn, HOSTLEN);
#ifndef INET6
  if (inet_netof (sk.SIN_ADDR) == IN_LOOPBACKNET)
# else
  if (in6_is_addr_loopback ((uint32_t *) & client_p->ip))
#endif
  {
#ifndef USE_ADNS
    client_p->hostp = NULL;
#endif
    strncpyzt (sockn, me.sockhost, HOSTLEN);
  }
  memcpy ((char *) &client_p->ip, (char *) &sk.SIN_ADDR,
          sizeof (struct IN_ADDR));

  client_p->port = (int) (ntohs (sk.SIN_PORT));

  return 0;
}

/*
 * Ordinary client access check. Look for conf lines which have the
 * same status as the flags passed. 0 = Success -1 = Access denied -2 =
 * Bad socket.
 */
int
check_client (aClient * client_p)
{
  static char sockname[HOSTLEN + 1];
#ifndef USE_ADNS
  struct hostent *hp = NULL;
#endif
  int i;

  Debug ((DEBUG_DNS, "ch_cl: check access for %s[%s]",
          client_p->name, inet_ntop (AFINET, (char *) &client_p->ip,
                                     mydummy, sizeof (mydummy))));

  if (check_init (client_p, sockname))
    return -2;

#ifndef USE_ADNS
  /* XXX Im not sure how this works yet (probably should be done in the adns functions */
  hp = client_p->hostp;
  /* 
   * Verify that the host to ip mapping is correct both ways and that
   * the ip#(s) for the socket is listed for the host.
   */
  if (hp)
  {
    for (i = 0; hp->h_addr_list[i]; i++)
      if (!memcmp
          (hp->h_addr_list[i],
           (char *) &client_p->ip, sizeof (struct IN_ADDR)))
        break;
    if (!hp->h_addr_list[i])
    {
      sendto_one (client_p,
                  "NOTICE AUTH :*** Your forward and reverse DNS do not match, ignoring hostname.");
      hp = NULL;
    }
  }
  if ((i = attach_Iline (client_p, hp, sockname)))
#else
  if ((i = attach_Iline (client_p, sockname)))
#endif
  {
    Debug ((DEBUG_DNS,
            "ch_cl: access denied: %s[%s]", client_p->name, sockname));
    return i;
  }

  Debug ((DEBUG_DNS, "ch_cl: access ok: %s[%s]", client_p->name, sockname));
#ifdef USE_ADNS
# ifndef INET6
  if (inet_netof (client_p->ip) ==
      IN_LOOPBACKNET
      || inet_netof (client_p->ip) == inet_netof (mysk.SIN_ADDR))
# else
  if ((in6_is_addr_loopback
       ((uint32_t *) & client_p->ip))
      || memcmp ((void *) client_p->ip.
                 S_ADDR,
                 (void *) mysk.SIN_ADDR.S_ADDR, sizeof (struct IN_ADDR)))
#endif
#else
# ifndef INET6
  if (inet_netof (client_p->ip) == IN_LOOPBACKNET ||
      inet_netof (client_p->ip) == inet_netof (mysk.SIN_ADDR))
# else
  if ((in6_is_addr_loopback ((uint32_t *) & client_p->ip))
      || memcmp ((void *) client_p->ip.S_ADDR, (void *) mysk.SIN_ADDR.S_ADDR,
                 sizeof (struct IN_ADDR)))
# endif
#endif
  {
    ircstp->is_loc++;
    client_p->flags |= FLAGS_LOCAL;
  }
  return 0;
}

#define	CFLAG	CONF_CONNECT_SERVER
#define	NFLAG	CONF_NOCONNECT_SERVER

/*
 * check_server_init(), check_server() check access for a server given
 * its name (passed in client_p struct). Must check for all C/N lines which
 * have a name which matches the name given and a host which matches. A
 * host alias which is the same as the server name is also acceptable
 * in the host field of a C/N line. 0 = Success -1 = Access denied -2 =
 * Bad socket.
 */
int
check_server_init (aClient * client_p)
{
  char *name;
  aConfItem *c_conf = NULL, *n_conf = NULL;
#ifndef USE_ADNS
  struct hostent *hp = NULL;
#endif
  Link *lp;
  name = client_p->name;
  Debug ((DEBUG_DNS,
          "sv_cl: check access for %s[%s]", name, client_p->sockhost));
  if (IsUnknown (client_p) && !attach_confs (client_p, name, CFLAG | NFLAG))
  {
    Debug ((DEBUG_DNS, "No C/N lines for %s", name));
    return -1;
  }
  lp = client_p->confs;
  /*
   * We initiated this connection so the client should have a C and N
   * line already attached after passing through the connect_server()
   * function earlier.
   */
  if (IsConnecting (client_p) || IsHandshake (client_p))
  {
    c_conf = find_conf (lp, name, CFLAG);
    n_conf = find_conf (lp, name, NFLAG);
    if (!c_conf || !n_conf)
    {
      sendto_realops_lev (DEBUG_LEV,
                          "Connecting Error: %s[%s]",
                          name, client_p->sockhost);
      det_confs_butmask (client_p, 0);
      return -1;
    }
  }
#ifdef USE_ADNS
  /*
   * check to make sure that the socket has a ip address. if it doesn't,
   * something is wrong - Fish
   */
  if (!client_p->ip.S_ADDR)
  {
    /* something is wrong if we don't have a IP addr here! (as the server connected to us!) */
    Debug ((DEBUG_DEBUG, "Hu? IP address of socket is empty!"));
    return -1;
  }
  return check_server (client_p, c_conf, n_conf, 0);
#else
/*
 * If the servername is a hostname, either an alias (CNAME) or
 * real name, then check with it as the host. Use gethostbyname()
 * to check for servername as hostname.
 */
  if (!client_p->hostp)
  {
    aConfItem *aconf;
    aconf = count_cnlines (lp);
    if (aconf)
    {
      char *s;
      Link lin;
      /* 
       * * Do a lookup for the CONF line *only* and not * the server
       * connection else we get stuck in a * nasty state since it
       * takes a SERVER message to * get us here and we cant
       * interrupt that very * well.
       */
      lin.value.aconf = aconf;
      lin.flags = ASYNC_CONF;
      nextdnscheck = 1;
      s = (char *) strchr (aconf->host, '@');
      s == NULL ? s = aconf->host : ++s;
      Debug ((DEBUG_DNS, "sv_ci:cache lookup (%s)", s));
      hp = gethost_byname (s, &lin);
    }
  }
  return check_server (client_p, hp, c_conf, n_conf, 0);
#endif
}

#ifdef USE_ADNS
int
check_server (aClient * client_p,
              aConfItem * c_conf, aConfItem * n_conf, int estab)
{
  char *name;
  char abuff[HOSTLEN + USERLEN + 2];
  char sockname[HOSTLEN + 1], fullname[HOSTLEN + 1];
  Link *lp = client_p->confs;
  struct DNSQuery *dnslookup = NULL;
  if (check_init (client_p, sockname))
    return -2;
/* This checks forward and reverse DNS are pointing to the same IP.
 * This check is going to be placed in the adns resolver callback (below)
 * and we might set a flag if its important 
 * - Fish (28/08/03)
 */
#if 0
  if (hp)
  {
    for (i = 0; hp->h_addr_list[i]; i++)
      if (!memcmp
          (hp->h_addr_list[i],
           (char *) &client_p->ip, sizeof (struct IN_ADDR)))
        break;
    if (!hp->h_addr_list[i])
    {
      sendto_realops_lev (DEBUG_LEV,
                          "Server IP# Mismatch: %s != %s[%08x]",
                          hp->h_name,
                          inet_ntop (AFINET,
                                     (char *)
                                     &client_p->
                                     ip,
                                     mydummy,
                                     sizeof
                                     (mydummy)),
                          *((unsigned long *) hp->h_addr));
      hp = NULL;
    }
  }
#endif
/* this check runs though all possible hostnames 
 * that the reverse DNS picked up. (ie, irc.irc-chat.net might point to multiple
 * CNAMES etc. 
 * seeing as we are not doing this forward reverse check yet, this can be ignored 
 * for now.
 * - Fish (28/08/03)
 */

  if (client_p->dnslookup)
    dnslookup = client_p->dnslookup;

  if (dnslookup)
    if ((dnslookup->answer->status == adns_s_ok)
        && (dnslookup->answer->type == adns_r_ptr))
    {
      Debug ((DEBUG_DNS,
              "CheckServer: Client has host %s",
              *dnslookup->answer->rrs.str));
      strncpyzt (fullname, (char *) *dnslookup->answer->rrs.str,
                 sizeof (fullname));
      add_local_domain (fullname, HOSTLEN - strlen (fullname));
      Debug ((DEBUG_DNS, "sv_cl: gethostbyaddr: %s->%s", sockname, fullname));
      (void) ircsprintf (abuff, "%s@%s", client_p->username, fullname);
      if (!c_conf)
        c_conf = find_conf_host (lp, abuff, CFLAG);
      if (!n_conf)
        n_conf = find_conf_host (lp, abuff, NFLAG);
      if (c_conf && n_conf)
      {
        get_sockhost (client_p, fullname);
      }
    }
  name = client_p->name;
  /*
   * Check for C and N lines with the hostname portion the ip number
   * of the host the server runs on. This also checks the case where
   * there is a server connecting from 'localhost'.
   */
  if (IsUnknown (client_p) && (!c_conf || !n_conf))
  {
    (void) ircsprintf (abuff, "%s@%s", client_p->username, sockname);
    if (!c_conf)
      c_conf = find_conf_host (lp, abuff, CFLAG);
    if (!n_conf)
      n_conf = find_conf_host (lp, abuff, NFLAG);
  }
  /*
   * Attach by IP# only if all other checks have failed. It is quite
   * possible to get here with the strange things that can happen when
   * using DNS in the way the irc server does. -avalon
   */
#if 0
  if ((!dnslookup) || (dnslookup->answer->status != adns_s_ok))
#endif
    if (!c_conf || !n_conf)
    {
      Debug ((DEBUG_DNS, "Attaching via IP only in check_server"));
      if (!c_conf)
        c_conf =
          find_conf_ip (lp, (char *) &client_p->ip, client_p->username,
                        CFLAG);
      if (!n_conf)
        n_conf =
          find_conf_ip (lp, (char *) &client_p->ip, client_p->username,
                        NFLAG);
    }
  /* detach all conf lines that got attached by attach_confs() */
  det_confs_butmask (client_p, 0);
  /* if no C or no N lines, then deny access */
  if (!c_conf || !n_conf)
  {
    get_sockhost (client_p, sockname);
    Debug ((DEBUG_DNS,
            "sv_cl: access denied: %s[%s@%s] c %x n %x",
            name, client_p->username, client_p->sockhost, c_conf, n_conf));
    return -1;
  }
  /* attach the C and N lines to the client structure for later use. */
  attach_conf (client_p, n_conf);
  attach_conf (client_p, c_conf);
  attach_confs (client_p, name, CONF_HUB | CONF_LEAF | CONF_ULINE);
  /* this may give client_p a new sendq length.. */
  client_p->sendqlen = get_sendq (client_p);
# ifndef INET6
  if ((c_conf->ipnum.S_ADDR == -1))
# else
  if (!memcmp (c_conf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR)))
#endif
    memcpy ((char *) &c_conf->ipnum,
            (char *) &client_p->ip, sizeof (struct IN_ADDR));
  get_sockhost (client_p, c_conf->host);
  Debug ((DEBUG_DNS, "sv_cl: access ok: %s[%s]", name, client_p->sockhost));
  if (estab)
    return m_server_estab (client_p);
  return 0;
}

#else /* USE_ADNS */

int
check_server (aClient * client_p,
              struct hostent *hp,
              aConfItem * c_conf, aConfItem * n_conf, int estab)
{
  char *name;
  char abuff[HOSTLEN + USERLEN + 2];
  char sockname[HOSTLEN + 1], fullname[HOSTLEN + 1];
  Link *lp = client_p->confs;
  int i;

  if (check_init (client_p, sockname))
    return -2;

  if (hp)
  {
    for (i = 0; hp->h_addr_list[i]; i++)
      if (!memcmp (hp->h_addr_list[i], (char *) &client_p->ip,
                   sizeof (struct IN_ADDR)))
        break;

    if (!hp->h_addr_list[i])
    {
      sendto_realops_lev (DEBUG_LEV,
                          "Server IP# Mismatch: %s != %s[%08x]",
                          hp->h_name, inet_ntop (AFINET,
                                                 (char *) &client_p->ip,
                                                 mydummy,
                                                 sizeof (mydummy)),
                          *((unsigned long *) hp->h_addr));
      hp = NULL;
    }
  }
  else if (client_p->hostp)
  {
    hp = client_p->hostp;
    for (i = 0; hp->h_addr_list[i]; i++)
      if (!memcmp (hp->h_addr_list[i], (char *) &client_p->ip,
                   sizeof (struct IN_ADDR)))
        break;
  }

  if (hp)
    /*
     * if we are missing a C or N line from above, search for it
     * under all known hostnames we have for this ip#.
     */
    for (i = 0, name = hp->h_name; name; name = hp->h_aliases[i++])
    {
      strncpyzt (fullname, name, sizeof (fullname));
      add_local_domain (fullname, HOSTLEN - strlen (fullname));
      Debug ((DEBUG_DNS, "sv_cl: gethostbyaddr: %s->%s", sockname, fullname));
      (void) ircsprintf (abuff, "%s@%s", client_p->username, fullname);
      if (!c_conf)
        c_conf = find_conf_host (lp, abuff, CFLAG);
      if (!n_conf)
        n_conf = find_conf_host (lp, abuff, NFLAG);
      if (c_conf && n_conf)
      {
        get_sockhost (client_p, fullname);
        break;
      }
    }

  name = client_p->name;
  /*
   * Check for C and N lines with the hostname portion the ip number
   * of the host the server runs on. This also checks the case where
   * there is a server connecting from 'localhost'.
   */
  if (IsUnknown (client_p) && (!c_conf || !n_conf))
  {
    (void) ircsprintf (abuff, "%s@%s", client_p->username, sockname);
    if (!c_conf)
      c_conf = find_conf_host (lp, abuff, CFLAG);
    if (!n_conf)
      n_conf = find_conf_host (lp, abuff, NFLAG);
  }
  /*
   * Attach by IP# only if all other checks have failed. It is quite
   * possible to get here with the strange things that can happen when
   * using DNS in the way the irc server does. -avalon
   */
  if (!hp)
  {
    if (!c_conf)
      c_conf =
        find_conf_ip (lp, (char *) &client_p->ip, client_p->username, CFLAG);
    if (!n_conf)
      n_conf =
        find_conf_ip (lp, (char *) &client_p->ip, client_p->username, NFLAG);
  }
  else
    for (i = 0; hp->h_addr_list[i]; i++)
    {
      if (!c_conf)
        c_conf = find_conf_ip (lp, hp->h_addr_list[i],
                               client_p->username, CFLAG);
      if (!n_conf)
        n_conf = find_conf_ip (lp, hp->h_addr_list[i],
                               client_p->username, NFLAG);
    }
  /* detach all conf lines that got attached by attach_confs() */
  det_confs_butmask (client_p, 0);
  /* if no C or no N lines, then deny access */
  if (!c_conf || !n_conf)
  {
    get_sockhost (client_p, sockname);
    Debug ((DEBUG_DNS, "sv_cl: access denied: %s[%s@%s] c %x n %x",
            name, client_p->username, client_p->sockhost, c_conf, n_conf));
    return -1;
  }
  /* attach the C and N lines to the client structure for later use. */
  attach_conf (client_p, n_conf);
  attach_conf (client_p, c_conf);
  attach_confs (client_p, name, CONF_HUB | CONF_LEAF | CONF_ULINE);
  /* this may give client_p a new sendq length.. */
  client_p->sendqlen = get_sendq (client_p);

# ifndef INET6
  if ((c_conf->ipnum.S_ADDR == -1))
# else
  if (!memcmp (c_conf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR)))
#endif
    memcpy ((char *) &c_conf->ipnum, (char *) &client_p->ip,
            sizeof (struct IN_ADDR));

  get_sockhost (client_p, c_conf->host);

  Debug ((DEBUG_DNS, "sv_cl: access ok: %s[%s]", name, client_p->sockhost));
  if (estab)
    return m_server_estab (client_p);
  return 0;
}
#endif

#undef	CFLAG
#undef	NFLAG

/*
 * completed_connection
 * Complete non-blocking
 * connect()-sequence. Check access and *       terminate connection,
 * if trouble detected. *
 *
 *      Return  TRUE, if successfully completed *               FALSE,
 * if failed and ClientExit
 */
int
completed_connection (aClient * client_p)
{
  aConfItem *aconf;
  aConfItem *nconf;
  if (!(client_p->flags & FLAGS_BLOCKED))
  {
    unset_fd_flags (client_p->fd, FDF_WANTWRITE);
  }

  unset_fd_flags (client_p->fd, FDF_WANTREAD);
  SetHandshake (client_p);
  aconf = find_conf (client_p->confs, client_p->name, CONF_CONNECT_SERVER);
  if (!aconf)
  {
    sendto_realops ("Lost C-Line for %s", get_client_name (client_p, HIDEME));
    return -1;
  }
  nconf = find_conf (client_p->confs, client_p->name, CONF_NOCONNECT_SERVER);
  if (!nconf)
  {
    sendto_realops ("Lost N-Line for %s", get_client_name (client_p, HIDEME));
    return -1;
  }
  if (!BadPtr (aconf->passwd))
    sendto_one (client_p, "PASS %s :TS", aconf->passwd);
  /* pass on our capabilities to the server we /connect'd */
  /* Ugly ugly ugly ugly - ShadowMaster */
#if defined(HAVE_ENCRYPTION_ON)
  if (!(nconf->port & CAPAB_DODKEY))
    sendto_one (client_p,
                "CAPAB TS5 NOQUIT SSJ5 BURST UNCONNECT ZIP TSMODE NICKIP CLIENT"
#ifdef INET6
                " IPV6"
#endif
      );
  else
    sendto_one (client_p,
                "CAPAB TS5 NOQUIT SSJ5 BURST UNCONNECT DKEY ZIP TSMODE NICKIP CLIENT"
#ifdef INET6
                " IPV6"
#endif
      );
#else
  sendto_one (client_p,
              "CAPAB TS5 NOQUIT SSJ5 BURST UNCONNECT ZIP TSMODE NICKIP CLIENT"
#ifdef INET6
              " IPV6"
#endif
    );
#endif
  aconf = nconf;
  sendto_one (client_p, "SERVER %s 1 :%s",
              my_name_for_link (me.name, aconf), me.info);
  sendto_one (client_p, "NETCTRL 0 0 0 0 0 0 0 0 0 0 0 0 0");
#ifdef DO_IDENTD
  /*
   * Is this the right place to do this?  dunno... -Taner
   */
  if (!IsDead (client_p))
    start_auth (client_p);
#endif
  check_client_fd (client_p);
  return (IsDead (client_p)) ? -1 : 0;
}

/*
 * * close_connection *       Close the physical connection. This function
 * must make *  MyConnect(client_p) == FALSE, and set client_p->from == NULL.
 */
void
close_connection (aClient * client_p)
{
  aConfItem *aconf;
  if (IsServer (client_p))
  {
    ircstp->is_sv++;
    ircstp->is_sbs += client_p->sendB;
    ircstp->is_sbr += client_p->receiveB;
    ircstp->is_sks += client_p->sendK;
    ircstp->is_skr += client_p->receiveK;
    ircstp->is_sti += timeofday - client_p->firsttime;
    if (ircstp->is_sbs > 2047)
    {
      ircstp->is_sks += (ircstp->is_sbs >> 10);
      ircstp->is_sbs &= 0x3ff;
    }
    if (ircstp->is_sbr > 2047)
    {
      ircstp->is_skr += (ircstp->is_sbr >> 10);
      ircstp->is_sbr &= 0x3ff;
    }
  }
  else if (IsClient (client_p))
  {
    ircstp->is_cl++;
    ircstp->is_cbs += client_p->sendB;
    ircstp->is_cbr += client_p->receiveB;
    ircstp->is_cks += client_p->sendK;
    ircstp->is_ckr += client_p->receiveK;
    ircstp->is_cti += timeofday - client_p->firsttime;
    if (ircstp->is_cbs > 2047)
    {
      ircstp->is_cks += (ircstp->is_cbs >> 10);
      ircstp->is_cbs &= 0x3ff;
    }
    if (ircstp->is_cbr > 2047)
    {
      ircstp->is_ckr += (ircstp->is_cbr >> 10);
      ircstp->is_cbr &= 0x3ff;
    }
  }
  else
    ircstp->is_ni++;
  /* 
   * remove outstanding DNS queries.
   */
#ifdef USE_ADNS
  delete_adns_queries (client_p->dnslookup);
#else
  del_queries ((char *) client_p);
#endif
  /*
   * If the connection has been up for a long amount of time, schedule
   * a 'quick' reconnect, else reset the next-connect cycle.
   */
  if ((aconf =
       find_conf_exact (client_p->name,
                        client_p->
                        username, client_p->sockhost, CONF_CONNECT_SERVER)))
  {
    /* 
     * Reschedule a faster reconnect, if this was a automaticly
     * connected configuration entry. (Note that if we have had a
     * rehash in between, the status has been changed to
     * CONF_ILLEGAL). But only do this if it was a "good" link.
     */
    aconf->hold = time (NULL);
    aconf->hold +=
      (aconf->hold - client_p->since >
       HANGONGOODLINK) ? HANGONRETRYDELAY : ConfConFreq (aconf);
    if (nextconnect > aconf->hold)
      nextconnect = aconf->hold;
  }

  if (client_p->authfd >= 0)
  {
    del_fd (client_p->authfd);
#ifdef _WIN32
    closesocket (client_p->authfd);
#else
    close (client_p->authfd);
#endif
    client_p->authfd = -1;
  }

  if (client_p->fd >= 0)
  {
#ifdef USE_SSL
    if (!IsDead (client_p))
#endif
      dump_connections (client_p->fd);
    local[client_p->fd] = NULL;
#ifdef USE_SSL
    if (IsSSL (client_p) && client_p->ssl)
    {
      SSL_set_shutdown (client_p->ssl, SSL_RECEIVED_SHUTDOWN);
      SSL_smart_shutdown (client_p->ssl);
      SSL_free (client_p->ssl);
      client_p->ssl = NULL;
    }
#endif
    del_fd (client_p->fd);
#ifdef _WIN32
    closesocket (client_p->fd);
#else
    close (client_p->fd);
#endif
    client_p->fd = -2;
    DBufClear (&client_p->sendQ);
    DBufClear (&client_p->recvQ);
    memset (client_p->passwd, '\0', sizeof (client_p->passwd));
    /* 
     * clean up extra sockets from P-lines which have been discarded.
     */
    if (client_p->acpt != &me && client_p->acpt != client_p)
    {
      aconf = client_p->acpt->confs->value.aconf;
      if (aconf->clients > 0)
        aconf->clients--;
      if (!aconf->clients && IsIllegal (aconf))
        close_connection (client_p->acpt);
    }
  }
  for (; highest_fd > 0; highest_fd--)
    if (local[highest_fd])
      break;
  det_confs_butmask (client_p, 0);
  client_p->from = NULL;        /* 
                                 * ...this should catch them! >:) --msa
                                 */
  return;
}

#ifdef MAXBUFFERS

/*
 * reset_sock_opts type =  0 = client, 1 = server
 */
void
reset_sock_opts (int fd, int type)
{
# define CLIENT_BUFFER_SIZE	4096
# define SEND_BUF_SIZE		2048
  int opt;
  opt = type ? rcvbufmax : CLIENT_BUFFER_SIZE;
  if (setsockopt (fd, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof (opt)) < 0)
    sendto_realops ("REsetsockopt(SO_RCVBUF) for fd %d (%s) failed", fd,
                    type ? "server" : "client");
  opt = type ? sndbufmax : SEND_BUF_SIZE;
  if (setsockopt (fd, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof (opt)) < 0)
    sendto_realops ("REsetsockopt(SO_SNDBUF) for fd %d (%s) failed", fd,
                    type ? "server" : "client");
}

#endif /*
        * MAXBUFFERS
        */

/*
 * * set_sock_opts
 */
static void
set_sock_opts (int fd, aClient * client_p)
{
  u3_socklen_t opt;
#ifdef SO_REUSEADDR
  opt = 1;
  if (setsockopt
      (fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof (opt)) < 0)
    silent_report_error ("setsockopt(SO_REUSEADDR) %s:%s", client_p);
#endif
#if  defined(SO_DEBUG) && defined(DEBUGMODE) && 0
  /*
   * Solaris with SO_DEBUG writes to syslog by default
   */
# if !defined(SOL20) || defined(USE_SYSLOG)
  opt = 1;
  if (setsockopt (fd, SOL_SOCKET, SO_DEBUG, (char *) &opt, sizeof (opt)) < 0)
    silent_report_error ("setsockopt(SO_DEBUG) %s:%s", client_p);
# endif /*
         * SOL20
         */
#endif
#ifdef	SO_USELOOPBACK
  opt = 1;
  if (setsockopt
      (fd, SOL_SOCKET, SO_USELOOPBACK, (char *) &opt, sizeof (opt)) < 0)
    silent_report_error ("setsockopt(SO_USELOOPBACK) %s:%s", client_p);
#endif
#ifdef	SO_RCVBUF
# if defined(MAXBUFFERS)
  if (rcvbufmax == 0)
  {
    u3_socklen_t optlen;
    optlen = sizeof (rcvbufmax);
    getsockopt (fd, SOL_SOCKET, SO_RCVBUF, (char *) &rcvbufmax,
                (u3_socklen_t *) & optlen);
    while ((rcvbufmax < 16385)
           &&
           (setsockopt
            (fd, SOL_SOCKET, SO_RCVBUF, (char *) (char *) &rcvbufmax,
             optlen) >= 0))
      rcvbufmax += 1024;
    getsockopt (fd, SOL_SOCKET, SO_RCVBUF, (char *) &rcvbufmax,
                (u3_socklen_t *) & optlen);
    readbuf = (char *) malloc (rcvbufmax * sizeof (char));
  }
  if (IsServer (client_p))
    opt = rcvbufmax;
  else
    opt = 4096;
# else
  opt = 8192;
# endif
  if (setsockopt (fd, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof (opt)) < 0)
    silent_report_error ("setsockopt(SO_RCVBUF) %s:%s", client_p);
#endif
#ifdef	SO_SNDBUF
# if defined(MAXBUFFERS)
  if (sndbufmax == 0)
  {
    int optlen;
    optlen = sizeof (sndbufmax);
    getsockopt (fd, SOL_SOCKET, SO_SNDBUF, (char *) &sndbufmax,
                (u3_socklen_t *) & optlen);
    while ((sndbufmax < 16385)
           &&
           (setsockopt
            (fd, SOL_SOCKET, SO_SNDBUF, (char *) &sndbufmax, optlen) >= 0))
      sndbufmax += 1024;
    getsockopt (fd, SOL_SOCKET, SO_SNDBUF, (char *) &sndbufmax,
                (u3_socklen_t *) & optlen);
  }
  if (IsServer (client_p))
    opt = sndbufmax;
  else
    opt = 4096;
# else
  opt = 8192;
# endif
  if (setsockopt (fd, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof (opt)) < 0)
    silent_report_error ("setsockopt(SO_SNDBUF) %s:%s", client_p);
#endif
# if defined(IP_OPTIONS) && defined(IPPROTO_IP) && !defined(INET6)
  {
#  if defined(MAXBUFFERS)
    char *s = readbuf, *t = readbuf + (rcvbufmax * sizeof (char)) / 2;
    opt = (rcvbufmax * sizeof (char)) / 8;
#  else
    char *s = readbuf, *t = readbuf + sizeof (readbuf) / 2;
    opt = sizeof (readbuf) / 8;
#  endif
    if (getsockopt (fd, IPPROTO_IP, IP_OPTIONS, t, &opt) < 0)
      silent_report_error ("getsockopt(IP_OPTIONS) %s:%s", client_p);
    else if (opt > 0)
    {
      for (*readbuf = '\0'; opt > 0; opt--, s += 3)
        (void) ircsprintf (s, "%02.2x:", *t++);
      *s = '\0';
      sendto_realops
        ("Connection %s using IP opts: (%s)",
         get_client_name (client_p, HIDEME), readbuf);
    }
    if (setsockopt (fd, IPPROTO_IP, IP_OPTIONS, (char *) NULL, 0) < 0)
      silent_report_error ("setsockopt(IP_OPTIONS) %s:%s", client_p);
  }
# endif
}


int
get_sockerr (aClient * client_p)
{
  int errtmp = errno, err = 0;
  u3_socklen_t len = sizeof (err);
#ifdef	SO_ERROR
  if (client_p->fd >= 0)
    if (!getsockopt
        (client_p->fd, SOL_SOCKET, SO_ERROR, (char *) &err,
         (u3_socklen_t *) & len))
      if (err)
        errtmp = err;
#endif
  return errtmp;
}

char *
irc_get_sockerr (aClient * client_p)
{
  if (client_p->sockerr == 0)
    return "No error";
  if (client_p->sockerr > 0)
    return strerror (client_p->sockerr);
  switch (client_p->sockerr)
  {
     case -1:                  /* this is the default */
       return "Unset error message [this is a bug!]";
     case IRCERR_BUFALLOC:
       return "dbuf allocation error";
     case IRCERR_ZIP:
       return "compression general failure";
     default:
       return "Unknown error!";
  }

  /* unreachable code, but the compiler is complaining.. */
  return NULL;
}


/*
 * * set_non_blocking 
 *       Set the client connection into non-blocking mode. 
 */

void
set_non_blocking (int fd, aClient * client_p)
{
  int res, nonb = 0;
#ifdef _WIN32
  nonb = 1;
  ioctlsocket (fd, FIONBIO, (u_long FAR *) & nonb);
#else
  nonb |= O_NONBLOCK;

  if ((res = fcntl (fd, F_GETFL, 0)) == -1)
    silent_report_error ("fcntl(fd, F_GETFL) failed for %s:%s", client_p);
  else if (fcntl (fd, F_SETFL, res | nonb) == -1)
    silent_report_error
      ("fcntl(fd, F_SETL, nonb) failed for %s:%s", client_p);
#endif
  return;
}

/*
 * Creates a client which has just connected to us on the given fd. The
 * sockhost field is initialized with the ip# of the host. The client
 * is added to the linked list of clients but isnt added to any hash
 * tables yuet since it doesnt have a name.
 */
aClient *
add_connection (aClient * client_p, int fd)
{
#ifndef USE_ADNS
  Link lin;
#endif
  aClient *target_p;
  aConfItem *aconf = NULL;
  char *s, *t;
#ifdef INET6
  char *e;
  char sock[IPLEN + 1], sock2[IPLEN + 1];
#endif
  u3_socklen_t len;
  struct SOCKADDR_IN addr;
  struct userBan *ban;

  target_p = make_client (NULL, &me);
  if (client_p != &me)
    aconf = client_p->confs->value.aconf;
  len = sizeof (struct SOCKADDR_IN);
  if (getpeername (fd, (struct SOCKADDR *) &addr, (u3_socklen_t *) & len) ==
      -1)
  {
    /* report_error("Failed in connecting to %s :%s", client_p);        */
    ircstp->is_ref++;
    target_p->fd = -2;
    free_client (target_p);
#ifdef _WIN32
    closesocket (fd);
#else
    close (fd);
#endif
    return NULL;
  }
  /*
   * don't want to add "Failed in connecting to" here..
   */
  if (aconf && IsIllegal (aconf))
  {
    ircstp->is_ref++;
    target_p->fd = -2;
    free_client (target_p);
#ifdef _WIN32
    closesocket (fd);
#else
    close (fd);
#endif
    return NULL;
  }
  /*
   * Copy ascii address to 'sockhost' just in case. Then we have
   * something valid to put into error messages...
   */

  inet_ntop (AFINET, (char *) &addr.SIN_ADDR, mydummy, sizeof (mydummy));
#ifdef INET6
  ip6_expand (mydummy, sizeof (mydummy));
#endif
  get_sockhost (target_p, mydummy);
  memcpy ((char *) &target_p->ip,
          (char *) &addr.SIN_ADDR, sizeof (struct IN_ADDR));
  target_p->port = ntohs (addr.SIN_PORT);
  /*
   * Check that this socket (client) is allowed to accept
   * connections from this IP#.
   */
#ifndef INET6
  for (s = (char *) &client_p->ip, t =
       (char *) &target_p->ip, len = IPLEN; len > 0; len--, s++, t++)
  {
    if (!*s)
      continue;
    if (*s != *t)
      break;
  }
#else
  /*
   * comparing the sockhost now.
   * len is used as a state discriptor
   *  - '\0' means that everything is fine ! = '\0' means 'kill'
   * We assume its a hostname if the client_p->sockhost contains no '*'
   * notice: client_p->sockhost is the first parameter of the P:Line
   * Its only enabled for ipv6 since this fix is a bit experimental.
   * - Againaway
   */
  strcpy (sock, client_p->sockhost);
  if ((e = strchr (sock, '*')) != NULL)
  {
    *e = '\0';
    len = strlen (sock);
    strncpy (sock2, target_p->sockhost, len);
    if (strncmp (sock, sock2, len))
    {
      len = 1;
    }
    else
    {
      len = '\0';
    }
  }
  else
  {
    len = '\0';
  }
#endif

  if (len)
  {
    ircstp->is_ref++;
    target_p->fd = -2;
    free_client (target_p);
#ifdef _WIN32
    closesocket (fd);
#else
    close (fd);
#endif
    return NULL;
  }

#ifdef USE_SSL
  if (IsSSL (client_p))
  {
    extern SSL_CTX *ircdssl_ctx;
    target_p->ssl = NULL;
    /*SSL client init. */
    if ((target_p->ssl = SSL_new (ircdssl_ctx)) == NULL)
    {
      sendto_realops_lev (DEBUG_LEV,
                          "SSL creation of "
                          "new SSL object failed [client %s]",
                          target_p->sockhost);
      ircstp->is_ref++;
      target_p->fd = -2;
      free_client (target_p);
#ifdef _WIN32
      closesocket (fd);
#else
      close (fd);
#endif
      return NULL;
    }

    SetSSL (target_p);
    set_non_blocking (fd, target_p);
    set_sock_opts (fd, target_p);
    SSL_set_fd (target_p->ssl, fd);
    if (!safe_SSL_accept (target_p, fd))
    {
      SSL_set_shutdown (target_p->ssl, SSL_RECEIVED_SHUTDOWN);
      SSL_smart_shutdown (target_p->ssl);
      SSL_free (target_p->ssl);
      ircstp->is_ref++;
      target_p->fd = -2;
      free_client (target_p);
#ifdef _WIN32
      closesocket (fd);
#else
      close (fd);
#endif
      return NULL;
    }
  }
#endif

  add_fd (fd, FDT_CLIENT, target_p);
  local[fd] = target_p;
  if (aconf)
  {
    aconf->clients++;
  }

  target_p->fd = fd;
  if (fd > highest_fd)
  {
    highest_fd = fd;
  }

  target_p->acpt = client_p;
  add_client_to_list (target_p);
#ifdef USE_SSL
  if (!IsSSL (target_p))
  {
#endif
    set_non_blocking (target_p->fd, target_p);
    set_sock_opts (target_p->fd, target_p);
#ifdef USE_SSL
  }
#endif

  inet_ntop (AFINET, &target_p->ip, target_p->hostip, HOSTIPLEN + 1);
  if (!find_matching_conf (&EList1, target_p->hostip))
  {
    ban =
      check_userbanned (target_p, UBAN_IP | UBAN_CIDR4 | UBAN_WILDUSER, 0);
    if (ban)
    {
      char *reason, *ktype;
      int lcl;
      lcl = (ban->flags & UBAN_LOCAL) ? 1 : 0;
      ktype = local ? LOCAL_BANNED_NAME : NETWORK_BANNED_NAME;
      reason = ban->reason ? ban->reason : ktype;
      sendto_one (target_p,
                  err_str (ERR_YOUREBANNEDCREEP), me.name, "*", ktype);
      sendto_one (target_p,
                  ":%s NOTICE * :*** You are not welcome on this %s.",
                  me.name, lcl ? "server" : "network");
      sendto_one (target_p,
                  ":%s NOTICE * :*** %s for %s", me.name, ktype, reason);
      sendto_one (target_p, ":%s NOTICE * :*** Your IP is %s", me.name,
                  target_p->hostip);
      sendto_one (target_p,
                  ":%s NOTICE * :*** For assistance, please email %s and "
                  "include everything shown here.", me.name,
                  lcl ? SERVER_KLINE_ADDRESS : NETWORK_KLINE_ADDRESS);
      ircstp->is_ref++;
      ircstp->is_ref_1++;
#ifdef THROTTLE_ENABLE
      throttle_force (target_p->hostip);
#endif
      exit_client (target_p, target_p, &me, reason);
      return NULL;
    }
  }

#ifdef SHOW_HEADERS
#ifdef USE_SSL
  if (!IsSSL (target_p))
  {
#endif
    sendto_one (target_p, REPORT_DO_DNS);
#ifdef USE_SSL
  }
#endif
#endif

#ifdef USE_ADNS
  Debug ((DEBUG_DNS, "lookup %s",
          inet_ntop (AFINET,
                     (char *) &addr.SIN_ADDR, mydummy, sizeof (mydummy))));
  SetDNS (target_p);
  target_p->dnslookup = malloc (sizeof (struct DNSQuery));
  target_p->dnslookup->ptr = target_p;
  target_p->dnslookup->callback = got_client_dns;
  /* XXX need to change AF_INET to represent either ipv4 or ipv6 */
  adns_getaddr ((struct in_addr)
                target_p->ip, AF_INET, target_p->dnslookup, 0);
#else
  lin.flags = ASYNC_CLIENT;
  lin.value.client_p = target_p;

  Debug ((DEBUG_DNS, "lookup %s", inet_ntop (AFINET, (char *)
                                             &addr.SIN_ADDR, mydummy,
                                             sizeof (mydummy))));

  target_p->hostp = gethost_byaddr ((char *) &target_p->ip, &lin);
  if (!target_p->hostp)
  {
    SetDNS (target_p);
  }
#ifdef SHOW_HEADERS
  else
  {
#ifdef USE_SSL
    if (!IsSSL (target_p))
    {
#endif
      sendto_one (target_p, REPORT_FIN_DNSC);
#ifdef USE_SSL
    }
#endif
  }
#endif
#endif
  nextdnscheck = 1;

#ifdef DO_IDENTD
  start_auth (target_p);
#endif
#ifdef INET6
  client_p->flags |= FLAGS_IPV6;
#endif
  check_client_fd (target_p);
  return target_p;
}

/* handle taking care of the client's recvq here */
int
do_client_queue (aClient * client_p)
{
  int dolen = 0, done;
  while (DBufLength (&client_p->recvQ)
         && !NoNewLine (client_p)
         &&
         ((client_p->status <
           STAT_UNKNOWN)
          || (client_p->since - timeofday < 10) || IsNegoServer (client_p)))
  {
    /* If it's become registered as a server, just parse the whole block */
    if (IsServer (client_p) || IsNegoServer (client_p))
    {
#if defined(MAXBUFFERS)
      dolen = dbuf_get (&client_p->recvQ, readbuf, rcvbufmax * sizeof (char));
#else
      dolen = dbuf_get (&client_p->recvQ, readbuf, sizeof (readbuf));
#endif
      if (dolen <= 0)
        break;
      if ((done = dopacket (client_p, readbuf, dolen)))
        return done;
      break;
    }

#if defined(MAXBUFFERS)
    dolen =
      dbuf_getmsg (&client_p->recvQ, readbuf, rcvbufmax * sizeof (char));
#else
    dolen = dbuf_getmsg (&client_p->recvQ, readbuf, sizeof (readbuf));
#endif
    if (dolen <= 0)
    {
      if (dolen < 0)
        return exit_client (client_p, client_p, client_p, "dbuf_getmsg fail");
      if (DBufLength (&client_p->recvQ) < 510)
      {
        client_p->flags |= FLAGS_NONL;
        break;
      }
      /* The buffer is full (more than 512 bytes) and it has no \n
       * Some user is trying to trick us. Kill their recvq. */
      DBufClear (&client_p->recvQ);
      break;
    }
    else if (client_dopacket (client_p, readbuf, dolen) == FLUSH_BUFFER)
      return FLUSH_BUFFER;
  }

  if (!(client_p->flags & FLAGS_HAVERECVQ)
      && DBufLength (&client_p->recvQ) && !NoNewLine (client_p))
  {
    add_to_list (&recvq_clients, client_p);
    client_p->flags |= FLAGS_HAVERECVQ;
  }

  return 1;
}

/*
 * read_packet
 *
 * Read a 'packet' of data from a connection and process it.  Read in 8k 
 * chunks to give a better performance rating (for server connections). 
 * Do some tricky stuff for client connections to make sure they don't
 * do any flooding >:-) -avalon
 */

#define MAX_CLIENT_RECVQ 8192   /* 4 dbufs */
#ifdef USE_SSL
#define RECV2(from, buf, len)   IsSSL(client_p) ? \
	                        safe_SSL_read(from, buf, len) : \
				RECV(from->fd, buf, len)
#else
#define RECV2(from, buf, len)   RECV(from->fd, buf, len)
#endif

int
read_packet (aClient * client_p)
{
  int length = 0, done;
  /* If data is ready, and the user is either not a person or
   * is a person and has a recvq of less than MAX_CLIENT_RECVQ,
   * read from this client
   */
  if (!
      (IsPerson (client_p)
       && DBufLength (&client_p->recvQ) > MAX_CLIENT_RECVQ))
  {
    errno = 0;
#if defined ( MAXBUFFERS )
    if (IsPerson (client_p))
      length = RECV2 (client_p, readbuf, 8192 * sizeof (char));
    else
      length = RECV2 (client_p, readbuf, rcvbufmax * sizeof (char));
#else /*MAXBUFFERS*/
      length = RECV2 (client_p, readbuf, sizeof (readbuf));
#endif /*MAXBUFFERS*/
#ifdef USE_REJECT_HOLD
      /* 
       * If client has been marked as rejected i.e. it is a client that
       * is trying to connect again after a k-line, pretend to read it
       * but don't actually. -Dianora
       */
      if (client_p->flags & FLAGS_REJECT_HOLD)
    {
      if ((client_p->firsttime + REJECT_HOLD_TIME) > timeofday)
        exit_client (client_p, client_p, client_p, "reject held client");
      else
        return 1;
    }
#endif

    client_p->lasttime = timeofday;
    if (client_p->lasttime > client_p->since)
      client_p->since = client_p->lasttime;
    client_p->flags &= ~(FLAGS_PINGSENT | FLAGS_NONL);
    /*
     * If not ready, fake it so it isnt closed
     */
    if (length == -1 &&
#ifdef _WIN32
        (WSAGetLastError () == WSAEWOULDBLOCK)
#else
        ((errno == EWOULDBLOCK) || (errno == EAGAIN))
#endif
      )
      return 1;
    if (length <= 0)
    {
      client_p->sockerr = length ? errno : 0;
      return length;
    }
  }

  /* 
   * For server connections, we process as many as we can without
   * worrying about the time of day or anything :)
   */
  if (IsServer (client_p)
      || IsConnecting (client_p)
      || IsHandshake (client_p) || IsNegoServer (client_p)
#ifdef OPER_CAN_FLOOD
      || IsOper (client_p)
#endif
    )
  {
    if (length > 0)
      if ((done = dopacket (client_p, readbuf, length)))
        return done;
  }
  else
  {
    /* 
     * Before we even think of parsing what we just read, stick 
     * it on the end of the receive queue and do it when its turn
     * comes around. */
    if (dbuf_put (&client_p->recvQ, readbuf, length) < 0)
      return exit_client (client_p, client_p, client_p, "dbuf_put fail");
    if (IsPerson (client_p) &&
#ifdef NO_OPER_FLOOD
        !IsAnOper (client_p) &&
#endif
        DBufLength (&client_p->recvQ) > CLIENT_FLOOD)
    {
      sendto_realops_lev (FLOOD_LEV,
                          "Flood -- %s!%s@%s (%d) Exceeds %d RecvQ",
                          client_p->name[0] ? client_p->name : "*",
                          client_p->user ? client_p->user->
                          username : "*",
                          client_p->user ? client_p->user->host : "*",
                          DBufLength (&client_p->recvQ), CLIENT_FLOOD);
      return exit_client (client_p, client_p, client_p, "Excess Flood");
    }
    return do_client_queue (client_p);
  }
  return 1;
}

void
read_error_exit (aClient * client_p, int length, int err)
{
  char fbuf[512];
  char errmsg[512];
  if (IsServer (client_p)
      || IsHandshake (client_p) || IsConnecting (client_p))
  {
    if (length == 0)
    {
      char *errtxt = "Server %s closed the connection";
      ircsprintf (fbuf, "from %s: %s", me.name, errtxt);
      sendto_gnotice (fbuf, get_client_name (client_p, HIDEME));
      ircsprintf (fbuf, ":%s GNOTICE :%s", me.name, errtxt);
      sendto_serv_butone (client_p, fbuf, get_client_name (client_p, HIDEME));
    }
    else
    {
      char *errtxt = (IsConnecting (client_p)
                      ||
                      IsHandshake (client_p))
        ? "Connect error to %s (%s)" :
        "Read error from %s, closing link (%s)";
      ircsprintf (fbuf, "from %s: %s", me.name, errtxt);
      sendto_gnotice (fbuf,
                      get_client_name (client_p, HIDEME), strerror (err));
      ircsprintf (fbuf, ":%s GNOTICE :%s", me.name, errtxt);
      sendto_serv_butone (client_p, fbuf,
                          get_client_name (client_p, HIDEME), strerror (err));
    }
  }

  if (err)
    ircsprintf (errmsg, "Read error: %s", strerror (err));
  else
    ircsprintf (errmsg, "Client closed connection");
  exit_client (client_p, client_p, &me, errmsg);
}

/* windows doesn't have socklen_t, its just an int to them */
void
accept_connection (aClient * client_p)
{
  static struct SOCKADDR_IN addr;
  u3_socklen_t addrlen = sizeof (struct SOCKADDR_IN);
  char host[HOSTLEN + 2];
  int newfd;
  int i;
  client_p->lasttime = timeofday;
  for (i = 0; i < 100; i++)     /* accept up to 100 times per call to deal with high connect rates */
  {
    if ((newfd =
         accept (client_p->fd, (struct SOCKADDR *) &addr,
                 (u3_socklen_t *) & addrlen)) < 0)
    {
      switch (errno)
      {
#ifdef EMFILE
         case EMFILE:
           report_error ("Cannot accept connections %s:%s", client_p);
           break;
#endif
#ifdef ENFILE
         case ENFILE:
           report_error ("Cannot accept connections %s:%s", client_p);
           break;
#endif
      }
      return;
    }

    inet_ntop (AFINET, (char *) &addr.SIN_ADDR, host, sizeof (host));
#ifdef INET6
    ip6_expand (host, sizeof (host));
#endif
#ifdef THROTTLE_ENABLE
/* if they are throttled, drop them silently. */
    if (throttle_check (host, newfd, NOW) == 0)
    {
      ircstp->is_ref++;
      ircstp->is_throt++;
#ifdef _WIN32
      closesocket (newfd);
#else
      close (newfd);
#endif
      return;
    }
#endif

    if (newfd >= HARD_FDLIMIT - 10)
    {
      ircstp->is_ref++;
      sendto_realops_lev (CCONN_LEV,
                          "All connections in use. fd: %d (%s)",
                          newfd, get_client_name (client_p, HIDEME));
      send (newfd, "ERROR :All connections in use\r\n", 32, 0);
#ifdef _WIN32
      closesocket (newfd);
#else
      close (newfd);
#endif
      return;
    }
    ircstp->is_ac++;
    add_connection (client_p, newfd);
#ifdef PINGNAZI
    nextping = timeofday;
#endif
    if (!client_p->acpt)
      client_p->acpt = &me;
  }
}

int
readwrite_client (aClient * client_p, int isread, int iswrite)
{
  /*
   * NOTE
   * We now do this in a more logical way.
   * We request a write poll on a socket for two reasons
   * - the socket is waiting for a connect() call
   * - the socket is blocked
   */

  /*
   * Ugly SSL kludge.
   */
#ifdef USE_SSL
  if (client_p->ssl && !SSL_is_init_finished (client_p->ssl))
  {
    if (IsDead (client_p) || (!safe_SSL_accept (client_p, client_p->fd)))
    {
      close_connection (client_p);
      return 0;
    }
    /*
     * SSL have not yet finished init, we cant read/write yet.
     */
    return 1;
  }
#endif

  if (iswrite)
  {
    if (IsConnecting (client_p) && completed_connection (client_p))
    {
      char errmsg[512];
      ircsprintf (errmsg, "Connect Error: %s", irc_get_sockerr (client_p));
      return exit_client (client_p, client_p, &me, errmsg);
    }

    if (client_p->flags & FLAGS_BLOCKED)
    {
      client_p->flags &= ~FLAGS_BLOCKED;
      unset_fd_flags (client_p->fd, FDF_WANTWRITE);
    }
    else
    {
      /* this may be our problem with occational 100% cpu looping     
       * we've experienced.  jason suggested this, here we will try   
       * this and see if it happens at all -epi */

      sendto_realops_lev (DEBUG_LEV, "Removing socket %d: reported ready"
                          " for write, but not blocking", client_p->fd);
      /* This unset_fd_flags() does not appear to make any difference 
       * to the write set.  The socket appears stuck, and there has   
       * to be a reason for it.  Since we're experiencing a very low  
       * number of these, simply drop the client entirely, and treat  
       * this as a socket handling error.  This is essentially a kludge       
       * however tracking down this bug will take a serious amount of 
       * time and testing - since its not easily reproducable.  This  
       * will in the meantime prevent maxing the CPU.  -epi   
       *      
       * unset_fd_flags(cptr->fd, FDF_WANTWRITE);     
       */
      exit_client (client_p, client_p, &me, "Socket error (write)");
      return FLUSH_BUFFER;
    }
  }

  if (isread)
  {
    int length = read_packet (client_p);
    if (length == FLUSH_BUFFER)
      return length;
    if (length <= 0)
    {
      read_error_exit (client_p, length, client_p->sockerr);
      return FLUSH_BUFFER;
    }
  }

  if (IsDead (client_p))
  {
    char errmsg[512];
    ircsprintf (errmsg, "Write Error: %s",
                (client_p->
                 flags & FLAGS_SENDQEX) ?
                "SendQ Exceeded" : irc_get_sockerr (client_p));
    return exit_client (client_p, client_p, &me, errmsg);
  }

  return 1;
}


/*
 * connect_server
 */
int
#ifdef USE_ADNS
connect_server (aConfItem * aconf, aClient * by)
#else
connect_server (aConfItem * aconf, aClient * by, struct hostent *hp)
#endif
{
  struct SOCKADDR *svp;
  aClient *client_p, *c2ptr;
  int errtmp, len;
  char *s;
  Debug ((DEBUG_NOTICE,
          "Connect to %s[%s] @%s",
          aconf->name, aconf->host,
          inet_ntop (AFINET, &aconf->ipnum, mydummy, sizeof (mydummy))));
  if ((c2ptr = find_server (aconf->name, NULL)))
  {
    sendto_ops
      ("Server %s already present from %s",
       aconf->name, get_client_name (c2ptr, HIDEME));
    if (by && IsPerson (by) && !MyClient (by))
      sendto_one (by,
                  ":%s NOTICE %s :Server %s already present from %s",
                  me.
                  name,
                  by->name, aconf->name, get_client_name (c2ptr, HIDEME));
    return -1;
  }
  /*
   * If we dont know the IP# for this host and itis a hostname and not
   * a ip# string, then try and find the appropriate host record.
   */

  /* with the new adns, if we don't have a IP when this is called, 
   * call adns_getaddr and set the callback to adns_connect_server,
   * which will complete the connect for us I hope. 
   * - Fish
   */
#ifndef INET6
  if ((!aconf->ipnum.S_ADDR))
#else
  if (!memcmp (aconf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR)))
#endif
  {
#ifdef USE_ADNS
    aconf->dnslookup = malloc (sizeof (struct DNSQuery));
    aconf->dnslookup->ptr = aconf;
    aconf->dnslookup->callback = got_async_connect_dns;
    /* XXX need to change AF_INET to represent either ipv4 or ipv6 */
    if ((s = strchr (aconf->host, '@')))
      s++;
    else
      s = aconf->host;

    adns_gethost (s, AF_INET, aconf->dnslookup);
    Debug ((DEBUG_DEBUG,
            "Calling adns_gethost for Server Connect for %s", aconf->name));
    return 0;
  }

  client_p = make_client (NULL, &me);
/* XXXX */
#if 0
  /* this should be copied from the aconf I guess */
  client_p->hostp = hp;

#endif
#else /* USE_ADNS */
    Link lin;

    lin.flags = ASYNC_CONNECT;
    lin.value.aconf = aconf;
    nextdnscheck = 1;
    s = (char *) strchr (aconf->host, '@');
    s++;                        /*
                                 * should NEVER be NULL
                                 */
#ifndef INET6
    /*
       if ((aconf->ipnum.S_ADDR = inet_addr(s)) == -1)
     */
    if (inet_pton (AFINET, s, &aconf->ipnum.S_ADDR) < 0)
    {
      aconf->ipnum.S_ADDR = 0;
#else
    if (inet_pton (AFINET, s, aconf->ipnum.S_ADDR) < 0)
    {
      memcpy (aconf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR));
#endif
      hp = gethost_byname (s, &lin);
      Debug ((DEBUG_NOTICE, "co_sv: hp %x ac %x na %s ho %s",
              hp, aconf, aconf->name, s));
      if (!hp)
        return 0;
      memcpy ((char *) &aconf->ipnum, hp->h_addr, sizeof (struct IN_ADDR));
    }
  }
  client_p = make_client (NULL, &me);
  client_p->hostp = hp;
#endif
  /*
   * Copy these in so we have something for error detection.
   */
  strncpyzt (client_p->name, aconf->name, sizeof (client_p->name));
  strncpyzt (client_p->sockhost, aconf->host, HOSTLEN + 1);
  svp = connect_inet (aconf, client_p, &len);
  if (!svp)
  {
    if (client_p->fd != -1)
#ifdef _WIN32
      closesocket (client_p->fd);
#else
      (void) close (client_p->fd);
#endif
    client_p->fd = -2;
    free_client (client_p);
    return -1;
  }

  set_non_blocking (client_p->fd, client_p);
  set_sock_opts (client_p->fd, client_p);
#ifndef _WIN32
  (void) signal (SIGALRM, dummy);
  if (connect (client_p->fd, svp, len) < 0 && errno != EINPROGRESS)
  {
    errtmp = errno;
#else
  if (connect (client_p->fd, svp, len) < 0 &&
      WSAGetLastError () != WSAEINPROGRESS &&
      WSAGetLastError () != WSAEWOULDBLOCK)
  {
    errtmp = WSAGetLastError ();
#endif
    /* 
     * other system calls may eat errno
     */
    report_error ("Connect to host %s failed: %s", client_p);
    if (by && IsPerson (by) && !MyClient (by))
      sendto_one (by,
                  ":%s NOTICE %s :Connect to server %s failed.",
                  me.name, by->name, client_p->name);
#ifdef _WIN32
    closesocket (client_p->fd);
#else
    (void) close (client_p->fd);
#endif
    client_p->fd = -2;
    free_client (client_p);
#ifndef _WIN32
    errno = errtmp;
    if (errno == EINTR)
      errno = ETIMEDOUT;
#else
    WSASetLastError (errtmp);
    if (errtmp == WSAEINTR)
      WSASetLastError (WSAETIMEDOUT);
#endif
    return -1;
  }
  /* 
   * Attach config entries to client here rather than in
   * completed_connection. This to avoid null pointer references when
   * name returned by gethostbyaddr matches no C lines (could happen
   * in 2.6.1a when host and servername differ). No need to check
   * access and do gethostbyaddr calls. There must at least be one as
   * we got here C line...  meLazy
   */
  (void) attach_confs_host (client_p,
                            aconf->host,
                            CONF_NOCONNECT_SERVER | CONF_CONNECT_SERVER);
  if (!find_conf_host
      (client_p->confs, aconf->host,
       CONF_NOCONNECT_SERVER)
      || !find_conf_host (client_p->confs, aconf->host, CONF_CONNECT_SERVER))
  {
    sendto_ops
      ("Server %s is not enabled for connecting:no C/N-line", aconf->name);
    if (by && IsPerson (by) && !MyClient (by))
      sendto_one (by,
                  ":%s NOTICE %s :Connect to server %s failed.",
                  me.name, by->name, client_p->name);
    det_confs_butmask (client_p, 0);
#ifdef _WIN32
    closesocket (client_p->fd);
#else
    (void) close (client_p->fd);
#endif
    client_p->fd = -2;
    free_client (client_p);
    return (-1);
  }
  /* 
   * * The socket has been connected or connect is in progress.
   */
  (void) make_server (client_p);
  if (by && IsPerson (by))
  {
    strcpy (client_p->serv->bynick, by->name);
    strcpy (client_p->serv->byuser, by->user->username);
    strcpy (client_p->serv->byhost, by->user->host);
  }
  else
  {
    strcpy (client_p->serv->bynick, "AutoConn.");
    *client_p->serv->byuser = '\0';
    *client_p->serv->byhost = '\0';
  }
  client_p->serv->up = me.name;
  if (client_p->fd > highest_fd)
    highest_fd = client_p->fd;
  local[client_p->fd] = client_p;
  client_p->acpt = &me;
  SetConnecting (client_p);
  /* sendq probably changed.. */
  client_p->sendqlen = get_sendq (client_p);
  get_sockhost (client_p, aconf->host);
  add_client_to_list (client_p);
#ifdef PINGNAZI
  nextping = timeofday;
#endif
  add_fd (client_p->fd, FDT_CLIENT, client_p);
  client_p->flags |= FLAGS_BLOCKED;
  set_fd_flags (client_p->fd, FDF_WANTREAD | FDF_WANTWRITE);
  return 0;
}

static struct SOCKADDR *
connect_inet (aConfItem * aconf, aClient * client_p, int *lenp)
{
  static struct SOCKADDR_IN server;
#ifndef USE_ADNS
  struct hostent *hp;
#endif
  struct SOCKADDR_IN scin;
  char *s;
  /*
   * Might as well get sockhost from here, the connection is attempted
   * with it so if it fails its useless.
   */
  client_p->fd = socket (AFINET, SOCK_STREAM, 0);
  if (client_p->fd >= (HARD_FDLIMIT - 10))
  {
    sendto_realops ("No more connections allowed (%s)", client_p->name);
    return NULL;
  }

  memset ((char *) &server, '\0', sizeof (server));
  memset ((char *) &scin, '\0', sizeof (scin));
  server.SIN_FAMILY = scin.SIN_FAMILY = AFINET;
  get_sockhost (client_p, aconf->host);
  if (aconf->localhost)
#ifndef INET6
    /*
       scin.SIN_ADDR.S_ADDR = inet_addr(aconf->localhost);
     */
    inet_pton (AFINET,
               (void *) aconf->localhost, (void *) &scin.SIN_ADDR.S_ADDR);
#else
    inet_pton (AFINET,
               (void *) aconf->localhost, (void *) scin.SIN_ADDR.S_ADDR);
#endif
  else if (specific_virtual_host == 1)
  {
    memcpy (&scin.SIN_ADDR, &vserv.SIN_ADDR, sizeof (scin.SIN_ADDR));
  }

  if (client_p->fd == -1)
  {
    report_error ("opening stream socket to server %s:%s", client_p);
    return NULL;
  }
  /*
   * * Bind to a local IP# (with unknown port - let unix decide) so *
   * we have some chance of knowing the IP# that gets used for a host *
   * with more than one IP#.
   */
  /* 
   * No we don't bind it, not all OS's can handle connecting with an
   * already bound socket, different ip# might occur anyway leading to
   * a freezing select() on this side for some time.
   */
  if (specific_virtual_host || aconf->localhost)
  {
    /* 
     * * No, we do bind it if we have virtual host support. If we
     * don't * explicitly bind it, it will default to IN_ADDR_ANY and
     * we lose * due to the other server not allowing our base IP
     * --smg
     */
    if (bind (client_p->fd, (struct SOCKADDR *) &scin, sizeof (scin)) == -1)
    {
      report_error ("error binding to local port for %s:%s", client_p);
      return NULL;
    }
  }
  /* 
   * By this point we should know the IP# of the host listed in the
   * conf line, whether as a result of the hostname lookup or the ip#
   * being present instead. If we dont know it, then the connect
   * fails.
   */
  s = strchr (aconf->host, '@');
  s == NULL ? s = aconf->host : ++s;
#ifndef INET6
  if (inet_pton (AFINET, s, &aconf->ipnum.S_ADDR) < 0)
  {
    aconf->ipnum.S_ADDR = -1;
#else
  if (inet_pton (AFINET, s, aconf->ipnum.S_ADDR) < 0)
  {
    memcpy (aconf->ipnum.S_ADDR, minus_one, sizeof (struct IN_ADDR));
#endif
#ifdef USE_ADNS
    if (!client_p->ip.S_ADDR)
#else
    hp = client_p->hostp;
    if (!hp)
#endif
    {
      Debug ((DEBUG_FATAL, "%s: unknown host", aconf->host));
      return NULL;
    }
#ifdef USE_ADNS
    aconf->ipnum.S_ADDR = client_p->ip.S_ADDR;
#else
    memcpy ((char *) &aconf->ipnum, hp->h_addr, sizeof (struct IN_ADDR));
#endif
  }
  memcpy ((char *) &server.SIN_ADDR,
          (char *) &aconf->ipnum, sizeof (struct IN_ADDR));
  memcpy ((char *) &client_p->ip,
          (char *) &aconf->ipnum, sizeof (struct IN_ADDR));
  server.SIN_PORT = htons ((aconf->port > 0) ? aconf->port : portnum);
  *lenp = sizeof (server);
  return (struct SOCKADDR *) &server;
}

/*
 * * find the real hostname for the host running the server (or one
 * which * matches the server's name) and its primary IP#.  Hostname is
 * stored * in the client structure passed as a pointer.
 */
void
get_my_name (aClient * client_p, char *name, int len)
{
  static char tmp[HOSTLEN + 1];
  struct hostent *hp;
  /* 
   * The following conflicts with both AIX and linux prototypes oh
   * well, we can put up with the errors from other systems -Dianora
   */
  /* 
   * extern int        gethostname(char *, int);
   */
  char *cname = client_p->name;
  /* 
   * * Setup local socket structure to use for binding to.
   */
  memset ((char *) &mysk, '\0', sizeof (mysk));
  mysk.SIN_FAMILY = AFINET;
  if (gethostname (name, len) == -1)
    return;
  name[len] = '\0';
  /* 
   * assume that a name containing '.' is a FQDN
   */
  if (!strchr (name, '.'))
    add_local_domain (name, len - strlen (name));
  /*
   * * If hostname gives another name than cname, then check if there
   * is * a CNAME record for cname pointing to hostname. If so accept *
   * cname as our name.   meLazy
   */
  if (BadPtr (cname))
    return;
  if ((hp = gethostbyname (cname)) || (hp = gethostbyname (name)))
  {
    char *hname;
    int i = 0;
    for (hname = hp->h_name; hname; hname = hp->h_aliases[i++])
    {
      strncpyzt (tmp, hname, sizeof (tmp));
      add_local_domain (tmp, sizeof (tmp) - strlen (tmp));
      /* 
       * * Copy the matching name over and store the * 'primary' IP#
       * as 'myip' which is used * later for making the right one is
       * used * for connecting to other hosts.
       */
      if (irccmp (me.name, tmp) == 0)
        break;
    }
    if (irccmp (me.name, tmp) != 0)
      strncpyzt (name, hp->h_name, len);
    else
      strncpyzt (name, tmp, len);
    memcpy ((char *) &mysk.SIN_ADDR, hp->h_addr, sizeof (struct IN_ADDR));
    Debug ((DEBUG_DEBUG, "local name is %s", get_client_name (&me, TRUE)));
  }
  return;
}

#ifdef USE_ADNS
/* 
 * got_client_dns
 * 
 * Called when ADNS has got a answer for a DNS Query for a aClient call
 *
 */
static void
got_client_dns (void *vptr, adns_answer * reply)
{
  struct Client *client_p = (struct Client *) vptr;
  ClearDNS (client_p);
  if (!reply || reply->status != adns_s_ok)
  {
#ifdef SHOW_HEADERS
#ifdef USE_SSL
    if (!IsSSL (client_p))
    {
#endif
      sendto_one (client_p, REPORT_FAIL_DNS);
#ifdef USE_SSL
    }
#endif
#endif

  }
  else
  {
#ifdef SHOW_HEADERS
#ifdef USE_SSL
    if (!IsSSL (client_p))
    {
#endif
      sendto_one (client_p, REPORT_FIN_DNS);
#ifdef USE_SSL
    }
#endif
#endif
    /* the actual hostname is attached in attach_iline! */

    Debug ((DEBUG_DEBUG, "Got Reply: %s", *reply->rrs.str));
  }
  check_client_fd (client_p);
  Debug ((DEBUG_DEBUG, "Got Callback for %s", client_p->name));
}

static void
got_async_connect_dns (void *ptr, adns_answer * reply)
{
  aConfItem *aconf = (aConfItem *) ptr;
  Debug ((DEBUG_DEBUG, "Got async_connect Callback for %s", aconf->host));
  if (!reply || reply->status != adns_s_ok)
  {
    sendto_realops ("Connect to %s failed: host lookup",
                    (aconf) ? aconf->host : "unknown");

    return;
  }
  else
  {
    memcpy ((char *) &aconf->ipnum,
            &reply->rrs.addr->addr.inet.sin_addr.s_addr,
            sizeof (struct IN_ADDR));
    (void) connect_server (aconf, NULL);
  }
}


void
got_async_conf_dns (void *ptr, adns_answer * reply)
{
  aConfItem *aconf = (aConfItem *) ptr;
  Debug ((DEBUG_DEBUG, "Got async_conf Callback for %s", aconf->host));
  if (!reply || reply->status != adns_s_ok)
  {
    sendto_realops ("Conf for %s failed: host lookup",
                    (aconf) ? aconf->host : "unknown");

    return;
  }
  else
  {
    memcpy ((char *) &aconf->ipnum,
            &reply->rrs.addr->addr.inet.sin_addr.s_addr,
            sizeof (struct IN_ADDR));
  }
}

#else
/*
 * do_dns_async
 *
 * Called when the fd returned from init_resolver() has been selected for
 * reading.
 */
void
#ifndef _WIN32
do_dns_async ()
#else
do_dns_async (int id)
#endif
{
  static Link ln;
  aClient *client_p;
  aConfItem *aconf;
  struct hostent *hp;
  int bytes, packets = 0;
  /*
   * if (ioctl(resfd, FIONREAD, &bytes) == -1) bytes = 1;
   */
  do
  {
    ln.flags = -1;
#ifndef _WIN32
    hp = get_res ((char *) &ln);
#else
    hp = get_res ((char *) &ln, id);
#endif
    Debug ((DEBUG_DNS,
            "%#x = get_res(%d,%#x)", hp, ln.flags, ln.value.client_p));
    switch (ln.flags)
    {
       case ASYNC_NONE:
         /*
          * no reply was processed that was outstanding or had
          * a client still waiting.
          */
         break;
       case ASYNC_CLIENT:
         if ((client_p = ln.value.client_p))
         {
           del_queries ((char *) client_p);
           Debug ((DEBUG_DNS, "Hrm, lets see"));
#ifdef SHOW_HEADERS
#ifdef USE_SSL
           if (!IsSSL (client_p))
           {
#endif
             Debug ((DEBUG_DNS, "Sending Header"));
             sendto_one (client_p, REPORT_FIN_DNS);
#ifdef USE_SSL
           }
#endif
#endif
           ClearDNS (client_p);
           client_p->hostp = hp;
           check_client_fd (client_p);
         }
         break;
       case ASYNC_CONNECT:
         aconf = ln.value.aconf;
         if (hp && aconf)
         {
           memcpy ((char *) &aconf->ipnum, hp->h_addr,
                   sizeof (struct IN_ADDR));
           (void) connect_server (aconf, NULL, hp);
         }
         else
           sendto_realops ("Connect to %s failed: host lookup",
                           (aconf) ? aconf->host : "unknown");
         break;
       case ASYNC_CONF:
         aconf = ln.value.aconf;
         if (hp && aconf)
           memcpy ((char *) &aconf->ipnum, hp->h_addr,
                   sizeof (struct IN_ADDR));
         break;
       default:
         break;
    }
#ifndef _WIN32
    if (ioctl (resfd, FIONREAD, &bytes) == -1)
#endif
      bytes = 0;
    packets++;
  }
  while ((bytes > 0) && (packets < 512));
}

#endif
