/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/dynconf.c
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
 *  $Id: dynconf.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#define DYNCONF_C
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include "config.h"
#include "dynconf.h"
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
# include <utmp.h>
#endif
#include <fcntl.h>
#include "h.h"
#if defined( HAVE_STRING_H )
# include <string.h>
#else
# include <strings.h>
# undef strchr
# define strchr index
#endif

#define DoDebug fprintf(stderr, "[%s] %s | %li\n", babuf, __FILE__, __LINE__);
#define AllocCpy(x,y) if ((x) && type == 1) MyFree((x)); x = (char *) MyMalloc(strlen(y) + 1); strcpy(x,y)
#define XtndCpy(x,y) x = (char *) MyMalloc(strlen(y) + 2); *x = '\0'; strcat(x, "*"); strcpy(x,y)


int report_dynconf (aClient *);
int report_network (aClient *);
int report_options (aClient *);
int report_hostmasks (aClient *);
int report_services (aClient *);
int report_misc (aClient *);

/* internals */
aConfiguration iConf;
int icx = 0;
char buf[1024];


/* strips \r and \n's from the line */
void
iCstrip (char *line)
{
  char *c;

  if ((c = strchr (line, '\n')))
    *c = '\0';
  if ((c = strchr (line, '\r')))
    *c = '\0';
}

/* this loads dynamic DCONF */
int
load_conf (char *filename, int type)
{
  FILE *zConf;
  char *myversion = NULL;
  char *i;

  zConf = fopen (filename, "r");

  if (!zConf)
  {
    if (type == 1)
    {
      sendto_realops ("Config Error: Could not open the config file %s", filename);
      return -1;
    }
    else
    {
      fprintf (stderr, "===== ERROR =====\n");
      fprintf (stderr, "Config Error: Could not open the config file %s\n",filename);
      fprintf (stderr, "=============================================================\n");
      exit (-1);
    }
  }
  i = fgets (buf, 1023, zConf);
  if (!i)
  {
    if (type == 1)
    {
      sendto_realops ("Config Fatal Error: Unable to read the config file %s", filename);
      return -1;
    }
    else
    {
      fprintf (stderr, "===== ERROR =====\n");
      fprintf (stderr, "Config Fatal Error: Unable to read the config file %s\n",filename);
      fprintf (stderr, "=============================================================\n");
      exit (-1);
    }
  }
  iCstrip (buf);
  myversion = strtok (buf, "^");
  myversion = strtok (NULL, "");
  /* is this an ircd.ini file? */
  if (!match ("1.*", myversion))
  {
    /* wrong version */
    if (strcmp (myversion, DYNCONF_CONF_VERSION))
    {
      if (type == 1)
      {
        sendto_realops
          ("Config Warning: %s is version %s. Expected version: %s - Please upgrade.",
           filename, myversion, DYNCONF_CONF_VERSION);
      }
      else
      {
        fprintf (stderr,
                 "Config Warning: %s is version %s. Expected version: %s - Please upgrade.\n",
                 filename, myversion, DYNCONF_CONF_VERSION);
      }
    }
    load_conf2 (zConf, filename, type);
    return -1;
  }
  else if (!match ("2.*", myversion))
  {
    /* network file */
    /* wrong version */
    if (strcmp (myversion, DYNCONF_NETWORK_VERSION))
    {
      if (type == 1)
      {
        sendto_realops
          ("Config Warning: %s is version %s. Expected version: %s - Please upgrade.",
           filename, myversion, DYNCONF_NETWORK_VERSION);
      }
      else
      {
        fprintf (stderr,
                 "Config Warning: %s is version %s. Expected version: %s - Please upgrade.\n",
                 filename, myversion, DYNCONF_NETWORK_VERSION);
      }
    }
    load_conf3 (zConf, filename, type);
    return -1;
  }
  else
  {
    if (type == 1)
    {
      sendto_realops
        ("Config Fatal Error: Malformed version header in %s.", INCLUDE);
      return -1;
    }
    else
    {
      fprintf (stderr, "===== ERROR =====\n");
      fprintf (stderr,
               "Config Fatal Error: Malformed or missing version header in %s.\n",
               INCLUDE);
      fprintf (stderr,
               "=============================================================\n");
      exit (-1);
    }
    return -1;
  }
  return -1;
}

int
load_conf2 (FILE * myconf, char *filename, int type)
{
  char *databuf = buf;
  char *mystat = databuf;
  char *p, *setto, *var;
  char tmp[128];

  if (!myconf)
    return -1;

  *databuf = '\0';

  /* loop to read data */
  while (mystat != NULL)
  {
    mystat = fgets (buf, 1020, myconf);
    if ((*buf == '#') || (*buf == '/'))
      continue;

    iCstrip (buf);
    if (*buf == '\0')
      continue;
    p = strtok (buf, " ");

    if (irccmp (p, "Include") == 0)
    {
      strtok (NULL, " ");
      setto = strtok (NULL, "");
      AllocCpy (INCLUDE, setto);

      ircsnprintf (tmp, 128, "%s/%s", ETCPATH, setto);

      load_conf (tmp, type);
    }
    else if (irccmp (p, "Set") == 0)
    {
      var = strtok (NULL, " ");
      if (var == NULL)
        continue;
      if (*var == '\0')
        continue;

      strtok (NULL, " ");
      setto = strtok (NULL, "");

      /*
       * HUB
       */
      if (strcmp (var, "hub") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hub in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set hub in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hub in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set hub in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        HUB = atoi (setto);
      }
      /*
       * CLIENT_FLOOD
       */
      else if (strcmp (var, "client_flood") == 0)
      {
        if (setto == NULL)
        {
          setto = "2560";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set client_flood in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set client_flood in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "2560";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set client_flood in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set client_flood in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        CLIENT_FLOOD = atol (setto);
      }
      /*
       * MAX_CHANNELS_PER_USER
       */
      else if (strcmp (var, "max_channels_per_user") == 0)
      {
        if (setto == NULL)
        {
          setto = "10";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "10";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        MAXCHANNELSPERUSER = atol (setto);
      }
      /*
       * GEO_LOCATION
       */
      else if (strcmp (var, "geo_location") == 0)
      {
        if (setto == NULL)
        {
          setto = "Somewhere on Earth, in the Solar System";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set geo_location in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set geo_location in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        AllocCpy (GEO_LOCATION, setto);
      }
      /*
       * SERVER_KLINE_ADDRESS
       */
      else if (strcmp (var, "server_kline_address") == 0)
      {
        if (setto == NULL)
        {
          setto = "Admin@PoorlyConfiguredServer.com";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set server_kline_address in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set server_kline_address in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "Admin@PoorlyConfiguredServer.com";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set server_kline_address in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set server_kline_address in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        if (!strchr (setto, '@'))
        {
          setto = "Admin@PoorlyConfiguredServer.com";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set server_kline_address in ircd.ini is invalid (Is not a valid Email-Address). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set server_kline_address in ircd.ini is invalid (Is not a valid Email-Adress). Resetting to default.");
        }
        AllocCpy (SERVER_KLINE_ADDRESS, setto);
      }
      /*
       * WINGATE_NOTICE
       */
      else if (strcmp (var, "wingate_notice") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set wingate_notice in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set wingate_notice in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set wingate_notice in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set wingate_notice in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        WINGATE_NOTICE = atoi (setto);
      }
      /*
       * MONITOR_HOST
       */
      else if (strcmp (var, "monitor_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "127.0.0.1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set monitor_host in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set monitor_host in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "127.0.0.1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set monitor_host in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set monitor_host in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        AllocCpy (MONITOR_HOST, setto);
      }
      /*
       * MONITOR_URL
       */
      else if (strcmp (var, "monitor_url") == 0)
      {
        if (setto == NULL)
        {
          setto = "http://www.PorlyConfiguredServer.com/proxy/";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set monitor_url in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set monitor_url in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "http://www.PorlyConfiguredServer.com/proxy/";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set monitor_url in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set monitor_url in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        AllocCpy (MONITOR_URL, setto);
      }
      /*
       * DEF_MODE_i
       */
      else if (strcmp (var, "def_mode_i") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set def_mode_i in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set def_mode_i in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set def_mode_i in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set def_mode_i in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        DEF_MODE_i = atoi (setto);
      }
      /*
       * SHORT_MOTD
       */
      else if (strcmp (var, "short_motd") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set short_motd in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set short_motd in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set short_motd in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set short_motd in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        SHORT_MOTD = atoi (setto);
      }
      /*
       * LOG_FLAGS
       */
      else if (!strcmp (var, "log_lcritical"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_CRITICAL;
        else
          LOG_FLAGS &= ~LOGF_CRITICAL;
      }
      else if (!strcmp (var, "log_lclients"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_CLIENTS;
        else
          LOG_FLAGS &= ~LOGF_CLIENTS;
      }
      else if (!strcmp (var, "log_lrouting"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_ROUTING;
        else
          LOG_FLAGS &= ~LOGF_ROUTING;
      }
      else if (!strcmp (var, "log_loper"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_OPER;
        else
          LOG_FLAGS &= ~LOGF_OPER;
      }
      else if (!strcmp (var, "log_loperact"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_OPERACT;
        else
          LOG_FLAGS &= ~LOGF_OPERACT;
      }
      else if (!strcmp (var, "log_lkills"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_KILLS;
        else
          LOG_FLAGS &= ~LOGF_KILLS;
      }
      else if (!strcmp (var, "log_lopchat"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_OPCHAT;
        else
          LOG_FLAGS &= ~LOGF_OPCHAT;
      }
      else if (!strcmp (var, "log_lautokill"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_AUTOKILL;
        else
          LOG_FLAGS &= ~LOGF_AUTOKILL;
      }
      else if (!strcmp (var, "log_lstats"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_STATS;
        else
          LOG_FLAGS &= ~LOGF_STATS;
      }
      else if (!strcmp (var, "log_ldebug"))
      {
        if (setto && !strcmp (setto, "1"))
          LOG_FLAGS |= LOGF_DEBUG;
        else
          LOG_FLAGS &= ~LOGF_DEBUG;
      }

      /*
       * LOG_CYCLE
       */
      else if (strcmp (var, "log_cycle") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set log_cycle in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set log_cycle in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set log_cycle in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set log_cycle in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        if (type == 0)
          LOG_CYCLE = atoi (setto);
      }

      /*
       * LOG_CYCLE_RENAMEAFTER
       */
      else if (strcmp (var, "log_cycle_renameafter") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set log_cycle_renameafter in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set log_cycle_renameafter in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set log_cycle_renameafter in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set log_cycle_renameafter in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        if (type == 0)
          LOG_CYCLE_RENAMEAFTER = atoi (setto);
      }

      /*
       * LOG_MULTIFILES
       */
      else if (strcmp (var, "log_multifiles") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set log_multifiles in ircd.ini is invalid (NULL value). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set log_multifiles in ircd.ini is invalid (NULL value). Resetting to default.");
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set log_multifiles in ircd.ini is invalid (Contains spaces). Resetting to default.\n");
          else
            sendto_realops
              ("Dynconf ERROR: Set log_multifiles in ircd.ini is invalid (Contains spaces). Resetting to default.");
        }
        if (type == 0)
          LOG_MULTIFILES = atoi (setto);
      }

      else
      {
        if (type == 0)
          fprintf (stderr,
                   "Dynconf Warning: Invalid Set %s %s setting in %s \n",
                   var, setto, filename);
        else
          sendto_realops
            ("Dynconf Warning: Invalid Set %s %s setting in %s ", var,
             setto, filename);
      }

    }
    else
    {

      if (type == 0)
      {
        fprintf (stderr,
                 "Dynconf Warning: %s in %s is not a known setting to me!\n",
                 p, filename);
      }
      else
      {
        sendto_realops
          ("Dynconf Warning: %s in %s is not a known setting to me!",
           p, filename);
      }
    }
  }
  if (type == 0)
  {
    fprintf (stderr, "Loaded config file: %s\n", filename);
  }
  else
  {
    sendto_realops ("Loaded %s", filename);
  }
  return 0;
}

/* Load .network options */
int
load_conf3 (FILE * myconf, char *filename, int type)
{
  char *databuf = buf;
  char *mystat = databuf;
  char *p, *setto, *var;
  if (!myconf)
    return -1;

  *databuf = '\0';

  /* loop to read data */
  while (mystat != NULL)
  {
    mystat = fgets (buf, 1020, myconf);
    if ((*buf == '#') || (*buf == '/'))
      continue;

    iCstrip (buf);
    if (*buf == '\0')
      continue;

    p = strtok (buf, " ");
    if (irccmp (p, "Set") == 0)
    {
      var = strtok (NULL, " ");
      if (var == NULL)
        continue;
      if (*var == '\0')
        continue;

      strtok (NULL, " ");
      setto = strtok (NULL, "");
      /*
       * IRCNETWORK
       */
      if (strcmp (var, "ircnetwork") == 0)
      {
        if (setto == NULL)
        {
          setto = "SomeIRC";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set ircnetwork in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set ircnetwork in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "SomeIRC";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set ircnetwork in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set ircnetwork in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (IRCNETWORK, setto);
      }
      /*
       * IRCNETWORK_NAME
       */
      else if (strcmp (var, "ircnetwork_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "The Some IRC Network";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set ircnetwork_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set ircnetwork_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (IRCNETWORK_NAME, setto);
      }
      /*
       * DEFSERV
       */
      else if (strcmp (var, "defserv") == 0)
      {
        if (setto == NULL)
        {
          setto = "irc.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set defserv in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set defserv in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "irc.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set defserv in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set defserv in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (DEFSERV, setto);
      }
      /*
       * WEBSITE
       */
      else if (strcmp (var, "website") == 0)
      {
        if (setto == NULL)
        {
          setto = "http://www.SomeIRC.net/";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set website in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set website in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "http://www.SomeIRC.net/";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set website in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set website in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (WEBSITE, setto);
      }
      /*
       * RULES_PAGE
       */
      else if (strcmp (var, "rules_page") == 0)
      {
        if (setto == NULL)
        {
          setto = "http://www.SomeIRC.net/rules/";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set rules_page in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set rules_page in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "http://www.SomeIRC.net/rules/";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set rules_page in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set rules_page in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (RULES_PAGE, setto);
      }
      /*
       * NETWORK_KLINE_ADDRESS
       */
      else if (strcmp (var, "network_kline_address") == 0)
      {
        if (setto == NULL)
        {
          setto = "KLine@SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set network_kline_address in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set network_kline_address in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "KLine@SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set network_kline_address in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set network_kline_address in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        if (!strchr (setto, '@'))
        {
          setto = "KLine@SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set network_kline_address in %s is invalid (Is not a valid Email-Address). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set network_kline_address in %s is invalid (Is not a valid Email-Adress). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (NETWORK_KLINE_ADDRESS, setto);
      }
      /*
       * MODE_x
       */
      else if (strcmp (var, "mode_x") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set mode_x in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set mode_x in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set mode_x in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set mode_x in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        MODE_x = atoi (setto);
      }
      /*
       * MODE_x_LOCK
       */
      else if (strcmp (var, "mode_x_lock") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set mode_x_lock in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set mode_x_lock in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set mode_x_lock in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set mode_x_lock in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        MODE_x_LOCK = atoi (setto);
      }
      /*
       * CLONE_PROTECTION
       */
      else if (strcmp (var, "clone_protection") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set clone_protection in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set clone_protection in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set clone_protection in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set clone_protection in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        CLONE_PROTECTION = atoi (setto);
      }
      /*
       * CLONE_LIMIT
       */
      else if (strcmp (var, "clone_limit") == 0)
      {
        if (setto == NULL)
        {
          setto = "5";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set clone_limit in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set clone_limit in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "5";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set clone_limit in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set clone_limit in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        CLONE_LIMIT = atol (setto);
      }
      /*
       * OPER_CLONE_LIMIT_BYPASS
       */
      else if (strcmp (var, "oper_clone_limit_bypass") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_clone_limit_bypass in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_clone_limit_bypass in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_clone_limit_bypass in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_clone_limit_bypass in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        OPER_CLONE_LIMIT_BYPASS = atoi (setto);
      }
      /*
       * HIDEULINEDSERVS
       */
      else if (strcmp (var, "hideulinedservs") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hideulinedservs in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set hideulinedservs in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hideulinedservs in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set hideulinedservs in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        HIDEULINEDSERVS = atoi (setto);
      }
      /*
       * GLOBAL_CONNECTS
       */
      else if (strcmp (var, "global_connects") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set global_connects in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set global_connects in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set global_connects in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set global_connects in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        GLOBAL_CONNECTS = atoi (setto);
      }
      /*
       * OPER_AUTOPROTECT
       */
      else if (strcmp (var, "oper_autoprotect") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_autoprotect in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_autoprotect in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_autoprotect in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_autoprotect in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        OPER_AUTOPROTECT = atoi (setto);
      }
      /*
       * OPER_MODE_OVERRIDE
       */
      else if (strcmp (var, "oper_mode_override") == 0)
      {
        if (setto == NULL)
        {
          setto = "3";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_mode_override in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_mode_override in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "3";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_mode_override in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_mode_override in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        OPER_MODE_OVERRIDE = atol (setto);
      }
      /*
       * OPER_MODE_OVERRIDE_NOTICE
       */
      else if (strcmp (var, "oper_mode_override_notice") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_mode_override_notice in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_mode_override_notice in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_mode_override_notice in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_mode_override_notice in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        OPER_MODE_OVERRIDE_NOTICE = atoi (setto);
      }
      /*
       * USERS_AUTO_JOIN
       */
      else if (strcmp (var, "users_auto_join") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set users_auto_join in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set users_auto_join in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set users_auto_join in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set users_auto_join in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (USERS_AUTO_JOIN, setto);
      }
      /*
       * OPERS_AUTO_JOIN
       */
      else if (strcmp (var, "opers_auto_join") == 0)
      {
        if (setto == NULL)
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set opers_auto_join in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set opers_auto_join in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "0";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set opers_auto_join in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set opers_auto_join in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (OPERS_AUTO_JOIN, setto);
      }
      /*
       * SERVICES_SERVER
       */
      else if (strcmp (var, "services_server") == 0)
      {
        if (setto == NULL)
        {
          setto = "services.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set services_server in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set services_server in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "services.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set services_server in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set services_server in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (SERVICES_SERVER, setto);
      }
      /*
       * STATSERV_SERVER
       */
      else if (strcmp (var, "statserv_server") == 0)
      {
        if (setto == NULL)
        {
          setto = "statistics.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set statserv_server in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set statserv_server in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "statistics.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set statserv_server in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set statserv_server in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (STATSERV_SERVER, setto);
      }
      /*
       * HOSTSERV_SERVER
       */
      else if (strcmp (var, "hostserv_server") == 0)
      {
        if (setto == NULL)
        {
          setto = "hostserv.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hostserv_server in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set hostserv_server in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "hostserv.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hostserv_server in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set hostserv_server in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (HOSTSERV_SERVER, setto);
      }
      /*
       * ENABLE_NICKSERV_ALIAS
       */
      else if (strcmp (var, "enable_nickserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_nickserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_nickserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_nickserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_nickserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_NICKSERV_ALIAS = atoi (setto);
      }
      /*
       * NICKSERV_NAME
       */
      else if (strcmp (var, "nickserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "nickserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set nickserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set nickserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "nickserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set nickserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set nickserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (NICKSERV_NAME, setto);
      }
      /*
       * ENABLE_CHANSERV_ALIAS
       */
      else if (strcmp (var, "enable_chanserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_chanserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_chanserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_chanserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_chanserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_CHANSERV_ALIAS = atoi (setto);
      }
      /*
       * CHANSERV_NAME
       */
      else if (strcmp (var, "chanserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "chanserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set chanserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set chanserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "chanserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set chanserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set chanserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (CHANSERV_NAME, setto);
      }
      /*
       * ENABLE_MEMOSERV_ALIAS
       */
      else if (strcmp (var, "enable_memoserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_memoserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_memoserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_memoserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_memoserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_MEMOSERV_ALIAS = atoi (setto);
      }
      /*
       * MEMOSERV_NAME
       */
      else if (strcmp (var, "memoserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "memoserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set memoserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set memoserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "memoserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set memoserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set memoserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (MEMOSERV_NAME, setto);
      }
      /*
       * ENABLE_BOTSERV_ALIAS
       */
      else if (strcmp (var, "enable_botserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_botserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_botserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_botserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_botserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_BOTSERV_ALIAS = atoi (setto);
      }
      /*
       * BOTSERV_NAME
       */
      else if (strcmp (var, "botserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "botserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set botserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set botserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "botserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set botserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set botserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (BOTSERV_NAME, setto);
      }
      /*
       * ENABLE_ROOTSERV_ALIAS
       */
      else if (strcmp (var, "enable_rootserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_rootserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_rootserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_rootserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_rootserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_ROOTSERV_ALIAS = atoi (setto);
      }
      /*
       * ROOTSERV_NAME
       */
      else if (strcmp (var, "rootserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "rootserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set rootserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set rootserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "rootserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set rootserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set rootserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (ROOTSERV_NAME, setto);
      }
      /*
       * ENABLE_OPERSERV_ALIAS
       */
      else if (strcmp (var, "enable_operserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_operserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_operserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_operserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_operserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_OPERSERV_ALIAS = atoi (setto);
      }
      /*
       * OPERSERV_NAME
       */
      else if (strcmp (var, "operserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "operserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set operserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set operserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "operserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set operserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set operserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (OPERSERV_NAME, setto);
      }
      /*
       * ENABLE_STATSERV_ALIAS
       */
      else if (strcmp (var, "enable_statserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_statserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_statserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_statserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_statserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_STATSERV_ALIAS = atoi (setto);
      }
      /*
       * STATSERV_NAME
       */
      else if (strcmp (var, "statserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "statserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set statserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set statserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "statserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set statserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set statserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (STATSERV_NAME, setto);
      }
      /*
       * ENABLE_HOSTSERV_ALIAS
       */
      else if (strcmp (var, "enable_hostserv_alias") == 0)
      {
        if (setto == NULL)
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_hostserv_alias in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_hostserv_alias in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "1";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set enable_hostserv_alias in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set enable_hostserv_alias in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        ENABLE_HOSTSERV_ALIAS = atoi (setto);
      }
      /*
       * HOSTSERV_NAME
       */
      else if (strcmp (var, "hostserv_name") == 0)
      {
        if (setto == NULL)
        {
          setto = "hostserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hostserv_name in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set hostserv_name in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "hostserv";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set hostserv_name in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set hostserv_name in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (HOSTSERV_NAME, setto);
      }
      /*
       * LOCOP_HOST
       */
      else if (strcmp (var, "locop_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "LocalOper.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set locop_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set locop_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "LocalOper.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set locop_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set locop_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (LOCOP_HOST, setto);
      }
      /*
       * OPER_HOST
       */
      else if (strcmp (var, "oper_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Oper.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Oper.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set oper_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set oper_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (OPER_HOST, setto);
      }
      /*
       * GUESTADMIN_HOST
       */
      else if (strcmp (var, "guestadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Guest-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set guestadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set guestadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Guest-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set guestadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set guestadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (GUESTADMIN_HOST, setto);
      }
      /*
       * SERVCOADMIN_HOST
       */
      else if (strcmp (var, "servcoadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Server-CoAdmin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set servcoadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set servcoadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Server-CoAdmin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set servcoadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set servcoadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (SERVCOADMIN_HOST, setto);
      }
      /*
       * SERVADMIN_HOST
       */
      else if (strcmp (var, "servadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Server-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set servadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set servadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Server-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set servadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set servadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (SERVADMIN_HOST, setto);
      }
      /*
       * TECHCOADMIN_HOST
       */
      else if (strcmp (var, "techcoadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Technical-CoAdmin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set techcoadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set techcoadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Technical-CoAdmin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set techcoadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set techcoadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (TECHCOADMIN_HOST, setto);
      }

      /*
       * TECHADMIN_HOST
       */
      else if (strcmp (var, "techadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Technical-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set techadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set techadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Technical-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set techadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set techadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (TECHADMIN_HOST, setto);
      }
      /*
       * NETCOADMIN_HOST
       */
      else if (strcmp (var, "netcoadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Network-CoAdmin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set netcoadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set netcoadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Network-CoAdmin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set netcoadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set netcoadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (NETCOADMIN_HOST, setto);
      }
      /*
       * NETADMIN_HOST
       */
      else if (strcmp (var, "netadmin_host") == 0)
      {
        if (setto == NULL)
        {
          setto = "Network-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set netadmin_host in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set netadmin_host in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "Network-Admin.SomeIRC.net";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set netadmin_host in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set netadmin_host in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (NETADMIN_HOST, setto);
      }
      /*
       * HELPCHAN
       */
      else if (strcmp (var, "helpchan") == 0)
      {
        if (setto == NULL)
        {
          setto = "#help";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set helpchan in %s is invalid (NULL value). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set helpchan in %s is invalid (NULL value). Resetting to default.",
               INCLUDE);
        }
        if (strchr (setto, ' '))
        {
          setto = "#help";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set helpchan in %s is invalid (Contains spaces). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set helpchan in %s is invalid (Contains spaces). Resetting to default.",
               INCLUDE);
        }
        if (!strchr (setto, '#'))
        {
          setto = "#help";

          if (type == 0)
            fprintf (stderr,
                     "Dynconf ERROR: Set helpchan in %s is invalid (Is no a valid channel). Resetting to default.\n",
                     INCLUDE);
          else
            sendto_realops
              ("Dynconf ERROR: Set helpchan in %s is invalid (Is no a valid channel). Resetting to default.",
               INCLUDE);
        }
        AllocCpy (HELPCHAN, setto);
      }
      else
      {
        if (type == 0)
          fprintf (stderr,
                   "Dynconf Warning: Invalid Set %s %s setting in %s \n",
                   var, setto, INCLUDE);
        else
          sendto_realops
            ("Dynconf Warning: Invalid Set %s %s setting in %s ", var,
             setto, INCLUDE);
      }

    }
    else
    {

      if (type == 0)
      {
        fprintf (stderr,
                 "Dynconf Warning: %s in %s is not a known setting to me!\n",
                 p, INCLUDE);
      }
      else
      {
        sendto_realops
          ("Dynconf Warning: %s in %s is not a known setting to me!",
           p, INCLUDE);
      }
    }

  }
  if (type == 0)
  {
    fprintf (stderr, "Loaded config file: %s\n", INCLUDE);
  }
  else
  {
    sendto_realops ("Loaded %s", INCLUDE);
  }
  return 0;
}


void
init_dynconf (void)
{
  bzero (&iConf, sizeof (iConf));
}

/* Report the unrealircd.conf info -codemastr*/

int
report_dynconf (aClient * source_p)
{
  sendto_one (source_p,
              ":%s %d %s :\2==================== Dynconf Settings ====================\2",
              me.name, RPL_SETTINGS, source_p->name);

  sendto_one (source_p, ":%s %d %s :", me.name, RPL_SETTINGS, source_p->name);


  sendto_one (source_p, ":%s %d %s :Include: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, INCLUDE);

  sendto_one (source_p, ":%s %d %s :HUB: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, HUB);

  sendto_one (source_p, ":%s %d %s :Flood Level: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, CLIENT_FLOOD);

  sendto_one (source_p, ":%s %d %s :Max channels per user: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, MAXCHANNELSPERUSER);

  sendto_one (source_p, ":%s %d %s :Geo Location: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, GEO_LOCATION);

  sendto_one (source_p, ":%s %d %s :Server KLined Address: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, SERVER_KLINE_ADDRESS);

  sendto_one (source_p, ":%s %d %s :Wingate Notice: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, WINGATE_NOTICE);

  sendto_one (source_p, ":%s %d %s :Wingate Monitor Host: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, MONITOR_HOST);

  sendto_one (source_p, ":%s %d %s :Wingate Monitor URL: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, MONITOR_URL);

  sendto_one (source_p, ":%s %d %s :Usermode i on login: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, DEF_MODE_i);

  sendto_one (source_p, ":%s %d %s :Short MOTD: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, SHORT_MOTD);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);

  sendto_one (source_p,
              ":%s %d %s :\2============================================================~\2",
              me.name, RPL_SETTINGS, source_p->name);

  sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name, source_p->name);

  return 0;
}

/* Report the network file info -codemastr */

int
report_network (aClient * source_p)
{
  sendto_one (source_p,
              ":%s %d %s :\2==================== Network Info ====================\2",
              me.name, RPL_SETTINGS, source_p->name);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);

  sendto_one (source_p, ":%s %d %s :Network File: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, IRCNETWORK);

  sendto_one (source_p, ":%s %d %s :Network Name: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, IRCNETWORK_NAME);

  sendto_one (source_p, ":%s %d %s :Default Server: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, DEFSERV);

  sendto_one (source_p, ":%s %d %s :Network Web Site: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, WEBSITE);

  sendto_one (source_p, ":%s %d %s :Network Rules Page: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, RULES_PAGE);

  sendto_one (source_p, ":%s %d %s :Network K-Line Address: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, NETWORK_KLINE_ADDRESS);


  sendto_one (source_p, ":%s %d %s :Network Help Channel: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, HELPCHAN);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);


  sendto_one (source_p,
              ":%s %d %s :\2============================================================~\2",
              me.name, RPL_SETTINGS, source_p->name);


  sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name, source_p->name);

  return 0;
}

/* Report Options */

int
report_options (aClient * source_p)
{
  char tmp[256];

  sendto_one (source_p,
              ":%s %d %s :\2==================== Network Options ====================\2",
              me.name, RPL_SETTINGS, source_p->name);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);

  sendto_one (source_p, ":%s %d %s :Client Hiddenhosts (+x): \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, MODE_x);

  sendto_one (source_p, ":%s %d %s :Client Hiddenhosts locked on: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, MODE_x_LOCK);

  sendto_one (source_p, ":%s %d %s :Clone Protection: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, CLONE_PROTECTION);

  sendto_one (source_p, ":%s %d %s :Allowed Connections Per Host: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, CLONE_LIMIT);

  sendto_one (source_p, ":%s %d %s :Oper Bypass Connection Limit: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, OPER_CLONE_LIMIT_BYPASS);

  sendto_one (source_p, ":%s %d %s :Hide U Lined Servers: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, HIDEULINEDSERVS);

  sendto_one (source_p, ":%s %d %s :Show Global Connect Notices: \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, GLOBAL_CONNECTS);

  sendto_one (source_p, ":%s %d %s :Auto Set +p (Protected Oper): \2%i\2",
              me.name, RPL_SETTINGS, source_p->name, OPER_AUTOPROTECT);

  if (OPER_MODE_OVERRIDE == 0)
    strcpy (tmp, "Disabeled");
  else if (OPER_MODE_OVERRIDE == 1)
    strcpy (tmp, "All Operators");
  else if (OPER_MODE_OVERRIDE == 2)
    strcpy (tmp, "All Global Operators");
  else if (OPER_MODE_OVERRIDE == 3)
    strcpy (tmp, "All Services Staff and All Administrators");
  else if (OPER_MODE_OVERRIDE == 4)
    strcpy (tmp, "All Administrators");
  else if (OPER_MODE_OVERRIDE == 5)
    strcpy (tmp, "All Network Administrators");

  sendto_one (source_p, ":%s %d %s :Oper Mode Override Level: \2%s (%l)\2 ",
              me.name, RPL_SETTINGS, source_p->name, tmp, OPER_MODE_OVERRIDE);

  sendto_one (source_p, ":%s %d %s :Oper Mode Override Notice: \2%i\2 ",
              me.name, RPL_SETTINGS, source_p->name,
              OPER_MODE_OVERRIDE_NOTICE);

  sendto_one (source_p, ":%s %d %s :Users Auto Join Channel(s): \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, USERS_AUTO_JOIN);

  sendto_one (source_p, ":%s %d %s :Opers Auto Join Channel(s): \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, OPERS_AUTO_JOIN);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);


  sendto_one (source_p,
              ":%s %d %s :\2============================================================~\2",
              me.name, RPL_SETTINGS, source_p->name);


  sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name, source_p->name);

  return 0;
}


/* Report Hostmasks */

int
report_hostmasks (aClient * source_p)
{
  sendto_one (source_p,
              ":%s %d %s :\2==================== Network Hostmasks ====================\2",
              me.name, RPL_SETTINGS, source_p->name);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);

  sendto_one (source_p, ":%s %d %s :Local Operator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, LOCOP_HOST);

  sendto_one (source_p, ":%s %d %s :Global Operator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, OPER_HOST);

  sendto_one (source_p, ":%s %d %s :Guest Administrator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, GUESTADMIN_HOST);

  sendto_one (source_p, ":%s %d %s :Server Co Administrator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, SERVCOADMIN_HOST);

  sendto_one (source_p, ":%s %d %s :Server Administrator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, SERVADMIN_HOST);

  sendto_one (source_p,
              ":%s %d %s :Technical Co Administrator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, TECHCOADMIN_HOST);

  sendto_one (source_p, ":%s %d %s :Technical Administrator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, TECHADMIN_HOST);

  sendto_one (source_p,
              ":%s %d %s :Network Co Administrator Hostname: \2%s\2", me.name,
              RPL_SETTINGS, source_p->name, NETCOADMIN_HOST);

  sendto_one (source_p, ":%s %d %s :Network Administrator Hostname: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, NETADMIN_HOST);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);


  sendto_one (source_p,
              ":%s %d %s :\2============================================================~\2",
              me.name, RPL_SETTINGS, source_p->name);


  sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name, source_p->name);

  return 0;
}

/* Report services */

int
report_services (aClient * source_p)
{
  sendto_one (source_p,
              ":%s %d %s :\2==================== Services Settings ====================\2",
              me.name, RPL_SETTINGS, source_p->name);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);

  sendto_one (source_p, ":%s %d %s :Nickserv alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_NICKSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :ChanServ alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_CHANSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :MemoServ alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_MEMOSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :BotServ alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_BOTSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :RootServ lias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_ROOTSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :OperServ alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_OPERSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :StatServ alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_STATSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :HostServ alias: \2%d\2",
              me.name, RPL_SETTINGS, source_p->name, ENABLE_HOSTSERV_ALIAS);

  sendto_one (source_p, ":%s %d %s :Services Server: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, SERVICES_SERVER);

  sendto_one (source_p, ":%s %d %s :StatServ Server: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, STATSERV_SERVER);

  sendto_one (source_p, ":%s %d %s :HostServ Server: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, HOSTSERV_SERVER);

  sendto_one (source_p, ":%s %d %s :NickServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, NICKSERV_NAME);

  sendto_one (source_p, ":%s %d %s :ChanServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, CHANSERV_NAME);

  sendto_one (source_p, ":%s %d %s :MemoServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, MEMOSERV_NAME);

  sendto_one (source_p, ":%s %d %s :BotServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, BOTSERV_NAME);

  sendto_one (source_p, ":%s %d %s :RootServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, ROOTSERV_NAME);

  sendto_one (source_p, ":%s %d %s :OperServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, OPERSERV_NAME);

  sendto_one (source_p, ":%s %d %s :StatServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, STATSERV_NAME);

  sendto_one (source_p, ":%s %d %s :HostServ: \2%s\2",
              me.name, RPL_SETTINGS, source_p->name, HOSTSERV_NAME);

  sendto_one (source_p, ":%s %d %s : ", me.name, RPL_SETTINGS,
              source_p->name);


  sendto_one (source_p,
              ":%s %d %s :\2============================================================~\2",
              me.name, RPL_SETTINGS, source_p->name);


  sendto_one (source_p, rpl_str (RPL_ENDOFSETTINGS), me.name, source_p->name);

  return 0;
}

void
check_dynconf (int type)
{
/*
** Here we do the "simple" task of going trough every dynconf setting we have and make
** sure the user didnt do anything stupid. If he did do something stupid we yell at the user
** and reset the setting to the default value.
**
** I know this is ugly but i cant think of a safer way to ensure that the user havent done anything
** that will mess things up badly - ShadowMaster
*/

/*
** Network settings file content - ShadowMaster
*/


/*
** IRCNETWORK
*/

  if (IRCNETWORK == NULL)
  {
    AllocCpy (IRCNETWORK, "SomeIRC");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set ircnetwork in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set ircnetwork in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** IRCNETWORK_NAME
*/
  if (IRCNETWORK_NAME == NULL)
  {
    AllocCpy (IRCNETWORK_NAME, "SomeIRC");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set ircnetwork_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set ircnetwork_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** DEFSERV
*/

  if (DEFSERV == NULL)
  {
    AllocCpy (DEFSERV, "irc.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set defserv in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set defserv in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** WEBSITE
*/

  if (WEBSITE == NULL)
  {
    AllocCpy (WEBSITE, "http://www.SomeIRC.net/");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set website in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set website in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** RULES_PAGE
*/

  if (RULES_PAGE == NULL)
  {
    AllocCpy (DEFSERV, "http://www.SomeIRC.net/rules.html");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set rules_page in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set rules_page in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** NETWORK_KLINE_ADDRESS
*/

  if (NETWORK_KLINE_ADDRESS == NULL)
  {
    AllocCpy (DEFSERV, "KLine@SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set network_kline_address in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set network_kline_address in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** CLONE_LIMIT
*/
  if (!CLONE_LIMIT)
  {
    CLONE_LIMIT = '5';

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set clone_limit in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set clone_limit in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** OPER_MODE_OVERRIDE
*/
  if (!OPER_MODE_OVERRIDE)
  {
    OPER_MODE_OVERRIDE = '3';

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set oper_mode_override in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set opers_mode_override in %s does not exist. Setting it to default.",
         INCLUDE);
  }


/*
** USERS_AUTO_JOIN
*/
  if (USERS_AUTO_JOIN == NULL)
  {
    USERS_AUTO_JOIN = "0";

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set users_auto_join in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set users_auto_join in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** OPERS_AUTO_JOIN
*/
  if (OPERS_AUTO_JOIN == NULL)
  {
    OPERS_AUTO_JOIN = "0";

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set opers_auto_join in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set opers_auto_join in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** SERVICES_SERVER
*/
  if (SERVICES_SERVER == NULL)
  {
    AllocCpy (SERVICES_SERVER, "services.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set services_server in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set services_server in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** STATSERV_SERVER
*/
  if (STATSERV_SERVER == NULL)
  {
    AllocCpy (STATSERV_SERVER, "statistics.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set statserv_server in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set statserv_server in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** HOSTSERV_SERVER
*/
  if (HOSTSERV_SERVER == NULL)
  {
    AllocCpy (HOSTSERV_SERVER, "hostserv.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set hostserv_server in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set hostserv_server in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** NICKSERV_NAME
*/
  if (NICKSERV_NAME == NULL)
  {
    AllocCpy (NICKSERV_NAME, "nickserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set nickserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set nickserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** CHANSERV_NAME
*/
  if (CHANSERV_NAME == NULL)
  {
    AllocCpy (CHANSERV_NAME, "chanserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set chanserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set chanserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }


/*
** MEMOSERV_NAME
*/
  if (MEMOSERV_NAME == NULL)
  {
    AllocCpy (MEMOSERV_NAME, "memoserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set memoserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set memoserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }


/*
** BOTSERV_NAME
*/
  if (BOTSERV_NAME == NULL)
  {
    AllocCpy (BOTSERV_NAME, "botserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set botserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set botserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }


/*
** ROOTSERV_NAME
*/
  if (ROOTSERV_NAME == NULL)
  {
    AllocCpy (ROOTSERV_NAME, "rootserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set rootserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set rootserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }


/*
** OPERSERV_NAME
*/
  if (OPERSERV_NAME == NULL)
  {
    AllocCpy (OPERSERV_NAME, "operserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set operserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set operserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }


/*
** STATSERV_NAME
*/
  if (STATSERV_NAME == NULL)
  {
    AllocCpy (STATSERV_NAME, "statserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set statserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set statserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** HOSTSERV_NAME
*/
  if (HOSTSERV_NAME == NULL)
  {
    AllocCpy (HOSTSERV_NAME, "hostserv");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set hostserv_name in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set hostserv_name in %s does not exist. Setting it to default.",
         INCLUDE);
  }

/*
** LOCOP_HOST
*/
  if (LOCOP_HOST == NULL)
  {
    AllocCpy (LOCOP_HOST, "LocalOper.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set locop_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set locop_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** OPER_HOST
*/
  if (OPER_HOST == NULL)
  {
    AllocCpy (OPER_HOST, "Oper.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set oper_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set oper_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** GUESTADMIN_HOST
*/
  if (GUESTADMIN_HOST == NULL)
  {
    AllocCpy (GUESTADMIN_HOST, "Guest-Admin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set guestadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set guestadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** SERVCOADMIN_HOST
*/
  if (SERVCOADMIN_HOST == NULL)
  {
    AllocCpy (SERVCOADMIN_HOST, "Server-CoAdmin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set servcoadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set servcoadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** SERVADMIN_HOST
*/
  if (SERVADMIN_HOST == NULL)
  {
    AllocCpy (SERVADMIN_HOST, "Server-Admin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set servadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set servadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** TECHCOADMIN_HOST
*/
  if (TECHCOADMIN_HOST == NULL)
  {
    AllocCpy (TECHCOADMIN_HOST, "Tech-CoAdmin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set techcoadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set techcoadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** TECHADMIN_HOST
*/
  if (TECHADMIN_HOST == NULL)
  {
    AllocCpy (TECHADMIN_HOST, "Tech-Admin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set techadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set techadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** NETCOADMIN_HOST
*/
  if (NETCOADMIN_HOST == NULL)
  {
    AllocCpy (NETCOADMIN_HOST, "Net-CoAdmin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set netcoadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set netcoadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** NETADMIN_HOST
*/
  if (NETADMIN_HOST == NULL)
  {
    AllocCpy (NETADMIN_HOST, "Net-Admin.SomeIRC.net");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set netadmin_host in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set netadmin_host in %s does not exist. Setting it to default.",
         INCLUDE);
  }
/*
** HELPCHAN
*/
  if (HELPCHAN == NULL)
  {
    AllocCpy (HELPCHAN, "#help");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set helpchan in %s does not exist. Setting it to default.\n",
               INCLUDE);
    else
      sendto_realops
        ("Dynconf ERROR: Set helpchan in %s does not exist. Setting it to default.",
         INCLUDE);
  }



/*
** ircd.ini settings - ShadowMaster
*/

/*
** CLIENT_FLOOD
*/
  if (!CLIENT_FLOOD)
  {
    CLIENT_FLOOD = atol ("2560");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set client_flood in ircd.ini does not exist. Setting it to default.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set client_flood in ircd.ini does not exist. Setting it to default.");
  }
  if (CLIENT_FLOOD > 8000)
  {
    CLIENT_FLOOD = atol ("8000");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set client_flood in ircd.ini is invalid (Too large). Resetting to max value allowed.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set client_flood in ircd.ini is invalid (Too large). Resetting to max value allowed.");
  }
  if (CLIENT_FLOOD < 512)
  {
    CLIENT_FLOOD = atol ("512");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set client_flood in ircd.ini is invalid (Too low). Resetting to min value allowed.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set client_flood in ircd.ini is invalid (Too low). Resetting to min value allowed.");
  }

/*
** MAXCHANNELSPERUSER
*/
  if (!MAXCHANNELSPERUSER)
  {
    MAXCHANNELSPERUSER = atol ("10");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set max_channels_per_user in ircd.ini does not exist. Setting it to default.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set max_channels_per_user in ircd.ini does not exist. Setting it to default.");
  }
  if (MAXCHANNELSPERUSER > 30)
  {
    MAXCHANNELSPERUSER = atol ("30");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (Too large). Resetting to max value allowed.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (Too large). Resetting to max value allowed.");
  }
  if (MAXCHANNELSPERUSER < 1)
  {
    MAXCHANNELSPERUSER = atol ("1");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (Too low). Resetting to min value allowed.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set max_channels_per_user in ircd.ini is invalid (Too low). Resetting to min value allowed.");
  }

/*
** GEO_LOCATION
*/
  if (GEO_LOCATION == NULL)
  {
    AllocCpy (GEO_LOCATION, "Somewhere on Earth, in the Solar System");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set geo_location in ircd.ini does not exist. Setting it to default.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set geo_location in ircd.ini does not exist. Setting it to default.");
  }

/*
** SERVER_KLINE_ADDRESS
*/
  if (SERVER_KLINE_ADDRESS == NULL)
  {
    AllocCpy (SERVER_KLINE_ADDRESS, "Admin@PorlyConfiguredServer.com");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set server_kline_address in ircd.ini does not exist. Setting it to default.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set server_kline_address in ircd.ini does not exist. Setting it to default.");
  }

/*
** MONITOR_HOST
*/
  if (MONITOR_HOST == NULL)
  {
    AllocCpy (MONITOR_HOST, "127.0.0.1");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set monitor_host in ircd.ini does not exist. Setting it to default.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set monitor_host in ircd.ini does not exist. Setting it to default.");
  }

/*
** MONITOR_URL
*/
  if (MONITOR_URL == NULL)
  {
    AllocCpy (MONITOR_URL, "http://www.PorlyConfiguredServer.com/proxy/");

    if (type == 0)
      fprintf (stderr,
               "Dynconf ERROR: Set monitor_url in ircd.ini does not exist. Setting it to default.\n");
    else
      sendto_realops
        ("Dynconf ERROR: Set monitor_url in ircd.ini does not exist. Setting it to default.");
  }




/*
** INCLUDE
*/

	/* I have no clue what it actually means */
  if (type == 0)
    (void) fprintf (stderr, "Config parser settings verified.\n");
  else
    sendto_realops ("Config parser settings verified.");

}
