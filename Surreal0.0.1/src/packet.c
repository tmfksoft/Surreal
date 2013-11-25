/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/packet.c
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
 *  $Id: packet.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "msg.h"
#include "h.h"
#include "dh.h"
#include "zlink.h"

/*
 * * dopacket *       client_p - pointer to client structure for which the buffer
 * data *              applies. *       buffer - pointr to the buffer
 * containing the newly read data *     length - number of valid bytes
 * of data in the buffer *
 * 
 * Note: *      It is implicitly assumed that dopacket is called only *
 * with client_p of "local" variation, which contains all the *
 * necessary fields (buffer etc..)
 */
int
dopacket (aClient * client_p, char *buffer, int length)
{
  char *ch1;
  char *ch2;
  char *client_pbuf = client_p->buffer;
  aClient *acpt = client_p->acpt;
  char *nbuf = NULL;
  int nlen;

#ifdef HAVE_ENCRYPTION_ON
  if (IsRC4IN (client_p))
    rc4_process_stream (client_p->serv->rc4_in, buffer, length);
#endif

  me.receiveB += length;        /*
                                 * Update bytes received 
                                 */
  client_p->receiveB += length;

  if (client_p->receiveB & 0x0400)
  {
    client_p->receiveK += (client_p->receiveB >> 10);
    client_p->receiveB &= 0x03ff;       /*
                                         * 2^10 = 1024, 3ff = 1023 
                                         */
  }

  if (acpt != &me)
  {
    acpt->receiveB += length;
    if (acpt->receiveB & 0x0400)
    {
      acpt->receiveK += (acpt->receiveB >> 10);
      acpt->receiveB &= 0x03ff;
    }
  }
  else if (me.receiveB & 0x0400)
  {
    me.receiveK += (me.receiveB >> 10);
    me.receiveB &= 0x03ff;
  }

zcontinue:
  ch1 = client_pbuf + client_p->count;
  ch2 = buffer;

  if (ZipIn (client_p))
  {
    int err;
    ch2 =
      zip_input (client_p->serv->zip_in, ch2, &length, &err, &nbuf, &nlen);

    if (length == -1)
    {
      sendto_realops ("Zipin error for %s: (%d) %s\n", client_p->name,
                      err, ch2);
      return exit_client (client_p, client_p, &me,
                          "fatal error in zip_input!");
    }
  }

  while (--length >= 0)
  {
    char g;

    g = (*ch1 = *ch2++);
    /*
     * Yuck.  Stuck.  To make sure we stay backward compatible, we
     * must assume that either CR or LF terminates the message and
     * not CR-LF.  By allowing CR or LF (alone) into the body of
     * messages, backward compatibility is lost and major problems
     * will arise. - Avalon
     */
    if (g < '\16' && (g == '\n' || g == '\r'))
    {
      if (ch1 == client_pbuf)
        continue;               /*
                                 * Skip extra LF/CR's 
                                 */
      *ch1 = '\0';
      me.receiveM += 1;         /*
                                 * Update messages received 
                                 */
      client_p->receiveM += 1;
      if (client_p->acpt != &me)
        client_p->acpt->receiveM += 1;
      client_p->count = 0;      /*
                                 * ...just in case parse returns with *
                                 * * FLUSH_BUFFER without removing the *
                                 * * structure pointed by client_p... --msa 
                                 */

      switch (parse (client_p, client_p->buffer, ch1))
      {
         case FLUSH_BUFFER:
           return FLUSH_BUFFER;

         case ZIP_NEXT_BUFFER:
           if (length)
           {
             int err;
             ch2 =
               zip_input (client_p->serv->zip_in, ch2, &length, &err,
                          &nbuf, &nlen);

             if (length == -1)
             {
               sendto_realops ("Zipin error for %s: (%d) %s\n",
                               client_p->name, err, ch2);
               return exit_client (client_p, client_p, &me,
                                   "fatal error in zip_input!");
             }
           }
           break;

#ifdef HAVE_ENCRYPTION_ON
         case RC4_NEXT_BUFFER:
           if (length)
             rc4_process_stream (client_p->serv->rc4_in, ch2, length);
           break;
#endif

         default:
           break;
      }



      /*
       * * Socket is dead so exit (which always returns with *
       * FLUSH_BUFFER here).  - avalon
       */
      if (client_p->flags & FLAGS_DEADSOCKET)
        return exit_client (client_p, client_p, &me,
                            (client_p->
                             flags & FLAGS_SENDQEX) ? "SendQ exceeded" :
                            "Dead socket");
      ch1 = client_pbuf;
    }
    else if (ch1 < client_pbuf + (sizeof (client_p->buffer) - 1))
      ch1++;                    /*
                                 * There is always room for the null 
                                 */
  }
  client_p->count = ch1 - client_pbuf;

  if (nbuf)
  {
#if 0                           /* this message is annoying and not quite that useful */
    static time_t last_complain = 0;
    static int numrepeat = 0;

    numrepeat++;

    if (NOW > (last_complain + 300))    /* if more than 5 minutes have elapsed.. */
    {
      if (last_complain == 0)
      {
        sendto_realops ("Overflowed zipInBuf! "
                        "If you see this a lot, you should increase zipInBufSize in src/zlink.c.");
      }
      else
      {
        sendto_realops
          ("Overflowed zipInBuf %d time%s in the last %d minutes. "
           "If you see this a lot, you should increase zipInBufSize in src/zlink.c.",
           numrepeat, numrepeat == 1 ? "" : "s", (NOW - last_complain) / 60);
      }
      last_complain = NOW;
      numrepeat = 0;
    }
#endif

    buffer = nbuf;
    length = nlen;
    nbuf = NULL;
    goto zcontinue;             /* gross, but it should work.. */
  }
  return 0;
}

int
client_dopacket (aClient * client_p, char *buffer, int length)
{

  strncpy (client_p->buffer, buffer, BUFSIZE);
  length = strlen (client_p->buffer);

  /*
   * Update messages received
   */
  ++me.receiveM;
  ++client_p->receiveM;

  /*
   * Update bytes received
   */
  client_p->receiveB += length;

  if (client_p->receiveB > 1023)
  {
    client_p->receiveK += (client_p->receiveB >> 10);
    client_p->receiveB &= 0x03ff;       /* 2^10 = 1024, 3ff = 1023 */
  }
  me.receiveB += length;

  if (me.receiveB > 1023)
  {
    me.receiveK += (me.receiveB >> 10);
    me.receiveB &= 0x03ff;
  }

  client_p->count = 0;          /* ...just in case parse returns with */
  if (FLUSH_BUFFER ==
      parse (client_p, client_p->buffer, client_p->buffer + length))
  {
    /*
     * CLIENT_EXITED means actually that client_p
     * structure *does* not exist anymore!!! --msa
     */
    return FLUSH_BUFFER;
  }
  else if (client_p->flags & FLAGS_DEADSOCKET)
  {
    /*
     * Socket is dead so exit (which always returns with
     * CLIENT_EXITED here).  - avalon
     */
    return exit_client (client_p, client_p, &me,
                        (client_p->
                         flags & FLAGS_SENDQEX) ? "SendQ exceeded" :
                        "Dead socket");
  }
  return 1;
}
