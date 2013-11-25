/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/inet_pton.c
 *
 *  Copyright (C) 1996 by Internet Software Consortium.
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
 *  $Id: inet_pton.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "config.h"
#include "struct.h"
#include "common.h"
#include "sys.h"
#ifndef _WIN32
# include <netinet/in.h>
#else
# include <winsock2.h>
#endif

#ifndef HAVE_INET_PTON

#include <sys/types.h>
#ifndef _WIN32
# include <sys/param.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <arpa/nameser.h>
#else
# include <winsock2.h>
# define __P(x) x
#endif
#include <string.h>
#include <errno.h>

#define NS_INADDRSZ sizeof(struct in_addr)
#define NS_IN6ADDRSZ sizeof(struct in6_addr)
#define NS_INT16SZ sizeof(int16_t)

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static int inet_pton4 __P ((const char *src, u_char * dst));
static int inet_pton6 __P ((const char *src, u_char * dst));

/* int
 * inet_pton(af, src, dst)
 *	convert from presentation format (which usually means ASCII printable)
 *	to network format (which is usually some kind of binary format).
 * return:
 *	1 if the address was valid for the specified address family
 *	0 if the address wasn't valid (`dst' is untouched in this case)
 *	-1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *	Paul Vixie, 1996.
 */
int
inet_pton (af, src, dst)
     int af;
     const char *src;
     void *dst;
{
  switch (af)
  {
     case AF_INET:
       return (inet_pton4 (src, dst));
#ifdef INET6
     case AF_INET6:
       return (inet_pton6 (src, dst));
#endif
     default:
#ifndef _WIN32
       errno = EAFNOSUPPORT;
#else
       WSASetLastError (WSAEAFNOSUPPORT);
#endif
       return (-1);
  }
  /* NOTREACHED */
}

/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton4 (src, dst)
     const char *src;
     u_char *dst;
{
  static const char digits[] = "0123456789";
  int saw_digit, octets, ch;
  u_char tmp[NS_INADDRSZ], *tp;

  saw_digit = 0;
  octets = 0;
  *(tp = tmp) = 0;
  while ((ch = *src++) != '\0')
  {
    const char *pch;

    if ((pch = strchr (digits, ch)) != NULL)
    {
      u_int new = *tp * 10 + (pch - digits);

      if (new > 255)
        return (0);
      *tp = new;
      if (!saw_digit)
      {
        if (++octets > 4)
          return (0);
        saw_digit = 1;
      }
    }
    else if (ch == '.' && saw_digit)
    {
      if (octets == 4)
        return (0);
      *++tp = 0;
      saw_digit = 0;
    }
    else
      return (0);
  }
  if (octets < 4)
    return (0);

  memcpy (dst, tmp, NS_INADDRSZ);
  return (1);
}

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
#ifdef INET6
static int
inet_pton6 (src, dst)
     const char *src;
     u_char *dst;
{
  static const char xdigits_l[] = "0123456789abcdef",
    xdigits_u[] = "0123456789ABCDEF";
  u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
  const char *xdigits, *curtok;
  int ch, saw_xdigit;
  u_int val;

  memset ((tp = tmp), '\0', NS_IN6ADDRSZ);
  endp = tp + NS_IN6ADDRSZ;
  colonp = NULL;
  /* Leading :: requires some special handling. */
  if (*src == ':')
    if (*++src != ':')
      return (0);
  curtok = src;
  saw_xdigit = 0;
  val = 0;
  while ((ch = *src++) != '\0')
  {
    const char *pch;

    if ((pch = strchr ((xdigits = xdigits_l), ch)) == NULL)
      pch = strchr ((xdigits = xdigits_u), ch);
    if (pch != NULL)
    {
      val <<= 4;
      val |= (pch - xdigits);
      if (val > 0xffff)
        return (0);
      saw_xdigit = 1;
      continue;
    }
    if (ch == ':')
    {
      curtok = src;
      if (!saw_xdigit)
      {
        if (colonp)
          return (0);
        colonp = tp;
        continue;
      }
      if (tp + NS_INT16SZ > endp)
        return (0);
      *tp++ = (u_char) (val >> 8) & 0xff;
      *tp++ = (u_char) val & 0xff;
      saw_xdigit = 0;
      val = 0;
      continue;
    }
    if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
        inet_pton4 (curtok, tp) > 0)
    {
      tp += NS_INADDRSZ;
      saw_xdigit = 0;
      break;                    /* '\0' was seen by inet_pton4(). */
    }
    return (0);
  }
  if (saw_xdigit)
  {
    if (tp + NS_INT16SZ > endp)
      return (0);
    *tp++ = (u_char) (val >> 8) & 0xff;
    *tp++ = (u_char) val & 0xff;
  }
  if (colonp != NULL)
  {
    /*
     * Since some memmove()'s erroneously fail to handle
     * overlapping regions, we'll do the shift by hand.
     */
    const int n = tp - colonp;
    int i;

    for (i = 1; i <= n; i++)
    {
      endp[-i] = colonp[n - i];
      colonp[n - i] = 0;
    }
    tp = endp;
  }
  if (tp != endp)
    return (0);
  memcpy (dst, tmp, NS_IN6ADDRSZ);
  return (1);
}
#endif
#endif /* OS_DARWIN */
