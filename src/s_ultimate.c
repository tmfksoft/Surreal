/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_ultimate.c
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
 *  $Id: s_ultimate.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifndef _WIN32
# include <unistd.h>
# include <sys/socket.h>
# include <sys/file.h>
# include <sys/ioctl.h>
# include <sys/un.h>
# include <utmp.h>
#else
# include <winsock2.h>
#endif
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "h.h"


#define AllocCpy(x,y) x = (char *) MyMalloc(strlen(y) + 1); strcpy(x,y)

aCRline *crlines = NULL;

char *cannotjoin_msg = NULL;


/*
**
** m_settings November 18th 1999 - ShadowMaster
**   Will display the dynconf settings for local or remote server.
**   /SETTINGS <parm> <optional servername>
**
*/


extern int report_dynconf (aClient *);
extern int report_network (aClient *);
extern int report_options (aClient *);
extern int report_hostmasks (aClient *);
extern int report_services (aClient *);
static void cr_report (aClient *);

static int cr_add (char *, int);
static aCRline *cr_del (aCRline * fl);
#ifdef UNUSED
static int check_dynconf_access (aClient *, int);
static int check_dynconf_permission (aClient *, int);
#endif

int
m_settings (aClient * client_p, aClient * source_p, int parc, char *parv[])
{

  if (MyClient (source_p) && !IsAnOper (source_p))
  {
    if (parc < 2)
    {

      sendto_one (source_p,
                  ":%s %d %s :\2==================== UltimateIRCd Settings ====================\2",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS,
                  source_p->name);


      sendto_one (source_p, ":%s %d %s :Use \2/SETTINGS (COMMAND)",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS,
                  source_p->name);


      sendto_one (source_p,
                  ":%s %d %s :\2CHANNELS\2         View Allowed Channel Settings",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS,
                  source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2=================================================================\2",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name,
                  source_p->name);


      return 0;
    }
    if (irccmp (parv[1], "channels") == 0)
    {
      cr_report (source_p);
      return 0;
    }

    else
    {
      sendto_one (source_p,
                  ":%s %d %s :Error: %s is not a supported parameter for SETTINGS.",
                  me.name, RPL_SETTINGS, source_p->name, parv[1]);
      sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name,
                  source_p->name);
      return 0;
    }

  }
  if (hunt_server (client_p, source_p, ":%s SETTINGS %s :%s", 2, parc, parv)
      == HUNTED_ISME)
  {
    if (parc < 2)
    {

      sendto_one (source_p,
                  ":%s %d %s :\2==================== UltimateIRCd Settings ====================\2",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS,
                  source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :Use \2/SETTINGS (COMMAND) [Server]",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS,
                  source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2DYNCONF\2          View IRCd Settings",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2NETINFO\2          View Network Info",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2OPTIONS\2          View Network Options",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2SERVICES\2         View Network Services Settings",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2HOSTMASKS\2        View Network Hostmask Settings",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2CHANNELS\2         View Allowed Channel Settings",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS,
                  source_p->name);

      sendto_one (source_p,
                  ":%s %d %s :\2=================================================================\2",
                  me.name, RPL_SETTINGS, source_p->name);

      sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name,
                  source_p->name);


      return 0;
    }

    if (irccmp (parv[1], "dynconf") == 0)
    {
      (void) report_dynconf (source_p);
      return 0;
    }
    else if (irccmp (parv[1], "netinfo") == 0)
    {
      (void) report_network (source_p);
      return 0;
    }
    else if (irccmp (parv[1], "options") == 0)
    {
      (void) report_options (source_p);
      return 0;
    }
    else if (irccmp (parv[1], "hostmasks") == 0)
    {
      (void) report_hostmasks (source_p);
      return 0;
    }

    else if (irccmp (parv[1], "services") == 0)
    {
      (void) report_services (source_p);
      return 0;
    }

    else if (irccmp (parv[1], "channels") == 0)
    {
      cr_report (source_p);
      return 0;
    }

    else
    {
      sendto_one (source_p,
                  ":%s %d %s :Error: %s is not a supported parameter for SETTINGS.",
                  me.name, RPL_SETTINGS, source_p->name, parv[1]);
      sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name,
                  source_p->name);
      return 0;
    }

  }
  return 0;
}

/*
** m_sethost
**
** Based on m_sethost by Stskeeps
*/
int
m_sethost (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *p;
  int bad_dns, dots;

  if (check_registered (source_p))
    return 0;

  /*
   *  Getting rid of the goto's - ShadowMaster
   */
  if (!IsPrivileged (source_p))
  {
    sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
    return 0;
  }


  if (parc < 3 && IsPerson (source_p))
  {
    if (MyConnect (source_p))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [SETHOST] -- Syntax: /SETHOST <nickname> <hostname>",
                  me.name, parv[0]);
    }
    return 0;
  }

  /* Safety net. */
  if (parc < 3)
  {
    return 0;
  }

  /* paranoia */
  if (parv[2] == NULL)
  {
    return 0;
  }

  if (strlen (parv[2]) > HOSTLEN)
  {
    if (MyConnect (source_p))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [SETHOST] -- Error: Hostnames are limited to %d characters.",
                  me.name, source_p->name, HOSTLEN);
    }
    return 0;
  }
  /*
   * We would want this to be at least 4 characters long.  - ShadowMaster
   */
  if (strlen (parv[2]) < 4)
  {
    if (MyConnect (source_p))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [SETHOST] -- Error: Please enter a valid hostname.",
                  me.name, source_p->name);
    }
    return 0;
  }

  dots = 0;
  p = parv[2];
  bad_dns = NO;

#ifdef INET6
  if (*p == ':')                /* We dont accept hostnames with ':' */
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [SETHOST] -- Error: First char in you hostname is ':'",
                me.name, parv[0]);
    return 0;
  }
#endif

  /* Fixes the RFC breakage issue rather nicely... - Quinn 27/08/2004 */
  if (*p == '.')
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [SETHOST] -- Error: First char in you hostname is '.'",
                me.name, parv[0]);
    return 0;
  }


  while (*p)
  {
    if (!MyIsAlnum (*p))
    {
      if ((*p != '-') && (*p != '.') && (*p != ':'))
        bad_dns = YES;
    }

    if ((*p == '.') || (*p == ':'))
      dots++;

    p++;
  }

  if (bad_dns == YES)
  {
    if (MyConnect (source_p))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [SETHOST] -- Error: A hostname may contain a-z, A-Z, 0-9, '-' & '.'",
                  me.name, parv[0]);
    }
    return 0;
  }
  if (!dots)
  {
    if (MyConnect (source_p))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [SETHOST] -- Error: You must have at least one . (dot) in the hostname.",
                  me.name, parv[0]);
    }
    return 0;
  }


  if ((target_p = find_person (parv[1], NULL)))
  {
    if (!IsHidden (target_p))
    {
      SetHidden (target_p);
    }
    /*
     * Now using strncpyzt instead of ircsprintf - ShadowMaster
     */
    strncpyzt (target_p->user->virthost, parv[2], HOSTLEN);

    sendto_serv_butone (client_p, ":%s SETHOST %s %s", me.name,
                        target_p->name, parv[2]);

    if (MyClient (source_p))
    {
      if (source_p == target_p)
      {
        sendto_serv_butone (client_p,
                            ":%s NETINFO : \2[SETHOST]\2 -- %s (%s@%s) set their virtualhost to \2%s\2",
                            me.name, parv[0], source_p->user->username,
                            source_p->user->host, source_p->user->virthost);
        sendto_netinfo
          ("from %s: \2[SETHOST]\2 -- %s (%s@%s) set their virtualhost to \2%s\2",
           me.name, parv[0], source_p->user->username,
           source_p->user->host, source_p->user->virthost);
        sendto_one (source_p,
                    ":%s NOTICE %s :*** [SETHOST] -- Your virtual hostname is now \2%s\2",
                    me.name, parv[0], source_p->user->virthost);

        ilog (LOGF_OPERACT,
              "[SETHOST] %s (%s@%s) set their virtualhost to %s", parv[0],
              source_p->user->username, source_p->user->host,
              source_p->user->virthost);
      }

      else
      {
        sendto_serv_butone (client_p,
                            ":%s NETINFO : \2[SETHOST]\2 -- %s set the virtualhost of %s (%s@%s) to \2%s\2",
                            me.name, parv[0], target_p->name,
                            target_p->user->username,
                            target_p->user->host, target_p->user->virthost);
        sendto_netinfo
          ("from %s: \2[SETHOST]\2 -- %s set the virtualhost of %s (%s@%s) to \2%s\2",
           me.name, parv[0], target_p->name, target_p->user->username,
           target_p->user->host, target_p->user->virthost);
        sendto_one (source_p,
                    ":%s NOTICE %s :*** [SETHOST] -- Set the virtualhost of %s to \2%s\2",
                    me.name, parv[0], target_p->name,
                    target_p->user->virthost);

        ilog (LOGF_OPERACT,
              "[SETHOST] %s set the virtualhost of %s (%s@%s) to %s", parv[0],
              target_p->name, target_p->user->username, target_p->user->host,
              target_p->user->virthost);

      }


    }
  }
  else
  {
    sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, source_p->name,
                parv[1]);
    return 0;
  }
  return 0;
}


/*
** m_vhost
**
** Based on the original m_oper in bahamut.
** Allows regular users to use VHOST's setup for them by
** the server admin. - ShadowMaster
**
** parv[0] = sender prefix
** parv[1] = username
** parv[2] = password
*/
int
m_vhost (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aConfItem *aconf;
  char *name, *password, *encr;

#ifdef HAVE_LIBCRYPT
# define CRYPT_VHOST_PASSWORD
#endif
#ifdef CRYPT_VHOST_PASSWORD
  extern char *crypt ();

#endif

  name = parc > 1 ? parv[1] : (char *) NULL;
  password = parc > 2 ? parv[2] : (char *) NULL;

  if (!IsServer (client_p) && (BadPtr (name) || BadPtr (password)))
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "VHOST");
    return 0;
  }
  if (!(aconf = find_conf_name (name, CONF_VHOST)))
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** [VHOST] -- No V line for that username.",
                me.name, parv[0]);

    sendto_netinfo
      ("from %s: \2[VHOST]\2 -- Failed VHOST attempt by %s (%s@%s)",
       me.name, parv[0], source_p->user->username, source_p->user->host);
    return 0;
  }

#ifdef CRYPT_VHOST_PASSWORD
  /* use first two chars of the password they send in as salt */
  /* passwd may be NULL pointer. Head it off at the pass... */
  if (password && *aconf->passwd)
    encr = crypt (password, aconf->passwd);
  else
    encr = "";
#else
  encr = password;
#endif /* CRYPT_VHOST_PASSWORD */

  if ((aconf->status & CONF_VHOST) &&
      StrEq (encr, aconf->passwd) && !attach_conf (source_p, aconf))
  {
    int old = (source_p->umode & ALL_UMODES);

    /*
     * In theory we can have very long hostnames. But we wanna stay securely on the safe side of
     * things. Prevent admins from goofing up and reject the /VHOST if its too long then warn the user
     * and the opers about it - ShadowMaster
     */

    if (strlen (aconf->host) > HOSTLEN)
    {
      sendto_netinfo
        ("from %s: \2[VHOST]\2 -- %s (%s@%s) attempted to set a vhost with login \2%s\2. The host set in the vhost.conf for this users is longer than %d chars long and was rejected.",
         me.name, parv[0], source_p->user->username, source_p->user->host,
         name, HOSTLEN);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [VHOST] -- The hostname set in the vhost.conf for your login is too long. Please contact the admin to have this corrected.",
                  me.name, parv[0]);
      return 0;
    }

    /* We also want to prevent our first char being '.' as well - Quinn 27/08/2004 */
    if (*aconf->host == '.')
    {
      sendto_netinfo
        ("from %s: \2[VHOST]\2 -- %s (%s@%s) attempted to set a vhost with login \2%s\2. The host set in the vhost.conf for this users breaks the RFC and was rejected.",
         me.name, parv[0], source_p->user->username, source_p->user->host,
         name, HOSTLEN);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** [VHOST] -- The hostname set in the vhost.conf for your login is invalid. Please contact the admin to have this corrected.",
                  me.name, parv[0]);
      return 0;
    }


    if (!IsHidden (source_p))
    {
      SetHidden (source_p);
    }

    strcpy (source_p->user->virthost, aconf->host);
    sendto_serv_butone (client_p, ":%s SETHOST %s %s", me.name, parv[0],
                        source_p->user->virthost);

    sendto_netinfo
      ("from %s: \2[VHOST]\2 -- %s (%s@%s) set their virtualhost to \2%s\2 using userid \2%s\2",
       me.name, parv[0], source_p->user->username, source_p->user->host,
       source_p->user->virthost, name);
    sendto_one (source_p,
                ":%s NOTICE %s :*** [VHOST] -- Your virtual hostname is now \2%s\2",
                me.name, parv[0], source_p->user->virthost);

    send_umode_out (client_p, source_p, old);




  }
  else
  {
    (void) detach_conf (source_p, aconf);
    sendto_one (source_p, err_str (ERR_PASSWDMISMATCH), me.name, parv[0]);
    sendto_netinfo
      ("from %s: \2[VHOST]\2 -- Failed VHOST attempt by %s (%s@%s)",
       me.name, parv[0], source_p->user->username, source_p->sockhost);

  }
  return 0;
}


/*
** m_ircops
**
** Based on m_ircops by ScOrP|On
** Rewritten by ShadowMaster for UltimateIRCd
**
** Lists online IRC Operators showing their oper level,
** home server and away status. - ShadowMaster
**
*/
int
m_ircops (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char buf[BUFSIZE];
  char buf2[BUFSIZE];
  char buf3[BUFSIZE];
  int locals = 0, globals = 0;

  if (!MyClient (source_p))
  {
    return 0;
  }

  sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0],
              "\2=========================\2 Online IRC Operators \2=========================\2");

  sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0], "-");

  sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0],
              "SO = Services Operator SA = Services Administrator SRA = Services Root Administrator");

  sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0], "-");

  for (target_p = client; target_p; target_p = target_p->next)
  {
    if (!IsULine (target_p) && IsAnOper (target_p))
    {
      if (!target_p->user)
      {
        continue;
      }

      buf2[0] = '\0';
      if (IsNetAdmin (target_p))
      {
        strcpy (buf2, "Network Administrator");
      }
      else if (IsNetCoAdmin (target_p))
      {
        strcpy (buf2, "Network Co Administrator");
      }
      else if (IsTechAdmin (target_p))
      {
        strcpy (buf2, "Technical Administrator");
      }
      else if (IsTechCoAdmin (target_p))
      {
        strcpy (buf2, "Technical Co Administrator");
      }
      else if (IsServerAdmin (target_p))
      {
        strcpy (buf2, "Server Administrator");
      }
      else if (IsServerCoAdmin (target_p))
      {
        strcpy (buf2, "Server Co Administrator");
      }
      else if (IsGuestAdmin (target_p))
      {
        strcpy (buf2, "Guest Administrator");
      }
      else if (IsOper (target_p))
      {
        strcpy (buf2, "Global Operator");
      }
      else if (IsLocOp (target_p))
      {
        strcpy (buf2, "Local Operator");
      }

      buf3[0] = '\0';

      if (IsServicesRoot (target_p))
      {
        strcpy (buf3, "(SRA)");
      }
      else if (IsServicesAdmin (target_p))
      {
        strcpy (buf3, "(SA)");
      }
      else if (IsServicesOper (target_p))
      {
        strcpy (buf3, "(SO)");
      }

      ircsprintf (buf, "\2%s\2 -- %s %s (%s)%s",
                  target_p->name, buf2,
                  (IsSkoServicesStaff (target_p) ? buf3 : ""),
                  target_p->user->server,
                  (target_p->user->
                   away ? " -- \2Currently marked away\2"
                   : (IsHelpOp (target_p) ? " -- \2Available for help\2" :
                      "")));

      sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0], buf);
      sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0], "-");
      if (IsOper (target_p))
      {
        globals++;
      }
      else
      {
        locals++;
      }
    }
  }


  ircsprintf (buf,
              "Total: \2%d\2 IRCOp%s online - \2%d\2 Globa%s, \2%d\2 Loca%s",
              globals + locals,
              (globals + locals) > 1 ? "s" : "", globals,
              globals > 1 ? "ls" : "l", locals, locals > 1 ? "ls" : "l");
  sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0], buf);

  sendto_one (source_p, rpl_str (RPL_IRCOPS), me.name, parv[0],
              "\2===========================================================================\2");
  sendto_one (source_p, rpl_str (RPL_ENDOFIRCOPS), me.name, parv[0]);
  return 0;
}

/*
**
** check_clones by Crash
**
**	Slight modifications for UltimateIRCd by ShadowMaster.
**
**	Checks the ENTIRE network for clients with the same host,
**	counts them and returns 0 if the clone count is below MAXCLONES
**	and -1 if the clone count is above MAXCLONES.
**	Ignores ULined clients.
**
*/
int
check_clones (aClient * client_p, aClient * source_p)
{
  aClient *target_p;
  int c = 1;
  for (target_p = client; target_p; target_p = target_p->next)
  {
    /* first, are they a "Real" person? */
    if (!target_p->user)
      continue;
    /* lets let services clone ;-) */
    if (IsULine (target_p))
      continue;
    /* just ignore servers */
    if (IsServer (target_p))
      continue;
    /* dont count the client doing it */
    if (irccmp (source_p->name, target_p->name) == 0)
      continue;
    /* If the network wants its opers to be allowed to clone.... - ShadowMaster */
    if ((OPER_CLONE_LIMIT_BYPASS == 1) && IsAnOper (target_p))
      continue;
    /* ahh! the hosts match!  count it */
    if (irccmp (source_p->user->host, target_p->user->host) == 0)
      c++;
  }

  /* WHAT?! exceding clone limit!? KILL EM?! */

  if ((CLONE_LIMIT - c) < 0)
  {
    return -1;
  }
  return 0;
}


/* Star Channel Restric */


/*
** channel_canjoin from UnrealIRCd
*/
int
channel_canjoin (aClient * source_p, char *name)
{
  aCRline *p;
  if (IsAnOper (source_p))
    return 1;
  if (IsULine (source_p))
    return 1;
  if (!crlines)
    return 1;
  for (p = crlines; p; p = p->next)
  {
    if (!match (p->channel, name))
      return 1;
  }
  return 0;
}

/*
** cr_add from UnrealIRCd
*/
int
cr_add (char *mychannel, int type)
{
  aCRline *fl;
  fl = (aCRline *) MyMalloc (sizeof (aCRline));
  AllocCpy (fl->channel, mychannel);
  fl->type = type;
  fl->next = crlines;
  fl->prev = NULL;
  if (crlines)
    crlines->prev = fl;
  crlines = fl;
  return 0;
}

aCRline *
cr_del (aCRline * fl)
{
  aCRline *p, *q;
  for (p = crlines; p; p = p->next)
  {
    if (p == fl)
    {
      q = p->next;
      MyFree ((char *) p->channel);
      /* chain1 to chain3 */
      if (p->prev)
      {
        p->prev->next = p->next;
      }
      else
      {
        crlines = p->next;
      }
      if (p->next)
      {
        p->next->prev = p->prev;
      }
      MyFree ((aCRline *) p);
      return q;
    }
  }
  return NULL;
}


/*
** cr_loadconf from UnrealIRCd
*/
int
cr_loadconf (char *filename, int type)
{
  char buf[2048];
  char *x, *y;
  FILE *f;
  f = fopen (filename, "r");
  if (!f)
    return -1;
  while (fgets (buf, 2048, f))
  {
    if (buf[0] == '#' || buf[0] == '/' || buf[0] == '\0'
        || buf[0] == '\n' || buf[0] == '\r')
      continue;
    iCstrip (buf);
    x = strtok (buf, " ");
    if (irccmp (x, "allow") == 0)
    {
      y = strtok (NULL, " ");
      if (!y)
        continue;
      cr_add (y, 0);
    }
    else if (irccmp (x, "msg") == 0)
    {
      y = strtok (NULL, "");
      if (!y)
        continue;
      if (cannotjoin_msg)
        MyFree ((char *) cannotjoin_msg);
      cannotjoin_msg = MyMalloc (strlen (y) + 1);
      strcpy (cannotjoin_msg, y);
    }
  }
  return 0;
}

/*
** cr_rehash
*/
void
cr_rehash (void)
{
  aCRline *p, q;
  for (p = crlines; p; p = p->next)
  {
    if ((p->type == 0) || (p->type == 2))
    {
      q.next = cr_del (p);
      p = &q;
    }
  }
  cr_loadconf (IRCD_RESTRICT, 1);
}

/*
** cr_report from UnrealIRCd
**
** Adapted for UltimateIRCd by ShadowMaster
*/
void
cr_report (aClient * source_p)
{
  aCRline *tmp;
  char *filemask;
  if (!crlines)
  {
    sendto_one (source_p,
                ":%s %d %s :There are currently no channel restrictions in place on %s",
                me.name, RPL_SETTINGS, source_p->name, me.name);
    sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name,
                source_p->name);
    return;
  }

  sendto_one (source_p,
              ":%s %d %s :\2==================== Allowed Channels ====================\2",
              me.name, RPL_SETTINGS, source_p->name);
  sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS, source_p->name);
  for (tmp = crlines; tmp; tmp = tmp->next)
  {
    filemask = BadPtr (tmp->channel) ? "<NULL>" : tmp->channel;
    sendto_one (source_p, ":%s %d %s :%s", me.name,
                RPL_SETTINGS, source_p->name, filemask);
  }
  sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS, source_p->name);
  sendto_one (source_p,
              ":%s %d %s :\2=================================================================\2",
              me.name, RPL_SETTINGS, source_p->name);
  sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name, source_p->name);
}

/* End Channel Restrict */


/*
 * m_makepass based on the code by Nelson Minar (minar@reed.edu)
 *
 * Copyright (C) 1991, all rights reserved.
 *
 *	parv[0] = sender prefix
 *	parv[1] = password to encrypt
 */
#ifdef HAVE_LIBCRYPT
int
m_makepass (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  static char saltChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
  char salt[3];
  extern char *crypt ();
  int useable = 0;
  if (parc > 1)
  {
    if (strlen (parv[1]) >= 1)
      useable = 1;
  }

  if (useable == 0)
  {
    sendto_one (source_p,
                ":%s NOTICE %s :*** Encryption's MUST be atleast 1 character in length",
                me.name, parv[0]);
    return 0;
  }
  srandom (time (0));
  salt[0] = saltChars[random () % 64];
  salt[1] = saltChars[random () % 64];
  salt[2] = 0;
  if ((strchr (saltChars, salt[0]) == NULL)
      || (strchr (saltChars, salt[1]) == NULL))
  {
    sendto_one (source_p, ":%s NOTICE %s :*** Illegal salt %s",
                me.name, parv[0], salt);
    return 0;
  }


  sendto_one (source_p,
              ":%s NOTICE %s :*** Encryption for [%s] is %s",
              me.name, parv[0], parv[1], crypt (parv[1], salt));
  return 0;
}
#endif

#ifdef UNUSED
/* this isn't used anywhere 
** Fish (23/08/03)
*/
#define DYNCONF_ACCESS_FMODE 1
#define DYNCONF_ACCESS_FJOIN 2

/*
 * check_dynconf_access
 *
 * Copyright (C) 2003, Thomas. J. Stens� and ShadowRealm Creations
 *
 * Small function to check if an operator is permitted to
 * carry out specific commands controlled by dynconf level settings.
 *
 * returns 1 if oper have permission
 *
 */

int
check_dynconf_access (aClient * client_p, int function)
{
  int i = 0;

  switch (function)
  {
     case DYNCONF_ACCESS_FMODE:
       if (i == check_dynconf_permission (client_p, DYNCONF_ACCESS_FMODE))
       {
         return 1;
       }
       else
       {
         return 0;
       }
       break;
     case DYNCONF_ACCESS_FJOIN:
       break;

     default:                  /* wtf? */
       return 0;
  }
  return 0;
}

int
check_dynconf_permission (aClient * client_p, int function)
{
  int i;

  switch (function)
  {
     case DYNCONF_ACCESS_FMODE:
       i = OPER_MODE_OVERRIDE;

     case DYNCONF_ACCESS_FJOIN:

     default:                  /* wtf? */
       return 0;
  }

  switch (i)
  {
     case 0:
       /*
        * Command is disabled
        */
       return 0;
       break;

     case 1:
       /*
        * Command is availible to all Operators
        */
       if (IsAnOper (client_p))
       {
         return 1;
       }
       break;

     case 2:
       /*
        * Command is availible to all Global Operators
        */
       if (IsOper (client_p))
       {
         return 1;
       }
       break;

     case 3:
       /*
        * Command is availible to all Services Operators
        * and Tech/Net Admins.
        */
       if (IsOper (client_p)
           && (IsSkoServicesStaff (client_p) || IsSkoTechAdmin (client_p)
               || IsSkoNetAdmin (client_p)))
       {
         return 1;
       }
       break;

     case 4:
       /*
        * Command is availible to all Services Administrators
        * and Tech/Net Admins.
        */
       if (IsOper (client_p)
           && (IsServicesAdmin (client_p) || IsServicesRoot (client_p)
               || IsSkoTechAdmin (client_p) || IsSkoNetAdmin (client_p)))
       {
         return 1;
       }
       break;

     case 5:
       /*
        * Command is availible to all administrators except Guest Admins
        */
       if (IsSkoAdmin (client_p))
       {
         return 1;
       }
       break;

     case 6:
       /*
        * Command is availible to Tech Admins and Net Admins
        */
       if (IsSkoTechAdmin (client_p) || IsSkoNetAdmin (client_p))
       {
         return 1;
       }
       break;

     case 7:
       /*
        * Command is availible to Net admins
        */
       if (IsSkoNetAdmin (client_p))
       {
         return 1;
       }
       break;

     default:                  /* wtf? */
       return 0;
  }
  return 0;
}
#endif
