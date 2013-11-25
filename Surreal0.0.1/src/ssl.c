/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/ssl.c
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
 *  $Id: ssl.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include <sys/types.h>
#include "h.h"

#ifdef USE_SSL

#define SAFE_SSL_READ	1
#define SAFE_SSL_WRITE	2
#define SAFE_SSL_ACCEPT	3

#ifdef _WIN32
# pragma comment(lib, "C:\\OpenSSL\\lib\\VC\\libeay32.lib")
# pragma comment(lib, "C:\\OpenSSL\\lib\\VC\\ssleay32.lib")
#endif

extern int errno;

SSL_CTX *ircdssl_ctx;
int ssl_capable = 0;

int
initssl (void)
{
  SSL_load_error_strings ();
  SSLeay_add_ssl_algorithms ();
  ircdssl_ctx = SSL_CTX_new (SSLv23_server_method ());
  if (!ircdssl_ctx)
  {
    ERR_print_errors_fp (stderr);
    return 0;
  }
  if (SSL_CTX_use_certificate_file (ircdssl_ctx,
                                    IRCDSSL_CPATH, SSL_FILETYPE_PEM) <= 0)
  {
    ERR_print_errors_fp (stderr);
    SSL_CTX_free (ircdssl_ctx);
    return 0;
  }
  if (SSL_CTX_use_PrivateKey_file (ircdssl_ctx,
                                   IRCDSSL_KPATH, SSL_FILETYPE_PEM) <= 0)
  {
    ERR_print_errors_fp (stderr);
    SSL_CTX_free (ircdssl_ctx);
    return 0;
  }
  if (!SSL_CTX_check_private_key (ircdssl_ctx))
  {
    fprintf (stderr, "Server certificate does not match Server key");
    SSL_CTX_free (ircdssl_ctx);
    return 0;
  }
  return 1;
}

static int fatal_ssl_error (int, int, aClient *);

int
safe_SSL_read (aClient * target_p, void *buf, int sz)
{
  int len, ssl_err;

  len = SSL_read (target_p->ssl, buf, sz);
  if (len <= 0)
  {
    switch (ssl_err = SSL_get_error (target_p->ssl, len))
    {
       case SSL_ERROR_SYSCALL:
#ifdef _WIN32
         if (WSAGetLastError () == WSAEWOULDBLOCK
             || WSAGetLastError () == WSAEINTR)
#else
         if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
#endif
         {
       case SSL_ERROR_WANT_READ:
#ifdef _WIN32
           WSASetLastError (WSAEWOULDBLOCK);
#else
           errno = EWOULDBLOCK;
#endif
           return 0;
         }
       case SSL_ERROR_SSL:
#ifndef _WIN32
         if (errno == EAGAIN)
           return 0;
#endif
       default:
         return fatal_ssl_error (ssl_err, SAFE_SSL_READ, target_p);
    }
  }
  return len;
}

int
safe_SSL_write (aClient * target_p, const void *buf, int sz)
{
  int len, ssl_err;

  len = SSL_write (target_p->ssl, buf, sz);
  if (len <= 0)
  {
    switch (ssl_err = SSL_get_error (target_p->ssl, len))
    {
       case SSL_ERROR_SYSCALL:
#ifdef _WIN32
         if (WSAGetLastError () == WSAEWOULDBLOCK
             || WSAGetLastError () == WSAEINTR)
#else
         if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
#endif
         {
       case SSL_ERROR_WANT_WRITE:
#ifdef _WIN32
           WSASetLastError (WSAEWOULDBLOCK);
#else
           errno = EWOULDBLOCK;
#endif
           return 0;
         }
       case SSL_ERROR_SSL:
#ifndef _WIN32
         if (errno == EAGAIN)
           return 0;
#endif
       default:
         return fatal_ssl_error (ssl_err, SAFE_SSL_WRITE, target_p);
    }
  }
  return len;
}

int
safe_SSL_accept (aClient * target_p, int fd)
{

  int ssl_err;

  if ((ssl_err = SSL_accept (target_p->ssl)) <= 0)
  {
    switch (ssl_err = SSL_get_error (target_p->ssl, ssl_err))
    {
       case SSL_ERROR_SYSCALL:
#ifdef _WIN32
         if (WSAGetLastError () == WSAEINTR
             || WSAGetLastError () == WSAEWOULDBLOCK)
#else
         if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
#endif
       case SSL_ERROR_WANT_READ:
       case SSL_ERROR_WANT_WRITE:
           /* handshake will be completed later . . */
           return 1;
       default:
         return fatal_ssl_error (ssl_err, SAFE_SSL_ACCEPT, target_p);

    }
    /* NOTREACHED */
    return -1;
  }
  return 1;
}

int
SSL_smart_shutdown (SSL * ssl)
{
  char i;
  int rc;

  rc = 0;
  for (i = 0; i < 4; i++)
  {
    if ((rc = SSL_shutdown (ssl)))
      break;
  }

  return rc;
}

static int
fatal_ssl_error (int ssl_error, int where, aClient * source_p)
{
  /* don`t alter errno */
  int errtmp = errno;
  char *errstr = strerror (errtmp);
  char *ssl_errstr, *ssl_func;

  switch (where)
  {
     case SAFE_SSL_READ:
       ssl_func = "SSL_read()";
       break;
     case SAFE_SSL_WRITE:
       ssl_func = "SSL_write()";
       break;
     case SAFE_SSL_ACCEPT:
       ssl_func = "SSL_accept()";
       break;
     default:
       ssl_func =
         "undefined SSL func [this is a bug] repor to to Ultimate-Devel@Shadow-Realm.org";
  }

  switch (ssl_error)
  {
     case SSL_ERROR_NONE:
       ssl_errstr = "No error";
       break;
     case SSL_ERROR_SSL:
       ssl_errstr = "Internal OpenSSL error or protocol error";
       break;
     case SSL_ERROR_WANT_READ:
       ssl_errstr = "OpenSSL functions requested a read()";
       break;
     case SSL_ERROR_WANT_WRITE:
       ssl_errstr = "OpenSSL functions requested a write()";
       break;
     case SSL_ERROR_WANT_X509_LOOKUP:
       ssl_errstr = "OpenSSL requested a X509 lookup which didn`t arrive";
       break;
     case SSL_ERROR_SYSCALL:
       ssl_errstr = "Underlying syscall error";
       break;
     case SSL_ERROR_ZERO_RETURN:
       ssl_errstr = "Underlying socket operation returned zero";
       break;
     case SSL_ERROR_WANT_CONNECT:
       ssl_errstr = "OpenSSL functions wanted a connect()";
       break;
     default:
       ssl_errstr = "Unknown OpenSSL error (huh?)";
  }

  sendto_realops_lev (DEBUG_LEV, "%s to "
                      "%s!%s@%s aborted with%serror (%s). [%s]",
                      ssl_func,
                      *source_p->name ? source_p->name : "<unknown>",
                      (source_p->user
                       && source_p->user->username) ? source_p->user->
                      username : "<unregistered>", source_p->sockhost,
                      (errno > 0) ? " " : " no ", errstr, ssl_errstr);
#ifdef USE_SYSLOG
  syslog (LOG_ERR, "SSL error in %s: %s [%s]", ssl_func, errstr, ssl_errstr);
#endif

  /* if we reply() something here, we might just trigger another
   * fatal_ssl_error() call and loop until a stack overflow...
   * the client won`t get the ERROR : ... string, but this is
   * the only way to do it.
   * IRC protocol wasn`t SSL enabled .. --vejeta
   */

  errno = errtmp ? errtmp : EIO;        /* Stick a generic I/O error */
  source_p->sockerr = IRCERR_SSL;
  source_p->flags |= FLAGS_DEADSOCKET;
  return -1;
}
#endif
