/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/ircsprintf.c
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
 *  $Id: ircsprintf.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "ircsprintf.h"
#include "va_copy.h"

char num[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char itoa_tab[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
char xtoa_tab[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'a', 'b', 'c', 'd', 'e', 'f'
};
char nullstring[] = "(null)";

#ifdef WANT_SNPRINTF
inline int irc_printf (char *, size_t, const char *, va_list);
#else
inline int irc_printf (char *, const char *, va_list);
#endif



#ifdef WANT_SNPRINTF
inline int
irc_printf (char *str, size_t size, const char *pattern, va_list vl)
#else
inline int
irc_printf (char *str, const char *pattern, va_list vl)
#endif
{
  char *s;
  char *buf = str;
  const char *format = pattern;
  unsigned long i, u;
  int len = 0, is_long = 0;
  va_list ap;
  VA_COPY (ap, vl);


#ifdef WANT_SNPRINTF
  if (!size)
  {
#endif
    while (*format)
    {
      u = 0;
      switch (*format)
      {
         case '%':
           format++;
           is_long = 0;
           switch (*format)
           {
              case 's':        /* most popular ;) */
                s = va_arg (ap, char *);
                if (s == NULL)
                  s = nullstring;
                while (*s)
                  buf[len++] = *s++;
                format++;
                break;
              case 'u':
                format--;       /* falls through and is caught below */
              case 'l':
                is_long = 1;
                if (*(format + 1) == 'u')
                {
                  u = 1;
                  format++;
                }
                else if (*(format + 1) == 'd')
                {
                  u = 0;
                  format++;
                }
                else
                  u = 0;
                /* fallthrough */
              case 'd':
              case 'i':
                if (is_long == 1 || *format == 'd')
                {
                  i = va_arg (ap, unsigned long);
                }
                else
                {
                  i = va_arg (ap, unsigned int);
                }

                if (!u)
                  if (i & 0x80000000)
                  {
                    buf[len++] = '-';   /* it's negative.. */
                    i = 0x80000000 - (i & ~0x80000000);
                  }
                s = &num[11];
                do
                {
                  *--s = itoa_tab[i % 10];
                  i /= 10;
                }
                while (i != 0);
                while (*s)
                  buf[len++] = *s++;
                format++;
                break;
              case 'n':
                /* oo, sneaky...it really is just a long, though! */
                /*case 'x':
                   case 'X': */
                i = va_arg (ap, long);
                buf[len++] = '0';
                buf[len++] = 'x';
                s = &num[11];
                do
                {
                  *--s = xtoa_tab[i % 16];
                  i /= 16;
                }
                while (i != 0);
                while (*s)
                {
                  buf[len++] = *s++;
                }
                format++;
                break;
              case 'c':
                buf[len++] = (char) va_arg (ap, int);
                format++;
                break;
              default:
                /* yick, unknown type...default to returning what our
                   s[n]printf friend would */
                return vsprintf (str, pattern, vl);
                break;
           }
           break;
         default:
           buf[len++] = *format++;
           break;
      }
    }
    buf[len] = 0;
    return len;
#ifdef WANT_SNPRINTF
  }
  else
  {
    while (*format && len < size)
    {
      u = 0;
      switch (*format)
      {
         case '%':
           format++;
           is_long = 0;
           switch (*format)
           {
              case 's':        /* most popular ;) */
                s = va_arg (ap, char *);
                if (s == NULL)
                  s = nullstring;
                while (*s && len < size)
                  buf[len++] = *s++;
                format++;
                break;
              case 'u':
                format--;       /* now fall through and it's caught, cool */
              case 'l':
                is_long = 1;
                if (*(format + 1) == 'u')
                {
                  u = 1;
                  format++;
                }
                else if (*(format + 1) == 'd')
                {
                  u = 0;
                  format++;
                }
                else
                  u = 0;
                /* fallthrough */
              case 'd':
              case 'i':
                if (is_long == 1 || *format == 'd')
                {
                  i = va_arg (ap, unsigned long);
                }
                else
                {
                  i = va_arg (ap, unsigned int);
                }

                if (!u)
                  if (i & 0x80000000)
                  {
                    buf[len++] = '-';   /* it's negative.. */
                    i = 0x80000000 - (i & ~0x80000000);
                  }
                s = &num[11];
                do
                {
                  *--s = itoa_tab[i % 10];
                  i /= 10;
                }
                while (i != 0);
                while (*s && len < size)
                  buf[len++] = *s++;
                format++;
                break;
              case 'n':
                /* oo, sneaky...it really is just a long, though! */
                /*case 'x':
                   case 'X': */
                i = va_arg (ap, long);
                buf[len++] = '0';
                if (len < size)
                  buf[len++] = 'x';
                else
                  break;
                s = &num[11];
                do
                {
                  *--s = xtoa_tab[i % 16];
                  i /= 16;
                }
                while (i != 0);
                while (*s && len < size)
                  buf[len++] = *s++;
                format++;
                break;
              case 'c':
                buf[len++] = (char) va_arg (ap, int);
                format++;
                break;
              default:
                /* yick, unknown type...default to returning what our
                   s[n]printf friend would */
                return vsnprintf (str, size, pattern, vl);
                break;
           }
           break;
         default:
           buf[len++] = *format++;
           break;
      }
    }
    buf[len] = 0;
    return len;
  }
#endif /* WANT_SNPRINTF */
}

int
ircsprintf (char *str, const char *format, ...)
{
  int ret;
  va_list vl;
  va_start (vl, format);
#ifdef WANT_SNPRINTF
  ret = irc_printf (str, 0, format, vl);
#else
  ret = irc_printf (str, format, vl);
#endif
  va_end (vl);
  return ret;
}

#ifdef WANT_SNPRINTF
int
ircsnprintf (char *str, size_t size, const char *format, ...)
{
  int ret;
  va_list vl;
  va_start (vl, format);
  ret = irc_printf (str, size, format, vl);
  va_end (vl);
  return ret;
}
#endif

int
ircvsprintf (char *str, const char *format, va_list ap)
{
  int ret;
#ifdef WANT_SNPRINTF
  ret = irc_printf (str, 0, format, ap);
#else
  ret = irc_printf (str, format, ap);
#endif
  return ret;
}

#ifdef WANT_SNPRINTF
int
ircvsnprintf (char *str, size_t size, const char *format, va_list ap)
{
  int ret;
  ret = irc_printf (str, size, format, ap);
  return ret;
}
#endif
