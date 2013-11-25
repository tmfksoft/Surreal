/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/match.c
 *
 *  Copyright (C) 1990 Jarkko Oikarinen
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
 *  $Id: match.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "h.h"
#include "common.h"
#include "sys.h"
#include "send.h"


/* match()
 *
 *  Compare if a given string (name) matches the given
 *  mask (which can contain wild cards: '*' - match any
 *  number of chars, '?' - match any single character.
 *
 *      return  0, if match
 *              1, if no match
 *
 *  Originally by Douglas A Lewis (dalewis@acsu.buffalo.edu)
 */
#define MATCH_MAX_CALLS 512     /* ACK! This dies when it's less that this
                                   and we have long lines to parse */
int
match (const char *mask, const char *name)
{
  const unsigned char *m = (const unsigned char *) mask;
  const unsigned char *n = (const unsigned char *) name;
  const unsigned char *ma = (const unsigned char *) mask;
  const unsigned char *na = (const unsigned char *) name;
  int wild = 0;
  int calls = 0;

  if (!mask || !name)
  {
    return 1;
  }

  while (calls++ < MATCH_MAX_CALLS)
  {
    if (*m == '*')
    {
      /*
       * XXX - shouldn't need to spin here, the mask should have been
       * collapsed before match is called
       */
      while (*m == '*')
      {
        m++;
      }
      wild = 1;
      ma = m;
      na = n;
    }

    if (!*m)
    {
      if (!*n)
      {
        return 0;
      }
      for (m--; (m > (const unsigned char *) mask) && (*m == '?'); m--)
        ;
      if (*m == '*' && (m > (const unsigned char *) mask))
      {
        return 0;
      }
      if (!wild)
      {
        return 1;
      }
      m = ma;
      n = ++na;
    }
    else if (!*n)
    {
      /*
       * XXX - shouldn't need to spin here, the mask should have been
       * collapsed before match is called
       */
      while (*m == '*')
      {
        m++;
      }
      return (*m != 0);
    }
    if (MyToLower (*m) != MyToLower (*n) && *m != '?')
    {
      if (!wild)
      {
        return 1;
      }
      m = ma;
      n = ++na;
    }
    else
    {
      if (*m)
      {
        m++;
      }
      if (*n)
      {
        n++;
      }
    }
  }
  return 1;
}


/* collapse()
 *
 * collapses a string containing multiple *'s.
 */
char *
collapse (char *pattern)
{
  char *p = pattern, *po = pattern;
  char c;
  int f = 0;

  if (p == NULL)
  {
    return NULL;
  }

  while ((c = *p++))
  {
    if (c == '*')
    {
      if (!(f & 1))
        *po++ = '*';
      f |= 1;
    }
    else
    {
      *po++ = c;
      f &= ~1;
    }
  }
  *po++ = 0;

  return pattern;
}


/*
 * irccmp - case insensitive comparison of two 0 terminated strings.
 *
 *      returns  0, if s1 equal to s2
 *	returns  1, if s1 is not equal to s2
 */
int
irccmp (const char *s1, const char *s2)
{
  const unsigned char *str1 = (const unsigned char *) s1;
  const unsigned char *str2 = (const unsigned char *) s2;


  if (!s1 || !s2)
  {
    sendto_realops ("irccmp called with s1=%s s2=%s", s1 ? s1 : "(NULL)",
                    s2 ? s2 : "NULL");
    sendto_realops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
    return 1;
  }

  /*
   * More often than not we wont have to bother about case
   * so lets not waste cycles on touppertab and looping
   */
  if (s1 == s2)
  {
    return 0;
  }

  while (touppertab[*str1] == touppertab[*str2])
  {
    if (*str1 == '\0')
    {
      return 0;
    }
    str1++;
    str2++;
  }
  return 1;
}


/*
 * ircncmp - case insensitive comparison of the first n characters
 * of two 0 terminated strings.
 *
 * returns 0, if the first n chars of s1 is equal to the first n chars of s2
 * returns 1, if the first n chars of s1 is not equal to the first n chars of s2
 */


int
ircncmp (const char *s1, const char *s2, int n)
{
  const unsigned char *str1 = (const unsigned char *) s1;
  const unsigned char *str2 = (const unsigned char *) s2;

  if (!s1 || !s2)
  {
    sendto_realops ("ircncmp called with s1=%s s2=%s", s1 ? s1 : "(NULL)",
                    s2 ? s2 : "NULL");
    sendto_realops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
    return 1;
  }

  while (touppertab[*str1] == touppertab[*str2])
  {
    str1++;
    str2++;
    n--;
    if (n == 0 || (*str1 == '\0' && *str2 == '\0'))
      return 0;
  }
  return 1;
}

/*
 * irccmp_lex - case insensitive comparison of two 0 terminated strings.
 *
 *      returns  0, if s1 equal to s2
 *              <0, if s1 lexicographically less than s2
 *              >0, if s1 lexicographically greater than s2
 */
int
irccmp_lex (const char *s1, const char *s2)
{
  const unsigned char *str1 = (const unsigned char *) s1;
  const unsigned char *str2 = (const unsigned char *) s2;
  int res;

  if (!s1 || !s2)
  {
    sendto_realops ("irccmp_lex called with s1=%s s2=%s",
                    s1 ? s1 : "(NULL)", s2 ? s2 : "NULL");
    sendto_realops
      ("Please report to the development team! Ultimate-Devel@Shadow-Realm.org");
    return 1;
  }

  /*
   * More often than not we wont have to bother about case
   * so lets not waste cycles on touppertab and looping
   */
  if (s1 == s2)
  {
    return 0;
  }

  while ((res = touppertab[*str1] - touppertab[*str2]) == 0)
  {
    if (*str1 == '\0')
      return 0;
    str1++;
    str2++;
  }
  return (res);
}


unsigned char tolowertab[] = {
  0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
  0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
  0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1f,
  ' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
  '*', '+', ',', '-', '.', '/',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  ':', ';', '<', '=', '>', '?',
  '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
  't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '~',
  '_',
  '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
  't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
  0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
  0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
  0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
  0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
  0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
  0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
  0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
  0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
  0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
unsigned char touppertab[] = {
  0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
  0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
  0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1f,
  ' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
  '*', '+', ',', '-', '.', '/',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  ':', ';', '<', '=', '>', '?',
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
  'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
  'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
  0x5f,
  '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
  'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
  'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '^',
  0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
  0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
  0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
  0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
  0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
  0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
  0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
  0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
  0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
unsigned char char_atribs[] = {
  /*
   * 0-7
   */ CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL,
  /*
   * 8-12
   */ CNTRL, CNTRL | SPACE, CNTRL | SPACE, CNTRL | SPACE, CNTRL | SPACE,
  /*
   * 13-15
   */ CNTRL | SPACE, CNTRL, CNTRL,
  /*
   * 16-23
   */ CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL,
  /*
   * 24-31 
   */ CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL, CNTRL,
  /*
   * space 
   */ PRINT | SPACE,
  /*
   * !"#$%&'( 
   */ PRINT, PRINT, PRINT, PRINT, PRINT, PRINT, PRINT, PRINT,
  /*
   * )*+,-./ 
   */ PRINT, PRINT, PRINT, PRINT, PRINT, PRINT, PRINT,
  /*
   * 0123 
   */ PRINT | DIGIT, PRINT | DIGIT, PRINT | DIGIT, PRINT | DIGIT,
  /*
   * 4567
   */ PRINT | DIGIT, PRINT | DIGIT, PRINT | DIGIT, PRINT | DIGIT,
  /*
   * 89:; 
   */ PRINT | DIGIT, PRINT | DIGIT, PRINT, PRINT,
  /*
   * <=>? 
   */ PRINT, PRINT, PRINT, PRINT,
  /*
   * @ 
   */ PRINT,
  /*
   * ABC 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * DEF 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * GHI
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * JKL
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * MNO
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * PQR
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * STU
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * VWX
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * YZ[
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * \]^
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /* _` */
  PRINT, PRINT,
  /*
   * abc
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * def
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * ghi
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * jkl
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * mno 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * pqr 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * stu 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * vwx 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * yz{ 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * \}~ 
   */ PRINT | ALPHA, PRINT | ALPHA, PRINT | ALPHA,
  /*
   * del 
   */ 0,
  /*
   * 80-8f 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * 90-9f 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * a0-af 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * b0-bf 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * c0-cf 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * d0-df 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * e0-ef 
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*
   * f0-ff
   */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
