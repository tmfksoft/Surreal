/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/s_user.c
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
 *  $Id: s_user.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include "throttle.h"
#include <sys/stat.h>
#ifndef _WIN32
# include <utmp.h>
# include <sys/socket.h>
#else
# include <winsock2.h>
# include <io.h>
#endif
#include <fcntl.h>
#include "h.h"
#include "dynconf.h"
#include "supported.h"
#ifdef FLUD
#include "blalloc.h"
#endif /* FLUD  */
#include "userban.h"
#include "inet.h"               /*INET6 */
#if defined( HAVE_STRING_H)
#include <string.h>
#else
#include <strings.h>
#endif

int
do_user (char *, aClient *, aClient *, char *, char *, char *,
         unsigned long, unsigned int, char *, char *);


extern char motd_last_changed_date[];
extern char opermotd_last_changed_date[];

extern int send_motd (aClient *, aClient *, int, char **);
extern void send_topic_burst (aClient *);
extern void outofmemory (void); /*
                                 * defined in list.c 
                                 */

static int check_dccsend (aClient *, aClient *, char *);
static char *get_mode_str (aClient *);
static char *get_smode_str (aClient *);
static int add_dccallow (aClient *, aClient *);
static int del_dccallow (aClient *, aClient *);
int del_accept (aClient *, char *);

#ifdef MAXBUFFERS
extern void reset_sock_opts ();
extern int send_lusers (aClient *, aClient *, int, char **);

#endif
extern int lifesux;
extern void set_oper_access (aClient *, aConfItem *);

#ifdef WINGATE_NOTICE
extern char ProxyMonURL[TOPICLEN + 1];
extern char ProxyMonHost[HOSTLEN + 1];
#endif

static char buf[BUFSIZE], buf2[BUFSIZE];
int user_modes[] = {
  UMODE_a, 'a',                 /* Services Operator */
  UMODE_d, 'd',                 /* Deaf client */
  UMODE_h, 'h',                 /* HelpOp */
  UMODE_i, 'i',                 /* Invisible */
  UMODE_o, 'o',                 /* Global Oper */
  UMODE_p, 'p',                 /* Protected Operator */
  UMODE_r, 'r',                 /* Registered Nick */
  UMODE_x, 'x',                 /* Hiddenhost */

  UMODE_D, 'D',                 /* Seen DCC Warning Notice */
  UMODE_P, 'P',                 /* Services Admin */
  UMODE_R, 'R',                 /* Block messages from non registered users */
  UMODE_S, 'S',                 /* Services Client */
  UMODE_O, 'O',                 /* Local Oper */
  UMODE_W, 'W',                 /* Whois notices */
  UMODE_Z, 'Z',                 /* Services Root */

  0, 0
};

int user_mode_table[] = {
  /* 0 - 32 are control chars and space */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,                            /* ! */
  0,                            /* " */
  0,                            /* # */
  0,                            /* $ */
  0,                            /* % */
  0,                            /* & */
  0,                            /* ' */
  0,                            /* ( */
  0,                            /* ) */
  0,                            /* * */
  0,                            /* + */
  0,                            /* , */
  0,                            /* - */
  0,                            /* . */
  0,                            /* / */
  0,                            /* 0 */
  0,                            /* 1 */
  0,                            /* 2 */
  0,                            /* 3 */
  0,                            /* 4 */
  0,                            /* 5 */
  0,                            /* 6 */
  0,                            /* 7 */
  0,                            /* 8 */
  0,                            /* 9 */
  0,                            /* : */
  0,                            /* ; */
  0,                            /* < */
  0,                            /* = */
  0,                            /* > */
  0,                            /* ? */
  0,                            /* @ */
  0,                            /* A */
  0,                            /* B */
  0,                            /* C */
  UMODE_D,                      /* D */
  0,                            /* E */
  0,                            /* F */
  0,                            /* G */
  0,                            /* H */
  0,                            /* I */
  0,                            /* J */
  0,                            /* K */
  0,                            /* L */
  0,                            /* M */
  0,                            /* N */
  UMODE_O,                      /* O */
  UMODE_P,                      /* P */
  0,                            /* Q */
  UMODE_R,                      /* R */
  UMODE_S,                      /* S */
  0,                            /* T */
  0,                            /* U */
  0,                            /* V */
  UMODE_W,                      /* W */
  0,                            /* X */
  0,                            /* Y */
  UMODE_Z,                      /* Z */
  0,                            /* [ */
  0,                            /* \ */
  0,                            /* ] */
  0,                            /* ^ */
  0,                            /* _ */
  0,                            /* ` */
  UMODE_a,                      /* a */
  0,                            /* b */
  0,                            /* c */
  UMODE_d,                      /* d - Deaf client (SVSMODE will NOT allow this mode to be set as its used for Services Stamps) */
  0,                            /* e */
  0,                            /* f */
  0,                            /* g */
  UMODE_h,                      /* h */
  UMODE_i,                      /* i */
  0,                            /* j */
  0,                            /* k */
  0,                            /* l */
  0,                            /* m */
  0,                            /* n */
  UMODE_o,                      /* o */
  UMODE_p,                      /* p */
  0,                            /* q */
  UMODE_r,                      /* r */
  0,                            /* s - Reserved for client compactibility */
  0,                            /* t */
  0,                            /* u */
  0,                            /* v */
  0,                            /* w - Reserved for client compactibility */
  UMODE_x,                      /* x */
  0,                            /* y */
  0,                            /* z */
  0,                            /* { */
  0,                            /* | */
  0,                            /* } */
  0,                            /* ~ */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 127 - 141 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 142 - 156 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 157 - 171 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 172 - 186 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 187 - 201 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 202 - 216 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 217 - 231 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 232 - 246 */
  0, 0, 0, 0, 0, 0, 0, 0, 0     /* 247 - 255 */
};


int server_modes[] = {
  SMODE_a, 'a',                 /* Server Co Admin */
  SMODE_n, 'n',                 /* Network Co Admin */
  SMODE_s, 's',                 /* SSL Client */
  SMODE_t, 't',                 /* Tech Co Admin */

  SMODE_A, 'A',                 /* Server Admin */
  SMODE_G, 'G',                 /* Guest Admin */
  SMODE_N, 'N',                 /* Network Admin */
  SMODE_T, 'T',                 /* Tech Admin */
  0, 0
};

int server_mode_table[] = {
  /* 0 - 32 are control chars and space */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,                            /* ! */
  0,                            /* " */
  0,                            /* # */
  0,                            /* $ */
  0,                            /* % */
  0,                            /* & */
  0,                            /* ' */
  0,                            /* ( */
  0,                            /* ) */
  0,                            /* * */
  0,                            /* + */
  0,                            /* , */
  0,                            /* - */
  0,                            /* . */
  0,                            /* / */
  0,                            /* 0 */
  0,                            /* 1 */
  0,                            /* 2 */
  0,                            /* 3 */
  0,                            /* 4 */
  0,                            /* 5 */
  0,                            /* 6 */
  0,                            /* 7 */
  0,                            /* 8 */
  0,                            /* 9 */
  0,                            /* : */
  0,                            /* ; */
  0,                            /* < */
  0,                            /* = */
  0,                            /* > */
  0,                            /* ? */
  0,                            /* @ */
  SMODE_A,                      /* A */
  0,                            /* B */
  0,                            /* C */
  0,                            /* D */
  0,                            /* E */
  0,                            /* F */
  SMODE_G,                      /* G */
  0,                            /* H */
  0,                            /* I */
  0,                            /* J */
  0,                            /* K */
  0,                            /* L */
  0,                            /* M */
  SMODE_N,                      /* N */
  0,                            /* O */
  0,                            /* P */
  0,                            /* Q */
  0,                            /* R */
  0,                            /* S */
  SMODE_T,                      /* T */
  0,                            /* U */
  0,                            /* V */
  0,                            /* W */
  0,                            /* X */
  0,                            /* Y */
  0,                            /* Z */
  0,                            /* [ */
  0,                            /* \ */
  0,                            /* ] */
  0,                            /* ^ */
  0,                            /* _ */
  0,                            /* ` */
  SMODE_a,                      /* a */
  0,                            /* b */
  0,                            /* c */
  0,                            /* d */
  0,                            /* e */
  0,                            /* f */
  0,                            /* g */
  0,                            /* h */
  0,                            /* i */
  0,                            /* j */
  0,                            /* k */
  0,                            /* l */
  0,                            /* m */
  SMODE_n,                      /* n */
  0,                            /* o */
  0,                            /* p */
  0,                            /* q */
  0,                            /* r */
  SMODE_s,                      /* s */
  SMODE_t,                      /* t */
  0,                            /* u */
  0,                            /* v */
  0,                            /* w */
  0,                            /* x */
  0,                            /* y */
  0,                            /* z */
  0,                            /* { */
  0,                            /* | */
  0,                            /* } */
  0,                            /* ~ */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 127 - 141 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 142 - 156 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 157 - 171 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 172 - 186 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 187 - 201 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 202 - 216 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 217 - 231 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 232 - 246 */
  0, 0, 0, 0, 0, 0, 0, 0, 0     /* 247 - 255 */
};

int notice_modes[] = {
  NMODE_b, 'b',                 /* Blocked DCC Send Notices */
  NMODE_c, 'c',                 /* Local Client Connect/Exit Notices */
  NMODE_d, 'd',                 /* Debug Notices */
  NMODE_f, 'f',                 /* Server Flood Notices */
  NMODE_g, 'g',                 /* Various Oper Chat Notices */
  NMODE_k, 'k',                 /* Server Originating Kill Notices */
  NMODE_n, 'n',                 /* Network Info Notices */
  NMODE_r, 'r',                 /* Routing Notices */
  NMODE_s, 's',                 /* Server Notices */
  NMODE_w, 'w',                 /* Wallops Notices */

  NMODE_B, 'B',                 /* Spam Bot Notices */
  NMODE_C, 'C',                 /* Global Client Connect/Exit Notices */
  NMODE_G, 'G',                 /* Globops Notices */
  NMODE_N, 'N',                 /* Network Global Notices */
  NMODE_R, 'R',                 /* Reject Notices */
  NMODE_S, 'S',                 /* "Spy" Notices */
  0, 0
};

int notice_mode_table[] = {
  /* 0 - 32 are control chars and space */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,                            /* ! */
  0,                            /* " */
  0,                            /* # */
  0,                            /* $ */
  0,                            /* % */
  0,                            /* & */
  0,                            /* ' */
  0,                            /* ( */
  0,                            /* ) */
  0,                            /* * */
  0,                            /* + */
  0,                            /* , */
  0,                            /* - */
  0,                            /* . */
  0,                            /* / */
  0,                            /* 0 */
  0,                            /* 1 */
  0,                            /* 2 */
  0,                            /* 3 */
  0,                            /* 4 */
  0,                            /* 5 */
  0,                            /* 6 */
  0,                            /* 7 */
  0,                            /* 8 */
  0,                            /* 9 */
  0,                            /* : */
  0,                            /* ; */
  0,                            /* < */
  0,                            /* = */
  0,                            /* > */
  0,                            /* ? */
  0,                            /* @ */
  0,                            /* A */
  NMODE_B,                      /* B */
  NMODE_C,                      /* C */
  0,                            /* D */
  0,                            /* E */
  0,                            /* F */
  NMODE_G,                      /* G */
  0,                            /* H */
  0,                            /* I */
  0,                            /* J */
  0,                            /* K */
  0,                            /* L */
  0,                            /* M */
  NMODE_N,                      /* N */
  0,                            /* O */
  0,                            /* P */
  0,                            /* Q */
  NMODE_R,                      /* R */
  NMODE_S,                      /* S */
  0,                            /* T */
  0,                            /* U */
  0,                            /* V */
  0,                            /* W */
  0,                            /* X */
  0,                            /* Y */
  0,                            /* Z */
  0,                            /* [ */
  0,                            /* \ */
  0,                            /* ] */
  0,                            /* ^ */
  0,                            /* _ */
  0,                            /* ` */
  0,                            /* a */
  NMODE_b,                      /* b */
  NMODE_c,                      /* c */
  NMODE_d,                      /* d */
  0,                            /* e */
  NMODE_f,                      /* f */
  NMODE_g,                      /* g */
  0,                            /* h */
  0,                            /* i */
  0,                            /* j */
  NMODE_k,                      /* k */
  0,                            /* l */
  0,                            /* m */
  NMODE_n,                      /* n */
  0,                            /* o */
  0,                            /* p */
  0,                            /* q */
  NMODE_r,                      /* r */
  NMODE_s,                      /* s */
  0,                            /* t */
  0,                            /* u */
  0,                            /* v */
  NMODE_w,                      /* w */
  0,                            /* x */
  0,                            /* y */
  0,                            /* z */
  0,                            /* { */
  0,                            /* | */
  0,                            /* } */
  0,                            /* ~ */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 127 - 141 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 142 - 156 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 157 - 171 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 172 - 186 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 187 - 201 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 202 - 216 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 217 - 231 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 232 - 246 */
  0, 0, 0, 0, 0, 0, 0, 0, 0     /* 247 - 255 */
};


/*
 * internally defined functions
 */
int botreject (char *);
unsigned long my_rand (void);   /*

                                 * provided by orabidoo 
                                 */
/*
 * externally defined functions 
 */
extern int find_fline (aClient *);      /*

                                         * defined in s_conf.c
                                         */
extern Link *find_channel_link (Link *, aChannel *);    /*

                                                         * defined in list.c 
                                                         */
#ifdef FLUD
int flud_num = FLUD_NUM;
int flud_time = FLUD_TIME;
int flud_block = FLUD_BLOCK;
extern BlockHeap *free_fludbots;
extern BlockHeap *free_Links;

void announce_fluder (aClient *, aClient *, aChannel *, int);
struct fludbot *remove_fluder_reference (struct fludbot **, aClient *);
Link *remove_fludee_reference (Link **, void *);
int check_for_fludblock (aClient *, aClient *, aChannel *, int);
int check_for_flud (aClient *, aClient *, aChannel *, int);
void free_fluders (aClient *, aChannel *);
void free_fludees (aClient *);
#endif
int check_for_ctcp (char *, char **);
int allow_dcc (aClient *, aClient *);
static int is_silenced (aClient *, aClient *);

#ifdef ANTI_SPAMBOT
int spam_time = MIN_JOIN_LEAVE_TIME;
int spam_num = MAX_JOIN_LEAVE_COUNT;
#endif

/* defines for check_ctcp results */
#define CTCP_NONE 	0
#define CTCP_YES	1
#define CTCP_DCC	2
#define CTCP_DCCSEND 	3


/*
 * * m_functions execute protocol messages on this server: *
 *
 *      client_p    is always NON-NULL, pointing to a *LOCAL* client *
 * tructure (with an open socket connected!). This *
 * es the physical socket where the message *           originated (or
 * which caused the m_function to be *          executed--some
 * m_functions may call others...). *
 *
 *      source_p    is the source of the message, defined by the *
 * refix part of the message if present. If not *               or
 * prefix not found, then source_p==client_p. *
 *
 *              (!IsServer(client_p)) => (client_p == source_p), because *
 * refixes are taken *only* from servers... *
 * 
 *              (IsServer(client_p)) *                      (source_p == client_p)
 * => the message didn't *                      have the prefix. *
 *
 *                      (source_p != client_p && IsServer(source_p) means *
 * he prefix specified servername. (?) *
 * 
 *                      (source_p != client_p && !IsServer(source_p) means *
 * hat message originated from a remote *                       user
 * (not local). *
 * 
 *              combining *
 * 
 *              (!IsServer(source_p)) means that, source_p can safely *
 * aken as defining the target structure of the *               message
 * in this server. *
 *
 *      *Always* true (if 'parse' and others are working correct): *
 *
 *      1)      source_p->from == client_p  (note: client_p->from == client_p) *
 * 
 *      2)      MyConnect(source_p) <=> source_p == client_p (e.g. source_p *
 * annot* be a local connection, unless it's *          actually
 * client_p!). [MyConnect(x) should probably *              be defined as
 * (x == x->from) --msa ] *
 *
 *      parc    number of variable parameter strings (if zero, *
 * arv is allowed to be NULL) *
 * 
 *      parv    a NULL terminated list of parameter pointers, *
 *
 *                      parv[0], sender (prefix string), if not present *
 * his points to an empty string. *
 * arc-1] *                             pointers to additional
 * parameters *                 parv[parc] == NULL, *always* *
 *
 *              note:   it is guaranteed that parv[0]..parv[parc-1] are
 * all *                        non-NULL pointers.
 */
/*
 * * next_client *    Local function to find the next matching
 * client. The search * can be continued from the specified client
 * entry. Normal *      usage loop is: *
 *
 *      for (x = client; x = next_client(x,mask); x = x->next) *
 * andleMatchingClient; *
 * 
 */
aClient *
next_client (aClient * next,    /*
                                 * First client to check
                                 */
             char *ch)
{                               /*
                                 * search string (may include wilds) 
                                 */
  aClient *tmp = next;

  next = find_client (ch, tmp);
  if (tmp && tmp->prev == next)
    return ((aClient *) NULL);

  if (next != tmp)
    return next;
  for (; next; next = next->next)
  {
    if (!match (ch, next->name))
      break;
  }
  return next;
}

/*
 * this slow version needs to be used for hostmasks *sigh * 
 */

aClient *
next_client_double (aClient * next,     /*
                                         * First client to check 
                                         */
                    char *ch)
{                               /*
                                 * search string (may include wilds) 
                                 */
  aClient *tmp = next;

  next = find_client (ch, tmp);
  if (tmp && tmp->prev == next)
    return NULL;
  if (next != tmp)
    return next;
  for (; next; next = next->next)
  {
    if (!match (ch, next->name) || !match (next->name, ch))
      break;
  }
  return next;
}

/*
 * * hunt_server *
 *
 *      Do the basic thing in delivering the message (command) *
 * across the relays to the specific server (server) for *
 * actions. *
 * 
 *      Note:   The command is a format string and *MUST* be *
 * f prefixed style (e.g. ":%s COMMAND %s ..."). *              Command
 * can have only max 8 parameters. *
 *
 *      server  parv[server] is the parameter identifying the *
 * arget server. *
 * 
 *      *WARNING* *             parv[server] is replaced with the
 * pointer to the *             real servername from the matched client
 * (I'm lazy *          now --msa). *
 *
 *      returns: (see #defines)
 */
int
hunt_server (aClient * client_p,
             aClient * source_p,
             char *command, int server, int parc, char *parv[])
{
  aClient *target_p;
  int wilds;

  /*
   * * Assume it's me, if no server
   */
  if (parc <= server || BadPtr (parv[server]) ||
      match (me.name, parv[server]) == 0
      || match (parv[server], me.name) == 0)
    return (HUNTED_ISME);
  /*
   * * These are to pickup matches that would cause the following *
   * message to go in the wrong direction while doing quick fast *
   * non-matching lookups.
   */
  if ((target_p = find_client (parv[server], NULL)))
    if (target_p->from == source_p->from && !MyConnect (target_p))
      target_p = NULL;
  if (!target_p && (target_p = find_server (parv[server], NULL)))
    if (target_p->from == source_p->from && !MyConnect (target_p))
      target_p = NULL;

  collapse (parv[server]);
  wilds = (strchr (parv[server], '?') || strchr (parv[server], '*'));
  /*
   * Again, if there are no wild cards involved in the server name,
   * use the hash lookup - Dianora
   */

  if (!target_p)
  {
    if (!wilds)
    {
      target_p = find_name (parv[server], (aClient *) NULL);
      if (!target_p || !IsRegistered (target_p) || !IsServer (target_p))
      {
        sendto_one (source_p, err_str (ERR_NOSUCHSERVER), me.name,
                    parv[0], parv[server]);
        return (HUNTED_NOSUCH);
      }
    }
    else
    {
      for (target_p = client;
           (target_p = next_client (target_p, parv[server]));
           target_p = target_p->next)
      {
        if (target_p->from == source_p->from && !MyConnect (target_p))
          continue;
        /*
         * Fix to prevent looping in case the parameter for some
         * reason happens to match someone from the from link --jto
         */
        if (IsRegistered (target_p) && (target_p != client_p))
          break;
      }
    }
  }

  if (target_p)
  {
    if (IsMe (target_p) || MyClient (target_p))
      return HUNTED_ISME;
    if (match (target_p->name, parv[server]))
      parv[server] = target_p->name;
    sendto_one (target_p, command, parv[0],
                parv[1], parv[2], parv[3], parv[4],
                parv[5], parv[6], parv[7], parv[8]);
    return (HUNTED_PASS);
  }
  sendto_one (source_p, err_str (ERR_NOSUCHSERVER), me.name,
              parv[0], parv[server]);
  return (HUNTED_NOSUCH);
}

/*
 * * canonize *
 *
 * reduce a string of duplicate list entries to contain only the unique *
 * items.  Unavoidably O(n^2).
 */
char *
canonize (char *buffer)
{
  static char cbuf[BUFSIZ];
  char *s, *t, *cp = cbuf;
  int l = 0;
  char *p = NULL, *p2;

  *cp = '\0';

  for (s = strtoken (&p, buffer, ","); s; s = strtoken (&p, NULL, ","))
  {
    if (l)
    {
      for (p2 = NULL, t = strtoken (&p2, cbuf, ","); t;
           t = strtoken (&p2, NULL, ","))
        if (irccmp (s, t) == 0)
          break;
        else if (p2)
          p2[-1] = ',';
    }
    else
      t = NULL;

    if (!t)
    {
      if (l)
        *(cp - 1) = ',';
      else
        l = 1;
      strcpy (cp, s);
      if (p)
        cp += (p - s);
    }
    else if (p2)
      p2[-1] = ',';
  }
  return cbuf;
}

char umodestring[128];

void
build_umodestr (void)
{
  int *s;
  char *m;

  m = umodestring;

  for (s = user_modes; *s; s += 2)
  {
    *m++ = (char) (*(s + 1));
  }

  *m = '\0';
}

/*
 * show_isupport
 *
 * inputs	- pointer to client
 * output	-
 * side effects	- display to client what we support (for them)
 */
void
show_isupport (aClient * source_p)
{
  char isupportbuffer[512];

  ircsprintf (isupportbuffer, FEATURES, FEATURESVALUES);
  sendto_one (source_p, rpl_str (RPL_PROTOCTL), me.name, source_p->name,
              isupportbuffer);

  ircsprintf (isupportbuffer, FEATURES2, FEATURES2VALUES);
  sendto_one (source_p, rpl_str (RPL_PROTOCTL), me.name, source_p->name,
              isupportbuffer);
  return;
}


/*
 * register_user
 *
 * This function is called when both NICK and USER messages
 * have been accepted for the client, in whatever order.
 * Only after this, is the USER message propagated.
 *
 * NICK's must be propagated at once when received, although
 * it would be better to delay them too until full info is
 * available. Doing it is not so simple though, would have
 * to implement the following:
 *
 * (actually it has been implemented already for a while)
 * -orabidoo
 *
 * 1) user telnets in and gives only "NICK foobar" and waits
 * 2) another user far away logs in normally with the nick
 * "foobar" (quite legal, as this server didn't propagate it).
 * 3) now this server gets nick "foobar" from outside, but has
 * already the same defined locally. Current server would just
 * issue "KILL foobar" to clean out dups. But,this is not
 * fair. It should actually request another nick from local user
 * or kill him/her...
 */

int
register_user (aClient * client_p, aClient * source_p, char *nick,
               char *username)
{
  aClient *nsource_p;
  aConfItem *aconf = NULL, *pwaconf = NULL;
  char *parv[3];
  static char ubuf[34];
  static char sbuf[34];
#ifdef INET6
  char *virt[HOSTLEN + 1], vtmp[HOSTLEN + 1];
  char *p, *p2;
#else
  char *p;
#endif
  short oldstatus = source_p->status;
  struct userBan *ban;
  anUser *user = source_p->user;
  aMotd *smotd;
  int i, dots;
  int bad_dns;                  /*
                                 * flag a bad dns name
                                 */
#ifdef ANTI_SPAMBOT
  char spamchar = '\0';

#endif
  char tmpstr2[512];
  int fd;

  user->last = timeofday;
  parv[0] = source_p->name;
  parv[1] = parv[2] = NULL;

  /*
   * Local clients will already have source_p->hostip
   */
  if (!MyConnect (source_p))
  {
    if (IsIPV6Client (source_p))
    {
      strncpyzt (source_p->hostip, (char *) source_p->ip.S_ADDR,
                 HOSTIPLEN + 1);
    }
    else
    {
      inet_ntop (AFINET, &source_p->ip, source_p->hostip, HOSTIPLEN + 1);
    }

/* useless? - AgAiNaWaY*/
/*#ifdef INET6
      ip6_expand (source_p->hostip, HOSTIPLEN);
#endif*/
  }
  p = source_p->hostip;

  if (MyConnect (source_p))
  {
    /*
     * This is where we check for E and F lines and flag users if they match.
     * Should save us having to walk the entire E and F line list later on.
     * We have already walked each list twice if we have throttling enabled
     * but that cannot really be helped.
     */

    if (find_eline (source_p))
    {
      SetKLineExempt (source_p);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** You are exempt from K/G lines. congrats.",
                  me.name, source_p->name);
    }
    if (find_fline (source_p))
    {
      SetSuperExempt (source_p);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** You are exempt from user limits. congrats.",
                  me.name, source_p->name);
    }

    if ((i = check_client (source_p)))
    {
      /*
       * -2 is a socket error, already reported.
       */
      if (i != -2)
      {
        if (i == -4)
        {
          ircstp->is_ref++;
          return exit_client (client_p, source_p, &me,
                              "Too many connections from your hostname");
        }
        else if (i == -3)
          sendto_realops_lev (REJ_LEV, "%s for %s [%s] ",
                              "I-line is full (Server is full)",
                              get_client_host (source_p), p);
        else
          sendto_realops_lev (REJ_LEV, "%s from %s [%s]",
                              "Unauthorized client connection",
                              get_client_host (source_p), p);
        ircstp->is_ref++;
        return exit_client (client_p, source_p, &me, i == -3 ?
                            "No more connections allowed in your connection class (the server is full)"
                            : "You are not authorized to use this server");
      }
      else
        return exit_client (client_p, source_p, &me, "Socket Error");
    }

#ifdef ANTI_SPAMBOT
    /*
     * This appears to be broken 
     */
    /*
     * Check for single char in user->host -ThemBones 
     */
    if (*(user->host + 1) == '\0')
      spamchar = *user->host;
#endif
    Debug ((DEBUG_DEBUG, "host is %s", source_p->sockhost));
    strncpyzt (user->host, source_p->sockhost, HOSTLEN);

    dots = 0;
    p = user->host;
    bad_dns = NO;
    while (*p)
    {
      if (!MyIsAlnum (*p))
      {
#ifdef RFC1035_ANAL
        if ((*p != '-') && (*p != '.')
#ifdef INET6
            && (*p != ':')
#endif
          )
#else
        if ((*p != '-') && (*p != '.') && (*p != '_') && (*p != '/')
#ifdef INET6
            && (*p != ':')
#endif
          )
#endif /*
        * RFC1035_ANAL
        */
          bad_dns = YES;
      }
#ifndef INET6
      if (*p == '.')
#else
      if (*p == '.' || *p == ':')
#endif
        dots++;
      p++;
    }
    /*
     * Check that the hostname has AT LEAST ONE dot (.) in it. If
     * not, drop the client (spoofed host) -ThemBones
     */
    if (!dots)
    {
      sendto_realops ("Invalid hostname for %s, dumping user %s",
                      source_p->hostip, source_p->name);
      return exit_client (client_p, source_p, &me, "Invalid hostname");
    }

    if (bad_dns)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- You have a bad character in your hostname",
                  me.name, client_p->name);
      strcpy (user->host, source_p->hostip);
      strcpy (source_p->sockhost, source_p->hostip);
    }
#ifdef INET6
    /* time to start work =) - Againaway */
    p = user->host;
    if ((strstr (p, ".")) && (strstr (p, "::ffff:")))
    {
      p = strrchr (user->host, ':');
      strncpyzt (user->host, p, HOSTIPLEN + 1);
      p = user->host;
      p2 = strchr (p, ':');
      p = p2 + 1;
      *p2 = '\0';
      strncpyzt (user->host, p, HOSTIPLEN + 1);
      p = user->host;
      strncpyzt (source_p->sockhost, p, HOSTIPLEN + 1);

      if ((strchr (p, ':')) && (!strchr (p, '.')))
      {
        SetIPV6Client (source_p);
      }
      else
      {
        /* For now we dont allow ipv4 clients FIXME - AgAiNaWaY */
        sendto_realops
          ("Unauthorized connection from %s (ipv4 clients are not supported)",
           source_p->sockhost, source_p->name);
        return exit_client (client_p, source_p, &me,
                            "Unauthorized connection");
      }
    }
#endif
    pwaconf = source_p->confs->value.aconf;

    if (source_p->flags & FLAGS_DOID && !(source_p->flags & FLAGS_GOTID))
    {
      /*
       * because username may point to user->username
       */
      char temp[USERLEN + 1];

      strncpyzt (temp, username, USERLEN + 1);
      *user->username = '~';
      strncpy (&user->username[1], temp, USERLEN);
      user->username[USERLEN] = '\0';
#ifdef IDENTD_COMPLAIN
      /*
       * tell them to install identd -Taner
       */
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- It seems that you don't have identd installed on your host.",
                  me.name, client_p->name);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- If you wish to have your username show up without the ~ (tilde),",
                  me.name, client_p->name);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- then install identd.",
                  me.name, client_p->name);
      /*
       * end identd hack
       */
#endif
    }
#ifndef FOLLOW_IDENT_RFC
    else if (source_p->flags & FLAGS_GOTID && *source_p->username != '-')
      strncpyzt (user->username, source_p->username, USERLEN + 1);
#endif
    else if (username != user->username)        /* don't overlap */
      strncpyzt (user->username, username, USERLEN + 1);

    if (!BadPtr (pwaconf->passwd)
        && !StrEq (source_p->passwd, pwaconf->passwd))
    {
      ircstp->is_ref++;
      sendto_one (source_p, err_str (ERR_PASSWDMISMATCH), me.name, parv[0]);
      return exit_client (client_p, source_p, &me, "Bad Password");
    }


    /*
     ** Check for cloners
     */
    if (CLONE_PROTECTION == 1)
    {
      if (check_clones (client_p, source_p) == -1)
      {
        ircsprintf (buf,
                    "Cloning is not permitted on this network. Max %d connections per host",
                    CLONE_LIMIT);
        return exit_client (client_p, source_p, &me, buf);
      }
    }



    /*
     * Limit clients
     */
    /*
     * We want to be able to have servers and F-line clients connect,
     * so save room for "buffer" connections. Smaller servers may
     * want to decrease this, and it should probably be just a
     * percentage of the MAXCLIENTS... -Taner
     */
    /*
     * Except "F:" clients 
     */
    if ((Count.local >= (MAXCLIENTS - 10)) && !(find_fline (source_p)))
    {
      sendto_realops_lev (SPY_LEV, "Too many clients, rejecting %s[%s].",
                          nick, source_p->sockhost);
      ircstp->is_ref++;
      ircsprintf (buf,
                  "Sorry, server is full - Connect to %s or try later",
                  DEFSERV);
      return exit_client (client_p, source_p, &me, buf);
    }

#ifdef ANTI_SPAMBOT
    /*
     * It appears, this is catching normal clients 
     */
    /*
     * Reject single char user-given user->host's 
     */
    if (spamchar == 'x')
    {
      sendto_realops_lev (REJ_LEV,
                          "Rejecting possible Spambot: %s (Single char user-given userhost: %c)",
                          get_client_name (source_p, FALSE), spamchar);
      ircstp->is_ref++;
      return exit_client (client_p, source_p, source_p,
                          "Spambot detected, rejected.");
    }
#endif


    if (oldstatus == STAT_MASTER && MyConnect (source_p))
      m_oper (&me, source_p, 1, parv);

    /* hostile username checks begin here */

    {
      char *tmpstr;
      u_char c, cc;
      int lower, upper, special;

      lower = upper = special = cc = 0;

      /*
       * check for "@" in identd reply -Taner 
       */
      if ((strchr (user->username, '@') != NULL)
          || (strchr (username, '@') != NULL))
      {
        sendto_realops_lev (REJ_LEV,
                            "Illegal \"@\" in username: %s (%s)",
                            get_client_name (source_p, FALSE), username);
        ircstp->is_ref++;

        ircsprintf (tmpstr2,
                    "Invalid username [%s] - '@' is not allowed!", username);
        return exit_client (client_p, source_p, source_p, tmpstr2);
      }
      /*
       * First check user->username...
       */
#ifdef IGNORE_FIRST_CHAR
      tmpstr = (user->username[0] == '~' ? &user->username[2] :
                &user->username[1]);
      /*
       * Ok, we don't want to TOTALLY ignore the first character. We
       * should at least check it for control characters, etc -
       * ThemBones
       */
      cc = (user->username[0] == '~' ? user->username[1] : user->username[0]);
      if ((!MyIsAlnum (cc) && !strchr (" -_.", cc)) || (cc > 127))
        special++;
#else
      tmpstr = (user->username[0] == '~' ? &user->username[1] :
                user->username);
#endif /* IGNORE_FIRST_CHAR */

      while (*tmpstr)
      {
        c = *(tmpstr++);
        if (MyIsLower (c))
        {
          lower++;
          continue;
        }
        if (MyIsUpper (c))
        {
          upper++;
          continue;
        }
        if ((!MyIsAlnum (c) && !strchr (" -_.", c)) || (c > 127) || (c < 32))
          special++;
      }
#ifdef NO_MIXED_CASE
      if (lower && upper)
      {
        sendto_realops_lev (REJ_LEV, "Invalid username: %s (%s@%s)",
                            nick, user->username, user->host);
        ircstp->is_ref++;
        ircsprintf (tmpstr2, "Invalid username [%s]", user->username);
        return exit_client (client_p, source_p, &me, tmpstr2);
      }
#endif /* NO_MIXED_CASE */
      if (special)
      {
        sendto_realops_lev (REJ_LEV, "Invalid username: %s (%s@%s)",
                            nick, user->username, user->host);
        ircstp->is_ref++;
        ircsprintf (tmpstr2, "Invalid username [%s]", user->username);
        return exit_client (client_p, source_p, &me, tmpstr2);
      }
      /* Ok, now check the username they provided, if different */
      lower = upper = special = cc = 0;

      if (strcmp (user->username, username))
      {

#ifdef IGNORE_FIRST_CHAR
        tmpstr = (username[0] == '~' ? &username[2] : &username[1]);
        /*
         * Ok, we don't want to TOTALLY ignore the first character.
         * We should at least check it for control charcters, etc
         * -ThemBones
         */
        cc = (username[0] == '~' ? username[1] : username[0]);

        if ((!MyIsAlnum (cc) && !strchr (" -_.", cc)) || (cc > 127))
          special++;
#else
        tmpstr = (username[0] == '~' ? &username[1] : username);
#endif /* IGNORE_FIRST_CHAR */
        while (*tmpstr)
        {
          c = *(tmpstr++);
          if (MyIsLower (c))
          {
            lower++;
            continue;
          }
          if (MyIsUpper (c))
          {
            upper++;
            continue;
          }
          if ((!MyIsAlnum (c) && !strchr (" -_.", c)) || (c > 127))
            special++;
        }
#ifdef NO_MIXED_CASE
        if (lower && upper)
        {
          sendto_realops_lev (REJ_LEV, "Invalid username: %s (%s@%s)",
                              nick, username, user->host);
          ircstp->is_ref++;
          ircsprintf (tmpstr2, "Invalid username [%s]", username);
          return exit_client (client_p, source_p, &me, tmpstr2);
        }
#endif /* NO_MIXED_CASE */
        if (special)
        {
          sendto_realops_lev (REJ_LEV, "Invalid username: %s (%s@%s)",
                              nick, username, user->host);
          ircstp->is_ref++;
          ircsprintf (tmpstr2, "Invalid username [%s]", username);
          return exit_client (client_p, source_p, &me, tmpstr2);
        }
      }                         /* usernames different  */
    }

    /*
     * reject single character usernames which aren't alphabetic i.e.
     * reject jokers who have '?@somehost' or '.@somehost'
     * 
     * -Dianora
     */

    if ((user->username[1] == '\0') && !MyIsAlpha (user->username[0]))
    {
      sendto_realops_lev (REJ_LEV, "Invalid username: %s (%s@%s)",
                          nick, user->username, user->host);
      ircstp->is_ref++;
      ircsprintf (tmpstr2, "Invalid username [%s]", user->username);
      return exit_client (client_p, source_p, &me, tmpstr2);
    }

    if (!IsExempt (source_p))
    {
      /*
       * Check for gcos bans
       */
      if ((aconf = find_conf_name (source_p->info, CONF_GCOS)))
      {

        return exit_client (client_p, source_p, source_p,
                            BadPtr (aconf->
                                    passwd) ?
                            "Bad GCOS: Reason unspecified" : aconf->passwd);
      }

      if (!
          (ban =
           check_userbanned (source_p, UBAN_IP | UBAN_CIDR4, UBAN_WILDUSER)))
      {
        ban = check_userbanned (source_p, UBAN_HOST, 0);
      }
      if (ban)
      {
        char *reason, *ktype;
        int mylocal;

        mylocal = (ban->flags & UBAN_LOCAL) ? 1 : 0;
        ktype = mylocal ? LOCAL_BANNED_NAME : NETWORK_BANNED_NAME;
        reason = ban->reason ? ban->reason : ktype;

        sendto_one (source_p, err_str (ERR_YOUREBANNEDCREEP), me.name,
                    source_p->name, ktype);
        sendto_one (source_p,
                    ":%s NOTICE %s :*** You are not welcome on this %s.",
                    me.name, source_p->name, mylocal ? "server" : "network");
        sendto_one (source_p, ":%s NOTICE %s :*** %s for %s", me.name,
                    source_p->name, ktype, reason);
        sendto_one (source_p,
                    ":%s NOTICE %s :*** Your hostmask is %s!%s@%s",
                    me.name, source_p->name, source_p->name,
                    source_p->user->username, source_p->sockhost);
        sendto_one (source_p, ":%s NOTICE %s :*** Your IP is %s",
                    me.name, source_p->name, source_p->hostip);
        sendto_one (source_p,
                    ":%s NOTICE %s :*** For assistance, please email %s and "
                    "include everything shown here.", me.name,
                    source_p->name,
                    mylocal ? SERVER_KLINE_ADDRESS : NETWORK_KLINE_ADDRESS);

#ifdef USE_REJECT_HOLD
        client_p->flags |= FLAGS_REJECT_HOLD;
#endif
        ircstp->is_ref++;
        ircstp->is_ref_2++;

#ifndef USE_REJECT_HOLD
#ifdef THROTTLE_ENABLE
        throttle_force (source_p->hostip);
#endif
        return exit_client (client_p, source_p, &me, reason);
#endif
      }

      if (is_a_drone (source_p))
      {
#ifdef THROTTLE_ENABLE
        throttle_force (source_p->hostip);
#endif
        ircstp->is_ref++;
        ircstp->is_drone++;
        return exit_client (client_p, source_p, &me,
                            "You match the pattern of a known trojan, please check your system. Try altering your realname and/or ident to lowercase characters as this can trigger false positives.");
      }

    }

    /*
     * We also send global connect notices if this is enabeled in the networks settings file - ShadowMaster
     */

    sendto_connectnotice
      ("from %s: Client connecting on port %d: %s (%s@%s) [%s] {%d}%s [%s]",
       me.name, source_p->acpt->port, nick, user->username, user->host,
       source_p->hostip, get_client_class (source_p),
       IsSSL (source_p) ? " (SSL)" : "", source_p->info);

    ilog (LOGF_CLIENTS,
          "Client connecting on port %d: %s (%s@%s) [%s] {%d}%s [%s]",
          source_p->acpt->port, nick, user->username, user->host,
          source_p->hostip, get_client_class (source_p),
          IsSSL (source_p) ? " (SSL)" : "", source_p->info);

    if (GLOBAL_CONNECTS == 1)
    {
      sendto_serv_butone (client_p,
                          ":%s GCONNECT :Client connecting: %s (%s@%s) [%s]%s [%s]",
                          me.name, nick, user->username,
                          user->host, source_p->hostip,
                          IsSSL (source_p) ? " (SSL)" : "", source_p->info);
    }

    if ((++Count.local) > Count.max_loc)
    {
      Count.max_loc = Count.local;
      if (!(Count.max_loc % 10))
        sendto_ops ("New Max Local Clients: %d", Count.max_loc);
    }
    if ((NOW - Count.day) > 86400)
    {
      Count.today = 0;
      Count.day = NOW;
    }
    if ((NOW - Count.week) > 604800)
    {
      Count.weekly = 0;
      Count.week = NOW;
    }
    if ((NOW - Count.month) > 2592000)
    {
      Count.monthly = 0;
      Count.month = NOW;
    }
    if ((NOW - Count.year) > 31536000)
    {
      Count.yearly = 0;
      Count.year = NOW;
    }
    Count.today++;
    Count.weekly++;
    Count.monthly++;
    Count.yearly++;

    if (source_p->flags & FLAGS_BAD_DNS)
      sendto_realops_lev (DEBUG_LEV,
                          "DNS lookup: %s (%s@%s) is an attempted cache polluter",
                          source_p->name, source_p->user->username,
                          source_p->user->host);

  }
  else
    strncpyzt (user->username, username, USERLEN + 1);
  SetClient (source_p);
  /*
   * Increment our total user count here
   * But do not count ULined clients.
   */

  if (!IsULine (source_p))
  {
    Count.total++;

    if (Count.total > Count.max_tot)
    {
      Count.max_tot = Count.total;
    }

    if (IsInvisible (source_p))
    {
      Count.invisi++;
    }
  }

  if (MyConnect (source_p))
  {
    source_p->pingval = get_client_ping (source_p);
    source_p->sendqlen = get_sendq (source_p);
#ifdef MAXBUFFERS
    /*
     * Let's try changing the socket options for the client here...
     * -Taner
     */
    reset_sock_opts (source_p->fd, 0);
    /*
     * End sock_opt hack
     */
#endif
    sendto_one (source_p, rpl_str (RPL_WELCOME), me.name, nick,
                IRCNETWORK, nick, source_p->user->username,
                source_p->user->host);
    /*
     * This is a duplicate of the NOTICE but see below...
     * um, why were we hiding it? they did make it on to the
     * server and all..;) -wd
     */
    sendto_one (source_p, rpl_str (RPL_YOURHOST), me.name, nick,
                get_client_name (&me, TRUE), version);
#ifdef	IRCII_KLUDGE
    /*
     * * Don't mess with this one - IRCII needs it! -Avalon
     */
    sendto_one (source_p,
                "NOTICE %s :*** Your host is %s, running %s",
                nick, get_client_name (&me, TRUE), version);
#endif
    sendto_one (source_p, rpl_str (RPL_CREATED), me.name, nick, creation,
                GEO_LOCATION);

    sendto_one (source_p, rpl_str (RPL_MYINFO), me.name, parv[0], me.name,
                version, umodestring, cmodestring);

    show_isupport (source_p);

    send_lusers (source_p, source_p, 1, parv);

    fd = open (MOTD, O_RDONLY);
    if (!(fd == -1))
    {

      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- motd was last changed at %s",
                  me.name, nick, motd_last_changed_date);
    }
    close (fd);

    if (SHORT_MOTD == 1)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- Please read the motd if you haven't read it",
                  me.name, nick);

      sendto_one (source_p, rpl_str (RPL_MOTDSTART), me.name, parv[0],
                  me.name);
      if ((smotd = shortmotd) == NULL)
      {
        sendto_one (source_p,
                    rpl_str (RPL_MOTD),
                    me.name, parv[0], "*** This is the short motd ***");
      }
      else
      {
        while (smotd)
        {
          sendto_one (source_p, rpl_str (RPL_MOTD), me.name, parv[0],
                      smotd->line);
          smotd = smotd->next;
        }
      }

      sendto_one (source_p, rpl_str (RPL_ENDOFMOTD), me.name, parv[0]);

    }
    else
    {
      send_motd (source_p, source_p, 1, parv);
    }

    if (WINGATE_NOTICE == 1)
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- Upon connection we will conduct as security scan on your host",
                  me.name, nick);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- in search for insecure proxy servers.",
                  me.name, nick);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- Please disregard any connections originating from %s",
                  me.name, nick, MONITOR_HOST);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- For more information please see %s",
                  me.name, nick, MONITOR_URL);
    }

#ifdef LITTLE_I_LINES
    if (source_p->confs && source_p->confs->value.aconf &&
        (source_p->confs->value.aconf->flags & CONF_FLAGS_LITTLE_I_LINE))
    {
      SetRestricted (source_p);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- You are in a restricted access mode",
                  me.name, nick);
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- You can not be chanopped",
                  me.name, nick);
    }
#endif

  }
  else if (IsServer (client_p))
  {
    aClient *target_p;

    if ((target_p = find_server (user->server, NULL)) &&
        target_p->from != source_p->from)
    {
      sendto_realops_lev (DEBUG_LEV,
                          "Bad User [%s] :%s USER %s@%s %s, != %s[%s]",
                          client_p->name, nick, user->username,
                          user->host, user->server,
                          target_p->name, target_p->from->name);
      sendto_one (client_p,
                  ":%s KILL %s :%s (%s != %s[%s] USER from wrong direction)",
                  me.name, source_p->name, me.name, user->server,
                  target_p->from->name, target_p->from->sockhost);
      source_p->flags |= FLAGS_KILLED;
      return exit_client (source_p, source_p, &me,
                          "USER server wrong direction");

    }
    /*
     * Super GhostDetect: If we can't find the server the user is
     * supposed to be on, then simply blow the user away.     -Taner
     */
    if (!target_p)
    {
      sendto_one (client_p,
                  ":%s KILL %s :%s GHOST (no server %s on the net)",
                  me.name, source_p->name, me.name, user->server);
      sendto_realops ("No server %s for user %s[%s@%s] from %s",
                      user->server,
                      source_p->name, user->username,
                      user->host, source_p->from->name);
      source_p->flags |= FLAGS_KILLED;
      return exit_client (source_p, source_p, &me, "Ghosted Client");
    }
  }

  /*
   * If this is a remote client from a NON-Client capable server we wont have recieved the true hiddenhost for this user yet.
   * However we will have recieved his usermodes so we know he is hidden or not.
   * In order to correctly pass data along to other Client capable servers we need to create a hiddenhost for the user.
   * The correct hiddenhost will be sent after the CLIENT in a SETHOST. This is mostly a cludge for backwards compactibility
   * with older Ultimate3 alphas.
   */
  if (IsHidden (source_p))
  {
    /* Make sure the client has a valid hiddenhost, if not, generate it */
    if (strlen (user->virthost) <= 2)
    {
#ifdef INET6

      strncpy (vtmp, source_p->hostip, HOSTLEN);
      if (!((str2arr (virt, vtmp, ".")) == 4))
      {
        make_ipv6virthost (source_p->hostip, user->host, user->virthost);
      }
      else
      {
#endif
        make_virthost (source_p->hostip, user->host, user->virthost);
#ifdef INET6
      }
#endif
    }
  }

  send_umode (NULL, source_p, 0, SEND_UMODES, ubuf);
  if (!*ubuf)
  {
    ubuf[0] = '+';
    ubuf[1] = '\0';
  }

  send_smode (NULL, source_p, 0, SEND_SMODES, sbuf);
  if (!*sbuf)
  {
    sbuf[0] = '+';
    sbuf[1] = '\0';
  }
  hash_check_watch (source_p, RPL_LOGON);

#ifndef INET6
  if (IsIPV6Client (source_p))
  {
    sendto_clientcapab_servs_butone (1, client_p,
                                     "CLIENT %s %d %ld %s %s %s %s %s %s %lu %s :%s",
                                     nick, source_p->hopcount + 1,
                                     source_p->tsinfo, ubuf, sbuf,
                                     user->username, user->host,
                                     (IsHidden (source_p) ? source_p->user->
                                      virthost : "*"), user->server,
                                     source_p->user->servicestamp,
                                     source_p->ip.S_ADDR, source_p->info);

    sendto_clientcapab_servs_butone (0, client_p,
                                     "NICK %s %d %ld %s %s %s %s %lu %s :%s",
                                     nick, source_p->hopcount + 1,
                                     source_p->tsinfo, ubuf, user->username,
                                     user->host, user->server,
                                     source_p->user->servicestamp,
                                     source_p->ip.S_ADDR, source_p->info);
  }
  else
  {
    sendto_clientcapab_servs_butone (1, client_p,
                                     "CLIENT %s %d %ld %s %s %s %s %s %s %lu %lu :%s",
                                     nick, source_p->hopcount + 1,
                                     source_p->tsinfo, ubuf, sbuf,
                                     user->username, user->host,
                                     (IsHidden (source_p) ? source_p->user->
                                      virthost : "*"), user->server,
                                     source_p->user->servicestamp,
                                     htonl (source_p->ip.S_ADDR),
                                     source_p->info);

    sendto_clientcapab_servs_butone (0, client_p,
                                     "NICK %s %d %ld %s %s %s %s %lu %lu :%s",
                                     nick, source_p->hopcount + 1,
                                     source_p->tsinfo, ubuf, user->username,
                                     user->host, user->server,
                                     source_p->user->servicestamp,
                                     htonl (source_p->ip.S_ADDR),
                                     source_p->info);
  }
#else

  sendto_clientcapab_servs_butone (1, client_p,
                                   "CLIENT %s %d %ld %s %s %s %s %s %s %lu %s :%s",
                                   nick, source_p->hopcount + 1,
                                   source_p->tsinfo, ubuf, sbuf,
                                   user->username, user->host,
                                   (IsHidden (source_p) ? source_p->user->
                                    virthost : "*"), user->server,
                                   source_p->user->servicestamp,
                                   source_p->sockhost, source_p->info);

  sendto_clientcapab_servs_butone (0, client_p,
                                   "NICK %s %d %ld %s %s %s %s %lu %s :%s",
                                   nick, source_p->hopcount + 1,
                                   source_p->tsinfo, ubuf, user->username,
                                   user->host, user->server,
                                   source_p->user->servicestamp,
                                   source_p->sockhost, source_p->info);
#endif

  /*
   * If the client has a hiddenhost, tell the other servers about it. But _NOT_ CLIENT capab servers
   * as they will already have recieved this.
   * This is a very ugly cludge to ensure that the hiddenhost is sent out.
   * This will send out double messages and is only a temporary cludge till a24.
   */
  if (IsHidden (source_p))
  {
    sendto_clientcapab_servs_butone (0, client_p, ":%s SETHOST %s %s",
                                     me.name, nick, source_p->user->virthost);
  }

  if (MyClient (source_p))
  {
    /* if the I:line doesn't have a password and the user does, send it over to NickServ */
    if (BadPtr (pwaconf->passwd) && source_p->passwd[0]
        && (nsource_p = find_person (NICKSERV_NAME, NULL)) != NULL)
    {
      sendto_one (nsource_p, ":%s PRIVMSG %s@%s :SIDENTIFY %s",
                  source_p->name, NICKSERV_NAME, SERVICES_SERVER,
                  source_p->passwd);
    }

    if (source_p->passwd[0])
      memset (source_p->passwd, '\0', PASSWDLEN);

    if (ubuf[1])
      send_umode (client_p, source_p, 0, ALL_UMODES, ubuf);

    if (sbuf[1])
      send_smode (client_p, source_p, 0, SEND_SMODES, sbuf);

    /*
     * Ugly hack
     */
    if (irccmp (USERS_AUTO_JOIN, "0") != 0)
    {
      strncpyzt (tmpstr2, USERS_AUTO_JOIN, 512);
    }
    if (irccmp (USERS_AUTO_JOIN, "0") != 0)
    {
      char *chans[3] = {
        source_p->name, tmpstr2, NULL
      };


      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- Network has Auto Join enabled for Channel(s): \2%s\2",
                  me.name, source_p->name, USERS_AUTO_JOIN);

      m_join (source_p, source_p, 3, chans);

    }

  }

  return 0;
}

/*
 * Code provided by orabidoo
 */
/*
 * a random number generator loosely based on RC5; assumes ints are at
 * least 32 bit
 */

unsigned long
my_rand ()
{
  static unsigned long s = 0, t = 0, k = 12345678;
  int i;

  if (s == 0 && t == 0)
  {
    s = (unsigned long) getpid ();
    t = (unsigned long) time (NULL);
  }
  for (i = 0; i < 12; i++)
  {
    s = (((s ^ t) << (t & 31)) | ((s ^ t) >> (31 - (t & 31)))) + k;
    k += s + t;
    t = (((t ^ s) << (s & 31)) | ((t ^ s) >> (31 - (s & 31)))) + k;
    k += s + t;
  }
  return s;
}

char *exploits_2char[] = {
  "js", "pl", NULL
};
char *exploits_3char[] = {
  "exe", "com", "bat", "dll", "ini", "vbs", "pif",
  "mrc", "scr", "doc", "xls", "lnk", "shs", "htm", NULL
};
char *exploits_4char[] = {
  "html", NULL
};

int
check_dccsend (aClient * from, aClient * to, char *msg)
{
  /* we already know that msg will consist of "DCC SEND" so we can skip to the end */
  char *filename = msg + 8;
  char *ext;
  char **farray = NULL;
  int arraysz;
  int len = 0, extlen = 0, i;

  /* people can send themselves stuff all the like..
   * opers need to be able to send cleaner files
   * sanity checks..
   */

  if (from == to || !IsPerson (from) || IsAnOper (from) || !MyClient (to))
    return 0;

  while (*filename == ' ')
    filename++;

  if (!(*filename))
    return 0;

  while (*(filename + len) != ' ')
  {
    if (!(*(filename + len)))
      break;
    len++;
  }

  for (ext = filename + len;; ext--)
  {
    if (ext == filename)
      return 0;

    if (*ext == '.')
    {
      ext++;
      extlen--;
      break;
    }
    extlen++;
  }

  switch (extlen)
  {
     case 0:
       arraysz = 0;
       break;
     case 2:
       farray = exploits_2char;
       arraysz = 2;
       break;

     case 3:
       farray = exploits_3char;
       arraysz = 3;
       break;

     case 4:
       farray = exploits_4char;
       arraysz = 4;
       break;


       /* no executable file here.. */
     default:
       return 0;
  }

  if (arraysz != 0)
  {
    for (i = 0; farray[i]; i++)
    {
      if (ircncmp (farray[i], ext, arraysz) == 0)
        break;
    }

    if (farray[i] == NULL)
      return 0;
  }
  if (!allow_dcc (to, from))
  {
    char tmpext[8];
    char tmpfn[128];
    Link *tlp, *flp;
    aChannel *channel_p = NULL;

    strncpy (tmpext, ext, extlen);
    tmpext[extlen] = '\0';

    if (len > 127)
      len = 127;
    strncpy (tmpfn, filename, len);
    tmpfn[len] = '\0';

    /* use notices! 
     *   server notices are hard to script around.
     *   server notices are not ignored by clients.
     */

    sendto_one (from,
                ":%s NOTICE %s :The user %s is not accepting DCC sends of filetype *.%s from you."
                " Your file %s was not sent.", me.name, from->name,
                to->name, tmpext, tmpfn);

    sendto_one (to,
                ":%s NOTICE %s :%s (%s@%s) has attempted to send you a file named %s, which was blocked.",
                me.name, to->name, from->name, from->user->username,
                from->user->host, tmpfn);

    if (!SendSeenDCCNotice (to))
    {
      SetDCCNotice (to);

      sendto_one (to,
                  ":%s NOTICE %s :The majority of files sent of this type are malicious virii and trojan horses."
                  " In order to prevent the spread of this problem, we are blocking DCC sends of these types of"
                  " files by default.", me.name, to->name);
      sendto_one (to,
                  ":%s NOTICE %s :If you trust %s, and want him/her to send you this file, you may obtain"
                  " more information on using the dccallow system by typing /dccallow help",
                  me.name, to->name, from->name, to->name);
    }

    for (tlp = to->user->channel; tlp && !channel_p; tlp = tlp->next)
    {
      for (flp = from->user->channel; flp && !channel_p; flp = flp->next)
      {
        if (tlp->value.channel_p == flp->value.channel_p)
          channel_p = tlp->value.channel_p;
      }
    }

    if (channel_p)
      sendto_realops_lev (DCCSEND_LEV,
                          "%s (%s@%s) sending forbidden filetyped file %s to %s (channel %s)",
                          from->name, from->user->username,
                          from->user->host, tmpfn, to->name,
                          channel_p->chname);
    else
      sendto_realops_lev (DCCSEND_LEV,
                          "%s (%s@%s) sending forbidden filetyped file %s to %s",
                          from->name, from->user->username,
                          from->user->host, tmpfn, to->name);

    return 1;
  }

  return 0;
}

/* check to see if the message has any color chars in it. */
int
msg_has_colors (char *msg)
{
  char *c;
  if (msg == NULL)
    return 0;

  c = msg;

  while (*c)
  {
    if (*c == '\003' || *c == '\033')
      break;
    else
      c++;
  }

  if (*c)
    return 1;

  return 0;
}


/*
  * check target limit: message target rate limiting
  * anti spam control!
  * should only be called for local PERSONS!
  * source_p: client sending message
  * target_p: client receiving message
  *
  * return value:
  * 1: block
  * 0: do nothing
  */

#ifdef MSG_TARGET_LIMIT
int
check_target_limit (aClient * source_p, aClient * target_p)
{
  int ti;
  int max_targets;
  time_t tmin = MSG_TARGET_TIME;        /* minimum time to wait before another message can be sent */

  /* don't limit opers, people talking to themselves, or people talking to services */
  if (IsAnOper (source_p) || source_p == target_p || IsULine (target_p))
    return 0;

  max_targets =
    ((NOW - source_p->firsttime) >
     MSG_TARGET_MINTOMAXTIME) ? MSG_TARGET_MAX : MSG_TARGET_MIN;

  for (ti = 0; ti < max_targets; ti++)
  {
    if (source_p->targets[ti].cli == NULL ||    /* no client */
        source_p->targets[ti].cli == target_p ||        /* already have this client */
        source_p->targets[ti].sent < (NOW - MSG_TARGET_TIME)    /* haven't talked to this client in > MSG_TARGET_TIME secs */
      )
    {
      source_p->targets[ti].cli = target_p;
      source_p->targets[ti].sent = NOW;
      break;
    }
    else if ((NOW - source_p->targets[ti].sent) < tmin)
      tmin = NOW - source_p->targets[ti].sent;
  }

  if (ti == max_targets)
  {
    sendto_one (source_p, err_str (ERR_TARGETTOFAST), me.name,
                source_p->name, target_p->name, MSG_TARGET_TIME - tmin);
    source_p->since += 2;       /* penalize them 2 seconds for this! */
    source_p->num_target_errors++;

    if (source_p->last_target_complain + 60 <= NOW)
    {
      sendto_realops_lev (SPAM_LEV,
                          "Target limited: %s (%s@%s) [%d failed targets]",
                          source_p->name, source_p->user->username,
                          source_p->user->host, source_p->num_target_errors);
      source_p->num_target_errors = 0;
      source_p->last_target_complain = NOW;
    }
    return 1;
  }
  return 0;
}
#endif


/*
 * m_message (used in m_private() and m_notice()) the general
 * function to deliver MSG's between users/channels
 *
 * parv[0] = sender prefix
 * parv[1] = receiver list
 * parv[2] = message text
 *
 * massive cleanup * rev argv 6/91
 *
 */

static inline int
m_message (aClient * client_p, aClient * source_p, int parc, char *parv[],
           int notice)
{
  aClient *target_p;
  char *s;
  int i, ret, ischan;
  aChannel *channel_p;
  char *nick, *server, *p, *cmd, *dccmsg;
  char *n;

  cmd = notice ? MSG_NOTICE : MSG_PRIVATE;

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NORECIPIENT), me.name, parv[0], cmd);
    return -1;
  }

  if (parc < 3 || *parv[2] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NOTEXTTOSEND), me.name, parv[0]);
    return -1;
  }

  if (MyConnect (source_p))
  {
#ifdef ANTI_SPAMBOT
#ifndef ANTI_SPAMBOT_WARN_ONLY
    /*
     * if its a spambot, just ignore it
     */
    if (source_p->join_leave_count >= MAX_JOIN_LEAVE_COUNT)
    {
      return 0;
    }
#endif
#endif
    parv[1] = canonize (parv[1]);
  }

  for (p = NULL, nick = strtoken (&p, parv[1], ","), i = 0; nick && i < 20;
       nick = strtoken (&p, NULL, ","))
  {
    /*
     * If someone is spamming via "/msg nick1,nick2,nick3,nick4 SPAM"
     * (or even to channels) then subject them to flood control!
     * -Taner
     */
    if (i++ > 10)
    {
#ifdef NO_OPER_FLOOD
      if (!IsAnOper (source_p) && !IsULine (source_p))
      {
#endif
        source_p->since += 4;
#ifdef NO_OPER_FLOOD
      }
#endif
    }

    /*
     * channel msg?
     */
    ischan = IsChannelName (nick);
    if (ischan && (channel_p = find_channel (nick, NullChn)))
    {
      if (!notice)
        switch (check_for_ctcp (parv[2], NULL))
        {
           case CTCP_NONE:
             break;

           case CTCP_DCCSEND:
           case CTCP_DCC:
             sendto_one (source_p,
                         ":%s NOTICE %s :You may not send a DCC command to a channel (%s)",
                         me.name, parv[0], nick);
             continue;

           default:
#ifdef FLUD
             if (check_for_flud (source_p, NULL, channel_p, 1))
             {
               return 0;
             }
#endif
             break;
        }
      ret = IsULine (source_p) ? 0 : can_send (source_p, channel_p, parv[2]);

      if (ret)
      {
        if (!notice)            /* We now make an effort to tell the client why he cannot send to the channel - ShadowMaster */
        {
          if (can_send (source_p, channel_p, parv[2]) == MODE_MODERATED)
          {
            sendto_one (source_p, err_str (ERR_CANNOTSENDTOCHAN),
                        me.name, parv[0], nick, "Channel is moderated");
          }
          else if (can_send (source_p, channel_p, parv[2]) == MODE_NOPRIVMSGS)
          {
            sendto_one (source_p, err_str (ERR_CANNOTSENDTOCHAN),
                        me.name, parv[0], nick,
                        "Channel does not accept messages from non members");
          }
          else if (can_send (source_p, channel_p, parv[2]) == MODE_MODREG)
          {
            sendto_one (source_p, err_str (ERR_CANNOTSENDTOCHAN),
                        me.name, parv[0], nick,
                        "Channel does not accept messages from non registered users");
          }
          else if (can_send (source_p, channel_p, parv[2]) == MODE_NOCOLOR)
          {
            sendto_one (source_p, err_str (ERR_CANNOTSENDTOCHAN),
                        me.name, parv[0], nick,
                        "Channel does not accept messages containing color codes");
          }
          else if (can_send (source_p, channel_p, parv[2]) == MODE_BAN)
          {
            sendto_one (source_p, err_str (ERR_CANNOTSENDTOCHAN),
                        me.name, parv[0], nick, "You are banned");
          }
        }
      }
      else
      {
        sendto_channel_butone (client_p, source_p, channel_p,
                               ":%s %s %s :%s", parv[0], cmd, nick, parv[2]);
      }
      continue;

    }

    /*
     * nickname addressed?
     */
    if (!ischan && (target_p = find_person (nick, NULL)))
    {
      if (IsNoNonReg (target_p) && !IsRegNick (source_p)
          && !IsULine (source_p) && !IsAnOper (source_p))
      {
        sendto_one (source_p, rpl_str (ERR_NONONREG), me.name, parv[0],
                    target_p->name);
        continue;
      }

#ifdef MSG_TARGET_LIMIT
      /* Only check target limits for my clients */
      if (MyClient (source_p) && check_target_limit (source_p, target_p))
      {
        continue;
      }
#endif
#ifdef ANTI_DRONE_FLOOD
      if (MyConnect (target_p) && IsClient (source_p)
          && !IsAnOper (source_p) && !IsULine (source_p) && DRONE_TIME)
      {
        if ((target_p->first_received_message_time + DRONE_TIME) < timeofday)
        {
          target_p->received_number_of_privmsgs = 1;
          target_p->first_received_message_time = timeofday;
          target_p->drone_noticed = 0;
        }
        else
        {
          if (target_p->received_number_of_privmsgs > DRONE_COUNT)
          {
            if (target_p->drone_noticed == 0)   /* tiny FSM */
            {
#ifdef DRONE_WARNINGS
              sendto_netglobal
                ("from %s: Possible Drone Flooder %s [%s@%s] on %s target: %s",
                 me.name, source_p->name,
                 source_p->user->username, source_p->user->host,
                 source_p->user->server, target_p->name);
#endif
              target_p->drone_noticed = 1;
            }
            /* heuristic here, if target has been getting a lot
             * of privmsgs from clients, and sendq is above halfway up
             * its allowed sendq, then throw away the privmsg, otherwise
             * let it through. This adds some protection, yet doesn't
             * DOS the client.
             * -Dianora
             */
            if (DBufLength (&target_p->sendQ) > (get_sendq (target_p) / 2L))
            {
              if (target_p->drone_noticed == 1) /* tiny FSM */
              {
                sendto_serv_butone (client_p,
                                    ":%s NETGLOBAL : ANTI_DRONE_FLOOD SendQ protection activated for %s",
                                    me.name, target_p->name);

                sendto_netglobal
                  ("from %s: ANTI_DRONE_FLOOD SendQ protection activated for %s",
                   me.name, target_p->name);

                sendto_one (target_p,
                            ":%s NOTICE %s :*** Notice -- Server drone flood protection activated for %s",
                            me.name, target_p->name, target_p->name);
                target_p->drone_noticed = 2;
              }
            }

            if (DBufLength (&target_p->sendQ) <= (get_sendq (target_p) / 4L))
            {
              if (target_p->drone_noticed == 2)
              {
                sendto_one (target_p,
                            ":%s NOTICE %s :*** Notice -- Server drone flood protection de-activated for %s",
                            me.name, target_p->name, target_p->name);
                target_p->drone_noticed = 1;
              }
            }
            if (target_p->drone_noticed > 1)
              return 0;
          }
          else
            target_p->received_number_of_privmsgs++;
        }
      }
#endif
#ifdef FLUD
      if (!notice && MyFludConnect (target_p))
#else
      if (!notice && MyConnect (target_p))
#endif
      {

        switch (check_for_ctcp (parv[2], &dccmsg))
        {
           case CTCP_NONE:
             break;

           case CTCP_DCCSEND:
#ifdef FLUD
             if (check_for_flud (source_p, target_p, NULL, 1))
             {
               return 0;
             }
#endif

             if (check_dccsend (source_p, target_p, dccmsg))
             {
               continue;
             }
             break;

           default:
#ifdef FLUD
             if (check_for_flud (source_p, target_p, NULL, 1))
             {
               return 0;
             }
#endif
             break;
        }
      }

      if (!is_silenced (source_p, target_p))
      {
        if (!notice && MyClient (target_p) && target_p->user
            && target_p->user->away)
        {
          sendto_one (source_p, rpl_str (RPL_AWAY), me.name, parv[0],
                      target_p->name, target_p->user->away);
        }
        sendto_prefix_one (target_p, source_p, ":%s %s %s :%s", parv[0],
                           cmd, nick, parv[2]);
      }
      continue;
    }

    /*
     ** We probably should clean this up and use a loop later - ShadowMaster
     */
    if ((nick[1] == '#' || nick[2] == '#' || nick[3] == '#'
         || nick[4] == '#') && nick[0] != '#')
    {
      n = (char *) strchr (nick, '#');

      if (n && (channel_p = find_channel (n, NullChn)))
      {
        if (can_send (source_p, channel_p, parv[2]) == 0
            || IsULine (source_p))
        {
          /*
           *** FIXME ***
           *
           * This is a major mess and needs to be rewritten to handle things properly
           * (ATM it doesnt even do what its supposed to do
           */

          if (strchr (nick, '!') || strchr (nick, '@') || strchr (nick, '%'))
          {
            sendto_allchannelops_butone (client_p, source_p,
                                         channel_p, ":%s %s %s :%s",
                                         parv[0], cmd, nick, parv[2]);
          }
          else if (strchr (nick, '+'))
          {
            /*if (strchr (nick, '+')) */
            sendto_channelvoice_butone (client_p, source_p,
                                        channel_p, ":%s %s %s :%s",
                                        parv[0], cmd, nick, parv[2]);
          }
          /*
             if (strchr(nick, '!')) sendto_channeladmins_butone(client_p, source_p, channel_p, ":%s %s %s :%s", parv[0], cmd, nick, parv[2]);
             if (strchr(nick, '@')) sendto_channelops_butone(client_p, source_p, channel_p, ":%s %s %s :%s", parv[0], cmd, nick, parv[2]);
             if (strchr(nick, '%')) sendto_channelhalfops_butone(client_p, source_p, channel_p, ":%s %s %s :%s", parv[0], cmd, nick, parv[2]);
           */
        }
        else if (!notice)
        {
          sendto_one (source_p, err_str (ERR_CANNOTSENDTOCHAN),
                      me.name, parv[0], n, "Must be of type NOTICE");
        }
      }
      else
      {
        sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0], n);
      }
      continue;
    }

    if (IsAnOper (source_p) || IsULine (source_p))
    {
      /*
       * the following two cases allow masks in NOTICEs
       * (for OPERs only)
       *
       * Armin, 8Jun90 (gruner@informatik.tu-muenchen.de)
       */
      if ((*nick == '$' || *nick == '#'))
      {
        if (!(s = (char *) strrchr (nick, '.')))
        {
          sendto_one (source_p, err_str (ERR_NOTOPLEVEL), me.name,
                      parv[0], nick);
          continue;
        }
        while (*++s)
          if (*s == '.' || *s == '*' || *s == '?')
          {
            break;
          }
        if (*s == '*' || *s == '?')
        {
          sendto_one (source_p, err_str (ERR_WILDTOPLEVEL), me.name,
                      parv[0], nick);
          continue;
        }
        sendto_match_butone (IsServer (client_p) ? client_p : NULL,
                             source_p, nick + 1,
                             (*nick == '#') ? MATCH_HOST : MATCH_SERVER,
                             ":%s %s %s :%s", parv[0], cmd, nick, parv[2]);
        continue;
      }
    }

    /*
     * user@server addressed?
     */
    if (!ischan && (server = (char *) strchr (nick, '@'))
        && (target_p = find_server (server + 1, NULL)))
    {
      int count = 0;

      /* Not destined for a user on me :-( */
      if (!IsMe (target_p))
      {
        sendto_one (target_p, ":%s %s %s :%s", parv[0], cmd, nick, parv[2]);
        continue;
      }
      *server = '\0';

      /*
       * Look for users which match the destination host
       * (no host == wildcard) and if one and one only is found
       * connected to me, deliver message!
       */
      target_p = find_person (nick, NULL);
      if (server)
      {
        *server = '@';
      }
      if (target_p)
      {
        if (count == 1)
        {
          sendto_prefix_one (target_p, source_p, ":%s %s %s :%s",
                             parv[0], cmd, nick, parv[2]);
        }
        else if (!notice)
        {
          sendto_one (source_p, err_str (ERR_TOOMANYTARGETS), me.name,
                      parv[0], nick);
        }
      }
      if (target_p)
      {
        continue;
      }
    }
    sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0], nick);
  }
  if ((i > 20) && source_p->user)
  {
    sendto_realops_lev (SPY_LEV, "User %s (%s@%s) tried to msg %d users",
                        source_p->name, source_p->user->username,
                        source_p->user->host, i);
  }
  return 0;
}

/*
 * * m_private *      parv[0] = sender prefix *       parv[1] =
 * receiver list *      parv[2] = message text
 */

int
m_private (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  return m_message (client_p, source_p, parc, parv, 0);
}


/*
 * * m_notice *       parv[0] = sender prefix *       parv[1] = receiver list *
 * parv[2] = notice text
 */

int
m_notice (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  return m_message (client_p, source_p, parc, parv, 1);
}


/*
** get_mode_str
** by vmlinuz
** returns an ascii string of modes
*/
char *
get_mode_str (aClient * target_p)
{
  int flag;
  int *s;
  char *m;

  m = buf;
  *m++ = '+';
  for (s = user_modes; (flag = *s) && (m - buf < BUFSIZE - 4); s += 2)
    if ((target_p->umode & flag))
      *m++ = (char) (*(s + 1));
  *m = '\0';
  return buf;
}


/*
** get_smode_str
** returns an ascii string of modes
*/
char *
get_smode_str (aClient * target_p)
{
  int flag;
  int *s;
  char *m;

  m = buf2;
  *m++ = '+';
  for (s = server_modes; (flag = *s) && (m - buf2 < BUFSIZE - 4); s += 2)
    if ((target_p->smode & flag))
      *m++ = (char) (*(s + 1));
  *m = '\0';
  return buf2;
}


/*
 * * m_whois *        parv[0] = sender prefix *       parv[1] = nickname
 * masklist
 */
int
m_whois (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  static anUser UnknownUser = {
    NULL,                       /* channel */
    NULL,                       /* invited */
    NULL,                       /* away */
    0,                          /* last */
    0,                          /* joined */
    "<Unknown>",                /* user */
    "<Unknown>",                /* host */
    "<Unknown>",                /* virthost */
    "<Unknown>",                /* server */
    0,                          /* servicestamp */
    NULL                        /* silenced */
  };

  Link *lp;
  anUser *user;
  aClient *target_p, *a2client_p;
  aChannel *channel_p;
  char *nick, *tmp, *name;
  char *p = NULL;
  int found, len, mlen;

  static time_t last_used = 0L;

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NONICKNAMEGIVEN), me.name, parv[0]);
    return 0;
  }

  if (parc > 2)
  {
    if (hunt_server (client_p, source_p, ":%s WHOIS %s :%s", 1, parc, parv)
        != HUNTED_ISME)
      return 0;
    parv[1] = parv[2];
  }

  for (tmp = parv[1]; (nick = strtoken (&p, tmp, ",")); tmp = NULL)
  {
    int invis, member, showchan;

    found = 0;
    collapse (nick);
    target_p = hash_find_client (nick, (aClient *) NULL);
    if (!target_p || !IsPerson (target_p))
    {
      sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0], nick);
      continue;
    }

    user = target_p->user ? target_p->user : &UnknownUser;
    name = (!*target_p->name) ? "?" : target_p->name;
    invis = IsInvisible (target_p);
    member = (user->channel) ? 1 : 0;

    a2client_p = find_server (user->server, NULL);


    if (!IsAnOper (source_p) && IsAnOper (target_p))
    {
      if ((last_used + MOTD_WAIT) > NOW)
        return 0;
      else
        last_used = NOW;
    }
    /*
     * If the WHOIS is done on an IRC Operator send the target a notice about it - ShadowMaster
     */
    /*
     * But not if its an oper doing a whois on himself *doh* - ShadowMaster
     */
    /* now it only shows if the user is +W --Acidic32 */
    if (IsAnOper (target_p) && WantsWhois (target_p)
        && !(source_p == target_p))
      sendto_one (target_p,
                  ":%s NOTICE %s :*** Notice -- /WHOIS command used on you by: %s (%s@%s)",
                  me.name, name, parv[0], source_p->user->username,
                  source_p->user->host);

    sendto_one (source_p, rpl_str (RPL_WHOISUSER), me.name, parv[0], name,
                /*
                 * Give out the hidden hostname if user is +x - ShadowMaster
                 */
                user->username,
                IsHidden (target_p) ? user->virthost : user->host,
                target_p->info);

    if (IsRegNick (target_p))
      sendto_one (source_p, rpl_str (RPL_WHOISREGNICK), me.name, parv[0],
                  name);


    /*
     * If the WHOIS is done by an IRC Operator or if the WHOIS is used on the user sending it give out
     * the users modes. - ShadowMaster
     */
    if (IsAnOper (source_p) || source_p == target_p)
    {
      sendto_one (source_p, rpl_str (RPL_WHOISMODES),
                  me.name, parv[0], name, get_mode_str (target_p),
                  get_smode_str (target_p));
    }

    /*
     * If the WHOIS is done by an IRC Operator or if the WHOIS is used on the user sending it give out
     * the real hostname and the IP. - ShadowMaster
     */
    if (IsHidden (target_p) && (IsAnOper (source_p) || source_p == target_p))
    {
      sendto_one (source_p, rpl_str (RPL_WHOISHOST), me.name, parv[0],
                  name, target_p->user->host, target_p->hostip);
    }


    mlen = strlen (me.name) + strlen (parv[0]) + 6 + strlen (name);
    for (len = 0, *buf = '\0', lp = user->channel; lp; lp = lp->next)
    {
      channel_p = lp->value.channel_p;
      showchan = ShowChannel (source_p, channel_p);
      if ((showchan || IsAnOper (source_p)) && !IsULine (target_p))
      {
        if (len + strlen (channel_p->chname) > (size_t) BUFSIZE - 4 - mlen)
        {
          sendto_one (source_p,
                      ":%s %d %s %s :%s",
                      me.name, RPL_WHOISCHANNELS, parv[0], name, buf);
          *buf = '\0';
          len = 0;
        }
        if (!showchan)          /* if we're not really supposed to show the chan
                                 * but do it anyways, mark it as such! */
          *(buf + len++) = '=';
        if (is_chan_admin (target_p, channel_p))        /* there a channel administrator --Acidic32 */
          *(buf + len++) = '!';
        if (is_chan_op (target_p, channel_p))   /* there a channel operator --Acidic32 */
          *(buf + len++) = '@';
        if (is_half_op (target_p, channel_p))   /* there a half operator --Acidic32 */
          *(buf + len++) = '%';
        else if (has_voice (target_p, channel_p))       /* there a voice in the chan --Acidic32 */
          *(buf + len++) = '+';
        if (len)
          *(buf + len) = '\0';
        strcpy (buf + len, channel_p->chname);
        len += strlen (channel_p->chname);
        strcat (buf + len, " ");
        len++;
      }
    }
    if (buf[0] != '\0')
      sendto_one (source_p, rpl_str (RPL_WHOISCHANNELS),
                  me.name, parv[0], name, buf);

    sendto_one (source_p, rpl_str (RPL_WHOISSERVER),
                me.name, parv[0], name, user->server,
                a2client_p ? a2client_p->info : "*Not On This Net*");

    if (IsSSLClient (target_p))
      sendto_one (source_p, rpl_str (RPL_USINGSSL), me.name, parv[0], name);

    if (user->away)
      sendto_one (source_p, rpl_str (RPL_AWAY), me.name,
                  parv[0], name, user->away);
    if (IsAnOper (target_p))
    {
      buf[0] = '\0';
      if (IsNetAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Network Administrator");
      }
      else if (IsNetCoAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Network Co Administrator");
      }
      else if (IsTechAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Technical Administrator");
      }
      else if (IsTechCoAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Technical Co Administrator");
      }
      else if (IsServerAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Server Administrator");
      }
      else if (IsServerCoAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Server Co Administrator");
      }
      else if (IsGuestAdmin (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Guest Administrator");
      }
      else if (IsOper (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "an IRC Operator");
      }
      else if (IsLocOp (target_p))
      {
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Local IRC Operator");
      }
    }

    if (IsSkoServicesStaff (target_p))
    {
      buf[0] = '\0';
      if (IsServicesRoot (target_p))
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Services Root Administrator");

      else if (IsServicesAdmin (target_p))
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Services Administrator");

      else if (IsServicesOper (target_p))
        sendto_one (source_p, rpl_str (RPL_WHOISOPERATOR),
                    me.name, parv[0], name, "a Services Operator");
    }

    if (IsServicesClient (target_p))
      sendto_one (source_p, rpl_str (RPL_WHOISSERVICES),
                  me.name, parv[0], name);

    if (IsHelpOp (target_p) && (source_p->user->away == NULL))
      sendto_one (source_p, rpl_str (RPL_WHOISHELPOP), me.name, parv[0],
                  name);


    /*
     * Small ugly piece of code, do not display Opers Idle time to non opers - ShadowMaster
     */
    if (target_p->user && MyConnect (target_p))
    {
      sendto_one (source_p, rpl_str (RPL_WHOISIDLE),
                  me.name, parv[0], name,
                  timeofday - user->last, target_p->firsttime);
    }
    continue;
    if (!found)
      sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0], nick);
    if (p)
      p[-1] = ',';
  }
  sendto_one (source_p, rpl_str (RPL_ENDOFWHOIS), me.name, parv[0], parv[1]);

  return 0;
}

/*
 * m_user
 *
 * parv[0] = sender prefix
 * parv[1] = username (login name, account) 
 * parv[2] = client host name (used only from other servers) 
 * parv[3] = server host name (used only from other servers)
 * parv[4] = users real name info
 */
int
m_user (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
#define	UFLAGS	(UMODE_i)
#define NFLAGS	(NMODE_w|NMODE_s)
  char *username, *host, *server, *realname;

  if (parc > 2 && (username = (char *) strchr (parv[1], '@')))
  {
    *username = '\0';
  }

  if (parc < 5 || *parv[1] == '\0' || *parv[2] == '\0' ||
      *parv[3] == '\0' || *parv[4] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "USER");
    if (IsServer (client_p))
    {
      sendto_realops ("bad USER param count for %s from %s",
                      parv[0], get_client_name (client_p, FALSE));
    }
    else
    {
      return 0;
    }
  }
  /* Copy parameters into better documenting variables */
  username = (parc < 2 || BadPtr (parv[1])) ? "<bad-boy>" : parv[1];
  host = (parc < 3 || BadPtr (parv[2])) ? "<nohost>" : parv[2];
  server = (parc < 4 || BadPtr (parv[3])) ? "<noserver>" : parv[3];
  realname = (parc < 5 || BadPtr (parv[4])) ? "<bad-realname>" : parv[4];

  return do_user (parv[0], client_p, source_p, username, host, server, 0, 0,
                  realname, "*");
}

/*
 * do_user
 */
int
do_user (char *nick,
         aClient * client_p,
         aClient * source_p,
         char *username,
         char *host,
         char *server,
         unsigned long serviceid,
         unsigned int ip, char *realname, char *virthost)
{
  anUser *user;

  long oflags;

  user = make_user (source_p);
  oflags = source_p->umode;

  if (!MyConnect (source_p))
  {
    user->server = find_or_add (server);
    strncpyzt (user->host, host, sizeof (user->host));
    if (virthost)
    {
      strncpyzt (user->virthost, virthost, sizeof (user->virthost));
    }
  }
  else
  {
    if (!IsUnknown (source_p))
    {
      sendto_one (source_p, err_str (ERR_ALREADYREGISTRED), me.name, nick);
      return 0;
    }
    if (DEF_MODE_i == 1)
      source_p->umode |= UMODE_i;

    if (MODE_x == 1)
      source_p->umode |= UMODE_x;

#ifdef USE_SSL
    if (IsSSL (source_p))
      SetSSLClient (source_p);
#endif


    source_p->umode |= (UFLAGS & atoi (host));
    source_p->nmode |= (NFLAGS & atoi (host));
    strncpyzt (user->host, host, sizeof (user->host));
    user->server = me.name;
  }
  strncpyzt (source_p->info, realname, sizeof (source_p->info));

  source_p->user->servicestamp = serviceid;
  if (!MyConnect (source_p))
  {

#ifndef INET6
    if (IsIPV6Client (source_p))
    {
      source_p->ip.S_ADDR = ip;
    }
    else
    {
      /* no ipv4 at all .. FIXMEs */
      source_p->ip.S_ADDR = ntohl (ip);
    }
#else
    memset (source_p->ip.S_ADDR, 0x0, sizeof (struct IN_ADDR));
#endif
#ifdef THROTTLE_ENABLE
    /* add non-local clients to the throttle checker. obviously, we only
     * do this for REMOTE clients!@$$@! throttle_check() is called
     * elsewhere for the locals! -wd */
    if (ip != 0)
      throttle_check (inet_ntop (AFINET, (char *) &source_p->ip,
                                 mydummy, sizeof (mydummy)), -1,
                      source_p->tsinfo);
#endif
  }
  if (MyConnect (source_p))
  {
    source_p->oflag = 0;
  }
  if (source_p->name[0])        /*
                                 * NICK already received, now I have USER...
                                 */
    return register_user (client_p, source_p, source_p->name, username);
  else
    strncpyzt (source_p->user->username, username, USERLEN + 1);
  return 0;
}

/*
 * * m_quit * parv[0] = sender prefix *       parv[1] = comment
 */
int
m_quit (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  char *reason = (parc > 1 && parv[1]) ? parv[1] : client_p->name;
  char comment[TOPICLEN + 1];

  source_p->flags |= FLAGS_NORMALEX;
  if (!IsServer (client_p))
  {
#ifdef ANTI_SPAM_EXIT_MESSAGE
    if (MyConnect (source_p)
        && (source_p->firsttime + ANTI_SPAM_EXIT_MESSAGE_TIME) > timeofday)
      strcpy (comment, "Client exited");
    else
    {
#endif
      strcpy (comment, "Quit: ");
      strncpy (comment + 6, reason, TOPICLEN - 6);
      comment[TOPICLEN] = 0;
#ifdef ANTI_SPAM_EXIT_MESSAGE
    }
#endif
    return exit_client (client_p, source_p, source_p, comment);
  }
  else
    return exit_client (client_p, source_p, source_p, reason);
}

/*
 * * m_kill * parv[0] = sender prefix *       parv[1] = kill victim *
 * parv[2] = kill path
 *
 * Hidden hostmasks are now used, to stop users who are +sk seeing users hosts
 */
int
m_kill (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *user, *path, *killer, *p, *nick;
  char mypath[TOPICLEN + 1];
  char *unknownfmt = "<Unknown>";       /*
                                         * AFAIK this shouldnt happen
                                         * * but -Raist 
                                         */
  int chasing = 0, kcount = 0;

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "KILL");
    return 0;
  }

  user = parv[1];
  path = parv[2];               /*
                                 * Either defined or NULL (parc >= 2!!)
                                 */
  if (path == NULL)
    path = ")";

  if (!IsPrivileged (client_p))
  {
    sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
    return 0;
  }
  if (IsAnOper (client_p))
  {
    if (!BadPtr (path))
      if (strlen (path) > (size_t) TOPICLEN)
        path[TOPICLEN] = '\0';
  }
  if (MyClient (source_p))
    user = canonize (user);
  for (p = NULL, nick = strtoken (&p, user, ","); nick;
       nick = strtoken (&p, NULL, ","))
  {
    chasing = 0;
    if (!(target_p = find_client (nick, NULL)))
    {
      /*
       * * If the user has recently changed nick, we automaticly *
       * rewrite the KILL for this new nickname--this keeps *
       * servers in synch when nick change and kill collide
       */
      if (!(target_p = get_history (nick, (long) KILLCHASETIMELIMIT)))
      {
        sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name,
                    parv[0], nick);
        return 0;
      }
      sendto_one (source_p, ":%s NOTICE %s :KILL changed from %s to %s",
                  me.name, parv[0], nick, target_p->name);
      chasing = 1;
    }

    if ((!MyConnect (target_p) && MyClient (client_p)
         && !OPCanGKill (client_p)) || (MyConnect (target_p)
                                        && MyClient (client_p)
                                        && !OPCanLKill (client_p)))
    {
      sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
      continue;
    }

    if (IsServicesClient (target_p))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :Cannot kill services clients.", me.name,
                  parv[0]);
      continue;
    }

    if (IsServer (target_p) || IsMe (target_p) ||
        (MyClient (source_p) && IsULine (target_p)))
    {
      sendto_one (source_p, err_str (ERR_CANTKILLSERVER), me.name, parv[0]);
      continue;
    }

    kcount++;
    if (!IsServer (source_p) && (kcount > MAXKILLS))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :Too many targets, kill list was truncated. Maximum is %d.",
                  me.name, source_p->name, MAXKILLS);
      break;
    }

    if (MyClient (source_p))
    {
      char myname[HOSTLEN + 1], *s;
      int slen = 0;

      strncpy (myname, me.name, HOSTLEN + 1);
      if ((s = index (myname, '.')))
/*	    *s = 0;*/

        /* "<myname>!<source_p->user->host>!<source_p->name> (path)" */
        slen =
          TOPICLEN - (strlen (source_p->name) +
                      (IsHidden (source_p)
                       ? (strlen (source_p->user->virthost))
                       : (strlen (source_p->user->host))) +
                      strlen (myname) + 8);
      if (slen < 0)
        slen = 0;

      if (strlen (path) > slen)
        path[slen] = '\0';

      ircsprintf (mypath, "%s!%s!%s (%s)", myname,
                  (IsHidden (source_p) ? source_p->user->
                   virthost : source_p->user->host), source_p->name, path);
      mypath[TOPICLEN] = '\0';
    }
    else
      strncpy (mypath, path, TOPICLEN + 1);
    /*
     * * Notify all *local* opers about the KILL, this includes the
     * one * originating the kill, if from this server--the special
     * numeric * reply message is not generated anymore. *
     * 
     * Note: "target_p->name" is used instead of "user" because we may *
     * ave changed the target because of the nickname change.
     */
    if (IsLocOp (source_p) && !MyConnect (target_p))
    {
      sendto_one (source_p, err_str (ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }
    if (IsAnOper (source_p))
      sendto_ops ("Received KILL message for %s!%s@%s. From %s Path: %s",
                  target_p->name,
                  target_p->user ? target_p->user->username : unknownfmt,
                  target_p->user ? ((IsHidden (target_p) ? target_p->
                                     user->virthost : target_p->
                                     user->host)) : unknownfmt, parv[0],
                  mypath);
    else
      sendto_realops_lev (SKILL_LEV,
                          "Received KILL message for %s!%s@%s. From %s Path: %s",
                          target_p->name,
                          target_p->user ? target_p->user->
                          username : unknownfmt,
                          target_p->
                          user
                          ? ((IsHidden (target_p) ? target_p->user->
                              virthost : target_p->user->
                              host)) : unknownfmt, parv[0], mypath);

#if defined(USE_SYSLOG) && defined(SYSLOG_KILL)
    if (IsOper (source_p))
      syslog (LOG_INFO, "KILL From %s!%s@%s For %s Path %s",
              parv[0], target_p->name,
              target_p->user ? target_p->user->username : unknownfmt,
              target_p->user ? target_p->user->host : unknownfmt, mypath);
#endif
    if (IsOper (source_p) && (MyConnect (target_p) || MyConnect (source_p)))
      ilog (LOGF_KILLS, "KILL From %s!%s@%s For %s Path %s",
            parv[0], target_p->name,
            target_p->user ? target_p->user->username : unknownfmt,
            target_p->user ? target_p->user->host : unknownfmt, mypath);
    /*
     * * And pass on the message to other servers. Note, that if KILL *
     * was changed, the message has to be sent to all links, also *
     * back. * Suicide kills are NOT passed on --SRB
     */
    if (!MyConnect (target_p) || !MyConnect (source_p)
        || !IsAnOper (source_p))
    {
      sendto_serv_butone (client_p, ":%s KILL %s :%s",
                          parv[0], target_p->name, mypath);
      if (chasing && IsServer (client_p))
        sendto_one (client_p, ":%s KILL %s :%s",
                    me.name, target_p->name, mypath);
      target_p->flags |= FLAGS_KILLED;
    }
    /*
     * * Tell the victim she/he has been zapped, but *only* if * the
     * victim is on current server--no sense in sending the *
     * notification chasing the above kill, it won't get far * anyway
     * (as this user don't exist there any more either)
     */
    if (MyConnect (target_p))
      sendto_prefix_one (target_p, source_p, ":%s KILL %s :%s",
                         parv[0], target_p->name, mypath);
    /*
     * * Set FLAGS_KILLED. This prevents exit_one_client from sending *
     * the unnecessary QUIT for this. ,This flag should never be *
     * set in any other place...
     */
    if (MyConnect (target_p) && MyConnect (source_p) && IsAnOper (source_p))
      ircsprintf (buf2, "Local kill by %s (%s)", source_p->name,
                  BadPtr (parv[2]) ? source_p->name : parv[2]);
    else
    {
      killer = strchr (mypath, '(');
      if (killer == NULL)
        killer = "()";
      ircsprintf (buf2, "Killed (%s %s)", source_p->name, killer);
    }
    if (exit_client (client_p, target_p, source_p, buf2) == FLUSH_BUFFER)
      return FLUSH_BUFFER;
  }
  return 0;
}

/***********************************************************************
	 * m_away() - Added 14 Dec 1988 by jto.
 *            Not currently really working, I don't like this
 *            call at all...
 *
 *            ...trying to make it work. I don't like it either,
 *	      but perhaps it's worth the load it causes to net.
 *	      This requires flooding of the whole net like NICK,
 *	      USER, MODE, etc messages...  --msa
 *
 * 	      Added FLUD-style limiting for those lame scripts out there.
 ***********************************************************************/
/*
 * * m_away * parv[0] = sender prefix *       parv[1] = away message
 */
int
m_away (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  char *away, *awy2 = parv[1];
  /*
   * make sure the user exists
   */
  if (!(source_p->user))
  {
    sendto_realops_lev (DEBUG_LEV, "Got AWAY from nil user, from %s (%s)\n",
                        client_p->name, source_p->name);
    return 0;
  }

  away = source_p->user->away;

#ifdef NO_AWAY_FLUD
  if (MyClient (source_p))
  {
    if ((source_p->alas + MAX_AWAY_TIME) < NOW)
    {
      source_p->acount = 0;
    }
    source_p->alas = NOW;
    source_p->acount++;
  }
#endif

  if (parc < 2 || !*awy2)
  {
    /*
     * Marking as not away
     */

    if (away)
    {
      MyFree (away);
      source_p->user->away = NULL;
      /* Don't spam unaway unless they were away - lucas */
      sendto_serv_butone (client_p, ":%s AWAY", parv[0]);
    }

    if (MyConnect (source_p))
    {
      sendto_one (source_p, rpl_str (RPL_UNAWAY), me.name, parv[0]);
    }
    return 0;
  }

  /*
   * Marking as away
   */
#ifdef NO_AWAY_FLUD
  /* we dont care if they are just unsetting away, hence this is here */
  /* only care about local non-opers */
  if (MyClient (source_p) && (source_p->acount > MAX_AWAY_COUNT)
      && !IsAnOper (source_p))
  {
    sendto_one (source_p, err_str (ERR_TOOMANYAWAY), me.name, parv[0]);
    return 0;
  }
#endif
  if (strlen (awy2) > (size_t) TOPICLEN)
  {
    awy2[TOPICLEN] = '\0';
  }
  /*
   * some lamers scripts continually do a /away, hence making a lot of
   * unnecessary traffic. *sigh* so... as comstud has done, I've
   * commented out this sendto_serv_butone() call -Dianora
   * readded because of anti-flud stuffs -epi
   */

  sendto_serv_butone (client_p, ":%s AWAY :%s ", parv[0], parv[1]);

  if (away)
  {
    MyFree (away);
  }
  away = (char *) MyMalloc (strlen (awy2) + 1);
  strcpy (away, awy2);

  source_p->user->away = away;

  if (MyConnect (source_p))
  {
    sendto_one (source_p, rpl_str (RPL_NOWAWAY), me.name, parv[0]);
  }
  return 0;
}

/*
 * * m_ping * parv[0] = sender prefix *       parv[1] = origin *
 * parv[2] = destination
 */
int
m_ping (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *origin, *destination;

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NOORIGIN), me.name, parv[0]);
    return 0;
  }
  origin = parv[1];
  destination = parv[2];        /*
                                 * Will get NULL or pointer (parc >=
                                 * * 2!!) 
                                 */

  target_p = find_client (origin, NULL);
  if (!target_p)
    target_p = find_server (origin, NULL);
  if (target_p && target_p != source_p)
    origin = client_p->name;
  if (!BadPtr (destination) && (irccmp (destination, me.name) != 0))
  {
    if ((target_p = find_server (destination, NULL)))
      sendto_one (target_p, ":%s PING %s :%s", parv[0], origin, destination);
    else
    {
      sendto_one (source_p, err_str (ERR_NOSUCHSERVER),
                  me.name, parv[0], destination);
      return 0;
    }
  }
  else
    sendto_one (source_p, ":%s PONG %s :%s", me.name,
                (destination) ? destination : me.name, origin);
  return 0;
}

/*
 * * m_pong * parv[0] = sender prefix *       parv[1] = origin *
 * parv[2] = destination
 */
int
m_pong (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *origin, *destination;

  if (parc < 2 || *parv[1] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NOORIGIN), me.name, parv[0]);
    return 0;
  }

  origin = parv[1];
  destination = parv[2];
  client_p->flags &= ~FLAGS_PINGSENT;
  source_p->flags &= ~FLAGS_PINGSENT;

  /* if it's my client and it's a server.. */
  if (source_p == client_p && IsServer (client_p))
  {
    if (source_p->flags & FLAGS_USERBURST)
    {
      source_p->flags &= ~FLAGS_USERBURST;
      sendto_gnotice
        ("from %s: %s has processed user/channel burst, sending topic burst.",
         me.name, source_p->name);
      send_topic_burst (source_p);
      source_p->flags |= FLAGS_PINGSENT | FLAGS_SOBSENT;
      sendto_one (source_p, "PING :%s", me.name);
    }
    else if (source_p->flags & FLAGS_TOPICBURST)
    {
      source_p->flags &= ~FLAGS_TOPICBURST;
      sendto_gnotice
        ("from %s: %s has processed topic burst (synched to network data).",
         me.name, source_p->name);
      if (HUB == 1)
      {
        sendto_serv_butone (source_p,
                            ":%s GNOTICE :%s has synched to network data.",
                            me.name, source_p->name);
      }
      /* Kludge: Get the "sync" message on small networks immediately */
      sendto_one (source_p, "PING :%s", me.name);
    }
  }

  /*
   * Now attempt to route the PONG, comstud pointed out routable PING
   * is used for SPING.  routable PING should also probably be left in
   * -Dianora That being the case, we will route, but only for
   * registered clients (a case can be made to allow them only from
   * servers). -Shadowfax
   */
  if (!BadPtr (destination) && (irccmp (destination, me.name) != 0)
      && IsRegistered (source_p))
  {
    if ((target_p = find_client (destination, NULL)) ||
        (target_p = find_server (destination, NULL)))
      sendto_one (target_p, ":%s PONG %s %s", parv[0], origin, destination);
    else
    {
      sendto_one (source_p, err_str (ERR_NOSUCHSERVER),
                  me.name, parv[0], destination);
      return 0;
    }
  }

#ifdef	DEBUGMODE
  else
    Debug ((DEBUG_NOTICE, "PONG: %s %s", origin,
            destination ? destination : "*"));
#endif
  return 0;
}

/*
 * m_oper
 *
 * parv[0] = sender prefix
 * parv[1] = oper name
 * parv[2] = oper password
 */
int
m_oper (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aConfItem *aconf;
  char *name, *password, *encr, *oper_ip;
  char tmpstr[512];
  int fd;

#ifdef CRYPT_OPER_PASSWORD
  extern char *crypt ();

#endif /*
        * CRYPT_OPER_PASSWORD
        */

  name = parc > 1 ? parv[1] : (char *) NULL;
  password = parc > 2 ? parv[2] : (char *) NULL;

  if (!IsServer (client_p) && (BadPtr (name) || BadPtr (password)))
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "OPER");
    return 0;
  }

  /* if message arrived from server, trust it, and set to oper */

  if ((IsServer (client_p) || IsMe (client_p)) && !IsOper (source_p))
  {

    source_p->umode |= UMODE_o;
    sendto_serv_butone (client_p, ":%s MODE %s :+o", parv[0], parv[0]);

    Count.oper++;
    if (IsMe (client_p))
      sendto_one (source_p, rpl_str (RPL_YOUREOPER), me.name, parv[0]);
    return 0;
  }
  else if (IsAnOper (source_p))
  {
    if (MyConnect (source_p))
      sendto_one (source_p, rpl_str (RPL_YOUREOPER), me.name, parv[0]);

    fd = open (OMOTD, O_RDONLY);
    if (!(fd == -1))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- opermotd was last changed at %s",
                  me.name, parv[0], opermotd_last_changed_date);
    }
    close (fd);

    return 0;
  }

  if (!(aconf = find_conf_exact (name, source_p->username, source_p->sockhost,
                                 CONF_OPS)) &&
      !(aconf = find_conf_exact (name, source_p->username,
                                 client_p->hostip, CONF_OPS)))
  {
    sendto_one (source_p, err_str (ERR_NOOPERHOST), me.name, parv[0]);
    sendto_serv_butone
      (client_p,
       ":%s NETGLOBAL : Failed OPER attempt for [%s] by %s (%s@%s) (no O:line)",
       me.name, name, parv[0], source_p->user->username,
       source_p->user->host);
    sendto_netglobal
      ("from %s: Failed OPER attempt for [%s] by %s (%s@%s) (no O:line)",
       me.name, name, parv[0], source_p->user->username,
       source_p->user->host);
    ilog (LOGF_OPER, "Failed OPER attempt for [%s] by %s (%s@%s) (no O:line)",
          name, parv[0], source_p->user->username, source_p->user->host);
    return 0;
  }

  oper_ip = source_p->hostip;
#ifdef CRYPT_OPER_PASSWORD
  /* use first two chars of the password they send in as salt */
  /* passwd may be NULL pointer. Head it off at the pass... */
  if (password && *aconf->passwd)
  {
    encr = crypt (password, aconf->passwd);
  }
  else
  {
    encr = "";
  }
#else
  encr = password;
#endif /* CRYPT_OPER_PASSWORD */

  if ((aconf->status & CONF_OPS) &&
      StrEq (encr, aconf->passwd) && !attach_conf (source_p, aconf))
  {
    int old = (source_p->umode & ALL_UMODES);
    int old2 = (source_p->smode & SEND_SMODES);
    int old3 = (source_p->nmode & ALL_NMODES);
    char *s;

    s = strchr (aconf->host, '@');
    if (s == (char *) NULL)
    {
      sendto_one (source_p, err_str (ERR_NOOPERHOST), me.name, parv[0]);
      sendto_realops ("corrupt aconf->host = [%s]", aconf->host);
      return 0;
    }
    *s++ = '\0';

    /* Well, that's what we USED to do here, now we get our little routine of 'set_oper_access' to do all that
     * for us. It does the following :
     *        finds highest flag in the O-Line and sets it on us.
     *        Sets corresponding modes for us as well.
     *        Sets the hostmask for the oper up.
     *        Sends out any oper-up notices that need to be sent.
     * Hopefully this tidies things up a bit - Quinn (2003).
     */

    set_oper_access (source_p, aconf);

    source_p->oflag = aconf->port;

    send_umode_out (client_p, source_p, old);
    send_smode_out (client_p, source_p, old2);
    send_nmode_out (client_p, source_p, old3);

    Count.oper++;
    *--s = '@';
    add_to_list (&oper_list, source_p);
#ifdef THROTTLE_ENABLE
    throttle_remove (oper_ip);
#endif
    sendto_one (source_p, rpl_str (RPL_YOUREOPER), me.name, parv[0]);

    fd = open (OMOTD, O_RDONLY);
    if (!(fd == -1))
    {
      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- opermotd was last changed at %s",
                  me.name, parv[0], opermotd_last_changed_date);
    }
    close (fd);

    source_p->pingval = get_client_ping (source_p);
    source_p->sendqlen = get_sendq (source_p);

    /*
     * Ugly hack
     */
    if (irccmp (OPERS_AUTO_JOIN, "0") != 0)
    {
      strncpyzt (tmpstr, OPERS_AUTO_JOIN, 512);
    }
    if (irccmp (OPERS_AUTO_JOIN, "0") != 0)
    {
      char *chans[3] = {
        source_p->name, tmpstr, NULL
      };

      sendto_one (source_p,
                  ":%s NOTICE %s :*** Notice -- Network has Auto Join enabled for Channel(s): \2%s\2",
                  me.name, source_p->name, OPERS_AUTO_JOIN);

      m_join (source_p, source_p, 3, chans);
    }

#if !defined(CRYPT_OPER_PASSWORD) && (defined(FNAME_OPERLOG) || (defined(USE_SYSLOG) && defined(SYSLOG_OPER)))
    encr = "";
#endif
#if defined(USE_SYSLOG) && defined(SYSLOG_OPER)
    syslog (LOG_INFO, "OPER (%s) (%s) by (%s!%s@%s)",
            name, encr, parv[0], source_p->user->username,
            source_p->sockhost);
#endif
  }
  else
  {
    detach_conf (source_p, aconf);
    sendto_one (source_p, err_str (ERR_PASSWDMISMATCH), me.name, parv[0]);
#ifdef FAILED_OPER_NOTICE
    sendto_serv_butone
      (client_p,
       ":%s NETGLOBAL : Failed OPER attempt for [%s] by %s (%s@%s) (password mismatch)",
       me.name, name, parv[0], source_p->user->username,
       source_p->user->host);
    sendto_netglobal
      ("from %s: Failed OPER attempt for [%s] by %s (%s@%s) (password mismatch)",
       me.name, name, parv[0], source_p->user->username,
       source_p->user->host);
    ilog (LOGF_OPER,
          "Failed OPER attempt for [%s] by %s (%s@%s) (password mismatch)",
          name, parv[0], source_p->user->username, source_p->user->host);
#endif
  }
  return 0;
}

/***************************************************************************
 * m_pass() - Added Sat, 4 March 1989
 ***************************************************************************/
/*
 * * m_pass * parv[0] = sender prefix *       parv[1] = password *
 * parv[2] = optional extra version information
 */
int
m_pass (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  char *password = parc > 1 ? parv[1] : NULL;

  if (BadPtr (password))
  {
    sendto_one (client_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "PASS");
    return 0;
  }
  if (!MyConnect (source_p)
      || (!IsUnknown (client_p) && !IsHandshake (client_p)))
  {
    sendto_one (client_p, err_str (ERR_ALREADYREGISTRED), me.name, parv[0]);
    return 0;
  }
  strncpyzt (client_p->passwd, password, sizeof (client_p->passwd));
  if (parc > 2)
  {
    int l = strlen (parv[2]);

    if (l < 2)
      return 0;
    /*
     * if (strcmp(parv[2]+l-2, "TS") == 0) 
     */
    if (parv[2][0] == 'T' && parv[2][1] == 'S')
      client_p->tsinfo = (ts_val) TS_DOESTS;
  }
  return 0;
}

/*
 * m_userhost
 *
 * Added by Darren Reed 13/8/91 to aid clients and reduce the need for
 * complicated requests like WHOIS.
 *
 * Returns user/host information only (no spurious AWAY labels or channels).
 *
 * Rewritten by Thomas J. Stens� (ShadowMaster) 15/10/02 to clean it up a bit aswell as
 * add hiddenhost support.
 */
int
m_userhost (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *s, *p = NULL;
  int i, j = 0, len = 0;

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS), me.name, parv[0],
                "USERHOST");
    return 0;
  }

  *buf2 = '\0';

  for (i = 5, s = strtoken (&p, parv[1], " "); i && s;
       s = strtoken (&p, (char *) NULL, " "), i--)
  {
    if ((target_p = find_person (s, NULL)))
    {
      if (j == 0)
      {
        /* Always show realhost to self as some clients depend on this */
        if (MyClient (source_p) && (source_p == target_p))
        {
          ircsprintf (buf, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username, target_p->user->host);

        }
        else
        {
          ircsprintf (buf, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username,
                      (IsHidden (target_p) ? target_p->user->
                       virthost : target_p->user->host));
        }
        j++;
      }
      else
      {
        if (j == 1)
        {
          len = strlen (buf);
        }
        strcat (buf, " ");
        /* Always show realhost to self as some clients depend on this */
        if (MyClient (source_p) && (source_p == target_p))
        {
          ircsprintf (buf2, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username, target_p->user->host);

        }
        else
        {
          ircsprintf (buf2, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username,
                      (IsHidden (target_p) ? target_p->user->
                       virthost : target_p->user->host));
        }
        strncat (buf, buf2, sizeof (buf) - len);
        len += strlen (buf2);
        j++;
      }
    }
    else
    {
      sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0], s);
    }
  }
  if (j)
  {
    sendto_one (source_p, rpl_str (RPL_USERHOST), me.name, parv[0], buf);
  }

  /* We have a userhost for more than 1 client, and one or more needs to return realhost */
  if (j)
  {
    for (i = 5, s = strtoken (&p, parv[1], " "); i && s;
         s = strtoken (&p, (char *) NULL, " "), i--)
    {
      if ((target_p = find_person (s, NULL)))
      {
        if (IsHidden (target_p) && IsAnOper (source_p)
            && (source_p != target_p))
        {
          sendto_one (source_p, rpl_str (RPL_WHOISHOST), me.name,
                      parv[0], target_p->name, target_p->user->host,
                      target_p->hostip);
        }
      }
    }
  }
  return 0;
}


/*
 * m_userip
 *
 * A clone of m_userhost, but gives out the IP instead except in cases
 * where a non oper is requesting info for a client with hiddenhost
 * where the hiddenhost will be displayed, regardless if its a hidden IP or full Hostname.
 */
int
m_userip (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *s, *p = NULL;
  int i, j = 0, len = 0;

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS), me.name, parv[0],
                "USERIP");
    return 0;
  }

  *buf2 = '\0';

  for (i = 5, s = strtoken (&p, parv[1], " "); i && s;
       s = strtoken (&p, (char *) NULL, " "), i--)
  {
    if ((target_p = find_person (s, NULL)))
    {
      if (j == 0)
      {
        /* Always show realhost to self as some clients depend on this */
        if (MyClient (source_p) && (source_p == target_p))
        {
          ircsprintf (buf, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username, target_p->hostip);

        }
        else
        {
          ircsprintf (buf, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username,
                      (IsHidden (target_p) ? target_p->user->
                       virthost : target_p->hostip));
        }
        j++;
      }
      else
      {
        if (j == 1)
        {
          len = strlen (buf);
        }
        strcat (buf, " ");
        /* Always show realhost to self as some clients depend on this */
        if (MyClient (source_p) && (source_p == target_p))
        {
          ircsprintf (buf2, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username, target_p->hostip);

        }
        else
        {
          ircsprintf (buf2, "%s%s=%c%s@%s",
                      target_p->name,
                      IsAnOper (target_p) ? "*" : "",
                      (target_p->user->away) ? '-' : '+',
                      target_p->user->username,
                      (IsHidden (target_p) ? target_p->user->
                       virthost : target_p->hostip));
        }
        strncat (buf, buf2, sizeof (buf) - len);
        len += strlen (buf2);
        j++;
      }
    }
    else
    {
      sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0], s);
    }
  }
  if (j)
  {
    sendto_one (source_p, rpl_str (RPL_USERHOST), me.name, parv[0], buf);
  }

  /* We have a userhost for more than 1 client, and one or more needs to return realhost */
  if (j)
  {
    for (i = 5, s = strtoken (&p, parv[1], " "); i && s;
         s = strtoken (&p, (char *) NULL, " "), i--)
    {
      if ((target_p = find_person (s, NULL)))
      {
        if (IsHidden (target_p) && IsAnOper (source_p)
            && (source_p != target_p))
        {
          sendto_one (source_p, rpl_str (RPL_WHOISHOST), me.name,
                      parv[0], target_p->name, target_p->user->host,
                      target_p->hostip);
        }
      }
    }
  }
  return 0;
}


/*
 * m_ison added by Darren Reed 13/8/91 to act as an efficent user
 * indicator with respect to cpu/bandwidth used. Implemented for NOTIFY
 * feature in clients. Designed to reduce number of whois requests. Can
 * process nicknames in batches as long as the maximum buffer length.
 * 
 * format: ISON :nicklist
 */
/*
 * Take care of potential nasty buffer overflow problem -Dianora
 * 
 */

int
m_ison (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  aClient *target_p;
  char *s, **pav = parv;
  char *p = (char *) NULL;
  int len;
  int len2;
  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "ISON");
    return 0;
  }

  ircsprintf (buf, rpl_str (RPL_ISON), me.name, *parv);
  len = strlen (buf);
  if (!IsAnOper (client_p))
    client_p->priority += 20;   /*
                                 * this keeps it from moving to 'busy'
                                 * * list 
                                 */
  for (s = strtoken (&p, *++pav, " "); s;
       s = strtoken (&p, (char *) NULL, " "))
    if ((target_p = find_person (s, NULL)))
    {
      len2 = strlen (target_p->name);
      if ((len + len2 + 5) < sizeof (buf))
      {                         /*
                                 * make sure can never
                                 * * overflow  
         *//*
         * allow for extra ' ','\0' etc. 
         */
        strcat (buf, target_p->name);
        len += len2;
        strcat (buf, " ");
        len++;
      }
      else
        break;
    }
  sendto_one (source_p, "%s", buf);
  return 0;
}

/*
 * m_umode() added 15/10/91 By Darren Reed. parv[0] - sender parv[1] -
 * username to change mode for parv[2] - modes to change
 */
int
m_umode (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int flag;
  int *s;
  char **p, *m;
  aClient *target_p;
  int what, setflags;
  int old = (source_p->smode & SEND_SMODES);
  int old2 = (source_p->nmode & ALL_NMODES);
  int badflag = NO;             /*
                                 * Only send one bad flag notice
                                 * * -Dianora
                                 */
  what = MODE_ADD;
  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "MODE");
    return 0;
  }

  if (!(target_p = find_person (parv[1], NULL)))
  {
    if (MyConnect (source_p))
      sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                  me.name, parv[0], parv[1]);
    return 0;
  }

  if ((IsServer (source_p) || (source_p != target_p)
       || (target_p->from != source_p->from)))
  {
    if (!IsServer (client_p))
      sendto_one (source_p, err_str (ERR_USERSDONTMATCH), me.name, parv[0]);
    return 0;
  }


  if (parc < 3)
  {
    m = buf;
    *m++ = '+';
    for (s = user_modes; (flag = *s) && (m - buf < BUFSIZE - 4); s += 2)
    {
      if (source_p->umode & (flag & ALL_UMODES))
        *m++ = (char) (*(s + 1));
    }
    *m = '\0';
    sendto_one (source_p, rpl_str (RPL_UMODEIS), me.name, parv[0], buf);
    return 0;
  }

  /*
   * find flags already set for user
   */
  setflags = 0;
  for (s = user_modes; (flag = *s); s += 2)
    if (source_p->umode & flag)
      setflags |= flag;

  /*
   * parse mode change string(s)
   */
  for (p = &parv[2]; p && *p; p++)
    for (m = *p; *m; m++)
      switch (*m)
      {
         case '+':
           what = MODE_ADD;
           break;
         case '-':
           what = MODE_DEL;
           break;
           /*
            * we may not get these, but they shouldnt be in
            * default
            */
         case ' ':
         case '\r':
         case '\n':
         case '\t':
           break;
         case 'o':
           if (what == MODE_ADD)
           {
             if (IsServer (client_p) && !IsOper (source_p))
             {
               /*
                * Dont ever count ULined clients as they are services
                * of some sort.
                */
               if (!IsULine (source_p))
               {
                 ++Count.oper;
               }
               SetOper (source_p);
             }
           }
           else
           {
             if (!IsOper (source_p))
               break;

             ClearOper (source_p);

             if (!IsULine (source_p))
             {
               Count.oper--;
             }

             if (MyConnect (source_p))
             {
               det_confs_butmask (source_p, CONF_CLIENT & ~CONF_OPS);
               source_p->sendqlen = get_sendq (source_p);
               source_p->oflag = 0;
               remove_from_list (&oper_list, source_p, NULL);
             }

             if (MODE_x == 0)
             {
               source_p->umode &= ~UMODE_x;
             }
           }
           break;
         case 'O':
           if (what == MODE_ADD)
           {
             if (IsServer (client_p) && !IsLocOp (source_p))
             {
               /*
                * Dont ever count ULined clients as they are services
                * of some sort.
                */
               if (!IsULine (source_p))
               {
                 ++Count.oper;
               }
               SetLocOp (source_p);
             }
           }
           else
           {
             if (!IsLocOp (source_p))
               break;

             ClearLocOp (source_p);

             if (!IsULine (source_p))
             {
               Count.oper--;
             }

             if (MyConnect (source_p))
             {
               det_confs_butmask (source_p, CONF_CLIENT & ~CONF_OPS);
               source_p->sendqlen = get_sendq (source_p);
               source_p->oflag = 0;
               remove_from_list (&oper_list, source_p, NULL);
             }

             if (MODE_x == 0)
             {
               source_p->umode &= ~UMODE_x;
             }
           }
           break;
         case 'r':
           break;               /* users can't set themselves +r! */
         case 'x':
           if (MyClient (source_p) && !IsServer (client_p))
           {
             /*
              * This network doesnt use hiddenhosts, dont let users set themselves +x
              */
             if (MODE_x == 0 && what == MODE_ADD)
             {
               break;
             }
             /*
              * Else the network uses hiddenhosts
              * If the network wants to lock hiddenhosts on for all users
              * dont let them set -x
              */
             else if (MODE_x == 1 && MODE_x_LOCK == 1 && what == MODE_DEL)
             {
               break;
             }
           }

           if (what == MODE_ADD)
           {
             SetHidden (source_p);
           }
           else
           {
             ClearHidden (source_p);
           }
           break;
         case 'S':
           if (!IsServer (client_p) && !IsULine (source_p))
           {
             break;
           }

           if (what == MODE_ADD)
           {
             SetServicesClient (source_p);
           }
           else
           {
             ClearServicesClient (source_p);
           }
           break;

           if (!IsAnOper (source_p))
           {
             break;
           }

           if (what == MODE_ADD)
           {
             SetConnectNotice (source_p);
           }
           else
           {
             ClearGConnectNotice (source_p);
           }
           break;
         case 'W':
           if (!IsAnOper (source_p))
           {
             break;
           }

           if (what == MODE_ADD)
           {
             SetWantsWhois (source_p);
           }
           else
           {
             ClearWantsWhois (source_p);
           }
           break;
         case 'k':
           if (!IsAnOper (source_p))
           {
             break;
           }

           if (what == MODE_ADD)
           {
             SetSkillNotice (source_p);
           }
           else
           {
             ClearSkillNotice (source_p);
           }
           break;
         case 'g':
           if (!IsAnOper (source_p))
           {
             break;
           }

           if (what == MODE_ADD)
           {
             SetGlobops (source_p);
           }
           else
           {
             ClearGlobops (source_p);
           }
           break;
         case 'h':
           if (MyClient (source_p) && !IsAnOper (source_p)
               && !OPCanHelpOp (source_p))
           {
             break;
           }

           if (what == MODE_ADD)
           {
             SetHelpOp (source_p);
           }
           else
           {
             ClearHelpOp (source_p);
           }
           break;
         case 'd':
           if (what == MODE_ADD)
           {
             SetDeaf (source_p);
             if (MyClient (source_p))
             {
               sendto_one (source_p,
                           ":%s NOTICE %s :*** Notice -- You have marked yourself as deaf and will no longer receive channel messages or notices from any channel.",
                           me.name, source_p->name);
             }
           }
           else
           {
             ClearDeaf (source_p);
             if (MyClient (source_p))
             {
               sendto_one (source_p,
                           ":%s NOTICE %s :*** Notice -- You are no longer marked as deaf and will receive channel messages and notices as normal..",
                           me.name, source_p->name);
             }
           }
           break;
           /*
            * Legacy support for usermodes +s, +c and +w.
            */
         case 's':
           if (what == MODE_ADD)
           {
             SetServerNotice (source_p);
           }
           else
           {
             ClearServerNotice (source_p);
           }
           break;
         case 'w':
           if (what == MODE_ADD)
           {
             SetWallops (source_p);
           }
           else
           {
             ClearWallops (source_p);
           }
           break;
         case 'c':
           if (what == MODE_ADD)
           {
             SetConnectNotice (source_p);
           }
           else
           {
             ClearConnectNotice (source_p);
           }
           break;
         default:
           if ((flag = user_mode_table[(unsigned char) *m]))
           {
             if (what == MODE_ADD)
             {
               source_p->umode |= flag;
             }
             else
             {
               source_p->umode &= ~flag;
             }
           }
           else
           {
             if (MyConnect (source_p))
             {
               badflag = YES;
             }
           }
           break;
      }

  if (badflag)
    sendto_one (source_p, err_str (ERR_UMODEUNKNOWNFLAG), me.name, parv[0]);


  /*
   * compare new flags with old flags and send string which will cause
   * servers to update correctly.
   */
  if (!IsAnOper (source_p) && !IsServer (client_p))
  {
    if (IsServicesOper (source_p))
    {
      ClearServicesOper (source_p);
    }
    if (IsServicesAdmin (source_p))
    {
      ClearServicesRoot (source_p);
    }

    if (IsProtectedOper (source_p))
    {
      ClearProtectedOper (source_p);
    }

    if (SendChatops (source_p))
    {
      ClearChatops (source_p);
    }


    /*
     * Check for SMODE's that should be removed when a user deoper.
     */

    /* Guest Admin */
    if (IsGuestAdmin (source_p))
    {
      ClearGuestAdmin (source_p);
    }

    /* Server Co Admin */
    if (IsServerCoAdmin (source_p))
    {
      ClearServerCoAdmin (source_p);
    }

    /* Server Admin */
    if (IsServerAdmin (source_p))
    {
      ClearServerAdmin (source_p);
    }

    /* Tech Co Admin */
    if (IsTechCoAdmin (source_p))
    {
      ClearTechCoAdmin (source_p);
    }

    /* Tech Admin */
    if (IsTechAdmin (source_p))
    {
      ClearTechAdmin (source_p);
    }

    /* Net Co Admin */
    if (IsNetCoAdmin (source_p))
    {
      ClearNetCoAdmin (source_p);
    }

    /* Net Admin */
    if (IsNetAdmin (source_p))
    {
      ClearNetAdmin (source_p);
    }

    /*
     * Check for NMODE's that should be removed when a user deoper.
     */
    if (SendFloodNotice (source_p))
    {
      ClearFloodNotice (source_p);
    }

    if (SendConnectNotice (source_p))
    {
      ClearConnectNotice (source_p);
    }

    if (SendGConnectNotice (source_p))
    {
      ClearGConnectNotice (source_p);
    }

    if (SendSpyNotice (source_p))
    {
      ClearSpyNotice (source_p);
    }

    if (SendDebugNotice (source_p))
    {
      ClearDebugNotice (source_p);
    }

    if (SendNetInfo (source_p))
    {
      ClearNetInfo (source_p);
    }

    if (SendNetGlobal (source_p))
    {
      ClearNetGlobal (source_p);
    }

    if (SendRejectNotice (source_p))
    {
      ClearRejectNotice (source_p);
    }

    if (SendGlobops (source_p))
    {
      ClearGlobops (source_p);
    }

    if (SendSpamNotice (source_p))
    {
      ClearSpamNotice (source_p);
    }

    if (WantsWhois (source_p))
    {
      ClearWantsWhois (source_p);
    }

    if (SendRoutingNotice (source_p))
    {
      ClearRoutingNotice (source_p);
    }

    if (SendSkillNotice (source_p))
    {
      ClearSkillNotice (source_p);
    }
  }
  if (MyClient (source_p))
  {
    /*
     *** FIXME ***
     *
     * This needs to be brought up to date with current OLine flags.
     */

    if (IsServicesOper (source_p) && !OPIsServicesOper (source_p))
    {
      ClearServicesOper (source_p);
    }
    if (IsServicesAdmin (source_p) && !OPIsServicesAdmin (source_p))
    {
      ClearServicesAdmin (source_p);
    }
    if (IsServicesRoot (source_p) && !OPIsServicesRoot (source_p))
    {
      ClearServicesRoot (source_p);
    }

    if (IsProtectedOper (source_p) && !OPIsProtected (source_p))
    {
      ClearProtectedOper (source_p);
    }
  }

  send_umode_out (client_p, source_p, setflags);
  send_smode_out (client_p, source_p, old);
  send_nmode_out (client_p, source_p, old2);
  return 0;
}

/*
 * send the MODE string for user (user) to connection client_p -avalon
 */
void
send_umode (aClient * client_p,
            aClient * source_p, int old, int sendmask, char *umode_buf)
{
  int *s, flag;
  char *m;
  int what = MODE_NULL;
  /*
   * build a string in umode_buf to represent the change in the user's
   * mode between the new (source_p->flag) and 'old'.
   */
  m = umode_buf;
  *m = '\0';
  for (s = user_modes; (flag = *s); s += 2)
  {
    if (MyClient (source_p) && !(flag & sendmask))
      continue;
    if ((flag & old) && !(source_p->umode & flag))
    {
      if (what == MODE_DEL)
        *m++ = *(s + 1);
      else
      {
        what = MODE_DEL;
        *m++ = '-';
        *m++ = *(s + 1);
      }
    }
    else if (!(flag & old) && (source_p->umode & flag))
    {
      if (what == MODE_ADD)
        *m++ = *(s + 1);
      else
      {
        what = MODE_ADD;
        *m++ = '+';
        *m++ = *(s + 1);
      }
    }
  }
  *m = '\0';
  if (*umode_buf && client_p)
    sendto_one (client_p, ":%s MODE %s :%s", source_p->name, source_p->name,
                umode_buf);
}

/*
 * added Sat Jul 25 07:30:42 EST 1992
 */
/*
 * extra argument evenTS added to send to TS servers or not -orabidoo
 * 
 * extra argument evenTS no longer needed with TS only th+hybrid server
 * -Dianora
 */
void
send_umode_out (aClient * client_p, aClient * source_p, int old)
{
  aClient *target_p;
  DLink *lp;

  send_umode (NULL, source_p, old, SEND_UMODES, buf);

  if (*buf)
  {
    for (lp = server_list; lp; lp = lp->next)
    {
      target_p = lp->value.client_p;
      if ((target_p != client_p) && (target_p != source_p))
        sendto_one (target_p, ":%s MODE %s :%s", source_p->name,
                    source_p->name, buf);
    }
  }

  if (client_p && MyClient (client_p))
  {
    send_umode (client_p, source_p, old, ALL_UMODES, buf);
  }
}



int
m_smode (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int flag;
  int *s;
  char **p, *m;
  aClient *target_p;
  int what, setflags;
  what = MODE_ADD;

  /*
   * We dont allow local clients to change their own SMODE's
   */
  if (MyClient (source_p))
  {
    sendto_one (source_p, err_str (ERR_SERVERONLY), me.name, parv[0]);
    return 0;
  }

  if (parc < 2)
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS),
                me.name, parv[0], "SMODE");
    return 0;
  }

  if (!(target_p = find_person (parv[1], NULL)))
  {
    if (MyConnect (source_p))
      sendto_one (source_p, err_str (ERR_NOSUCHCHANNEL),
                  me.name, parv[0], parv[1]);
    return 0;
  }

  if ((IsServer (source_p) || (source_p != target_p)
       || (target_p->from != source_p->from)))
  {
    if (!IsServer (client_p))
      sendto_one (source_p, err_str (ERR_USERSDONTMATCH), me.name, parv[0]);
    return 0;
  }

  /*
   * find flags already set for user
   */
  setflags = 0;
  for (s = server_modes; (flag = *s); s += 2)
    if (source_p->smode & flag)
      setflags |= flag;
  /*
   * parse mode change string(s)
   */
  for (p = &parv[2]; p && *p; p++)
    for (m = *p; *m; m++)
      switch (*m)
      {
         case '+':
           what = MODE_ADD;
           break;
         case '-':
           what = MODE_DEL;
           break;
           /*
            * we may not get these, but they shouldnt be in
            * default
            */
         case ' ':
         case '\r':
         case '\n':
         case '\t':
           break;

         default:
           if ((flag = server_mode_table[(unsigned char) *m]))
           {
             if (what == MODE_ADD)
             {
               source_p->smode |= flag;
             }
             else
             {
               source_p->smode &= ~flag;
             }
           }
           break;
      }
  send_smode_out (client_p, source_p, setflags);
  return 0;
}


/*
 * send the SMODE string for user (user) to connection client_p
 */
void
send_smode (aClient * client_p,
            aClient * source_p, int old, int sendmask, char *smode_buf)
{
  int *s, flag;
  char *m;
  int what = MODE_NULL;
  /*
   * build a string in smode_buf to represent the change in the user's
   * mode between the new (source_p->flag) and 'old'.
   */
  m = smode_buf;
  *m = '\0';
  for (s = server_modes; (flag = *s); s += 2)
  {
    if (MyClient (source_p) && !(flag & sendmask))
      continue;
    if ((flag & old) && !(source_p->smode & flag))
    {
      if (what == MODE_DEL)
        *m++ = *(s + 1);
      else
      {
        what = MODE_DEL;
        *m++ = '-';
        *m++ = *(s + 1);
      }
    }
    else if (!(flag & old) && (source_p->smode & flag))
    {
      if (what == MODE_ADD)
        *m++ = *(s + 1);
      else
      {
        what = MODE_ADD;
        *m++ = '+';
        *m++ = *(s + 1);
      }
    }
  }
  *m = '\0';
  if (*smode_buf && client_p && !IsPerson (client_p))
    sendto_one (client_p, ":%s SMODE %s :%s", source_p->name, source_p->name,
                smode_buf);
}


/*
 * extra argument evenTS added to send to TS servers or not -orabidoo
 *
 * extra argument evenTS no longer needed with TS only th+hybrid server
 * -Dianora
 */
void
send_smode_out (aClient * client_p, aClient * source_p, int old)
{
  aClient *target_p;
  DLink *lp;

  send_smode (NULL, source_p, old, SEND_SMODES, buf);

  if (*buf)
  {
    for (lp = server_list; lp; lp = lp->next)
    {
      target_p = lp->value.client_p;
      if ((target_p != client_p) && (target_p != source_p))
        sendto_one (target_p, ":%s SMODE %s :%s", source_p->name,
                    source_p->name, buf);
    }
  }

  if (client_p && MyClient (client_p))
    send_smode (client_p, source_p, old, SEND_SMODES, buf);
}

int
m_nmode (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  int flag;
  int *s;
  char **p, *m;
  int what, setflags;
  what = MODE_ADD;

  if (parc < 2)
  {
    m = buf;
    *m++ = '+';
    for (s = notice_modes; (flag = *s) && (m - buf < BUFSIZE - 4); s += 2)
    {
      if (source_p->nmode & (flag & ALL_NMODES))
        *m++ = (char) (*(s + 1));
    }
    *m = '\0';
    sendto_one (source_p, rpl_str (RPL_NMODEIS), me.name, parv[0], buf);
    return 0;
  }

  /*
   * find flags already set for user
   */
  setflags = 0;
  for (s = notice_modes; (flag = *s); s += 2)
    if (source_p->nmode & flag)
      setflags |= flag;
  /*
   * parse mode change string(s)
   */
  for (p = &parv[1]; p && *p; p++)
    for (m = *p; *m; m++)
      switch (*m)
      {
         case '+':
           what = MODE_ADD;
           break;
         case '-':
           what = MODE_DEL;
           break;
           /*
            * we may not get these, but they shouldnt be in
            * default
            */
         case ' ':
         case '\r':
         case '\n':
         case '\t':
           break;
           /*
            * Modes that cannot be handeled by the default "allow everyone"
            */
         case 'b':
         case 'c':
         case 'd':
         case 'f':
         case 'g':
         case 'k':
         case 'n':
         case 'r':
         case 'B':
         case 'C':
         case 'G':
         case 'N':
         case 'R':
         case 'S':
           if (!IsAnOper (source_p))
           {
             break;
           }
           else
           {
             if ((flag = notice_mode_table[(unsigned char) *m]))
             {
               if (what == MODE_ADD)
               {
                 source_p->nmode |= flag;
               }
               else
               {
                 source_p->nmode &= ~flag;
               }
             }
           }
           break;
         default:
           if ((flag = notice_mode_table[(unsigned char) *m]))
           {
             if (what == MODE_ADD)
             {
               source_p->nmode |= flag;
             }
             else
             {
               source_p->nmode &= ~flag;
             }
           }
           break;
      }
  send_nmode_out (client_p, source_p, setflags);
  return 0;
}


/*
 * send the NMODE string for user (user) to connection client_p
 */
void
send_nmode (aClient * client_p,
            aClient * source_p, int old, int sendmask, char *nmode_buf)
{
  int *s, flag;
  char *m;
  int what = MODE_NULL;
  /*
   * build a string in nmode_buf to represent the change in the user's
   * mode between the new (source_p->flag) and 'old'.
   */
  m = nmode_buf;
  *m = '\0';
  for (s = notice_modes; (flag = *s); s += 2)
  {
    if (MyClient (source_p) && !(flag & sendmask))
      continue;
    if ((flag & old) && !(source_p->nmode & flag))
    {
      if (what == MODE_DEL)
        *m++ = *(s + 1);
      else
      {
        what = MODE_DEL;
        *m++ = '-';
        *m++ = *(s + 1);
      }
    }
    else if (!(flag & old) && (source_p->nmode & flag))
    {
      if (what == MODE_ADD)
        *m++ = *(s + 1);
      else
      {
        what = MODE_ADD;
        *m++ = '+';
        *m++ = *(s + 1);
      }
    }
  }
  *m = '\0';
  if (*nmode_buf && client_p)
    sendto_one (client_p, ":%s NMODE %s :%s", source_p->name,
                source_p->name, nmode_buf);
}


void
send_nmode_out (aClient * client_p, aClient * source_p, int old)
{
  if (client_p && MyClient (client_p))
    send_nmode (client_p, source_p, old, ALL_NMODES, buf);
}

/*
 * This function checks to see if a CTCP message (other than ACTION) is
 * contained in the passed string.  This might seem easier than I am
 * doing it, but a CTCP message can be changed together, even after a
 * normal message.
 * 
 * If the message is found, and it's a DCC message, pass it back in
 * *dcclient_p.
 *
 * Unfortunately, this makes for a bit of extra processing in the
 * server.
 */

int
check_for_ctcp (char *str, char **dcclient_p)
{
  char *p = str;
  while ((p = strchr (p, 1)) != NULL)
  {
    if (ircncmp (++p, "DCC", 3) == 0)
    {
      if (dcclient_p)
        *dcclient_p = p;
      if (ircncmp (p + 3, " SEND", 5) == 0)
        return CTCP_DCCSEND;
      else
        return CTCP_DCC;
    }
    if (ircncmp (p, "ACTION", 6) != 0)
      return CTCP_YES;
    if ((p = strchr (p, 1)) == NULL)
      return CTCP_NONE;
    if (!(*(++p)))
      break;;
  }
  return CTCP_NONE;
}

/*
 * Shadowfax's FLUD code 
 */

#ifdef FLUD

void
announce_fluder (aClient * fluder,      /*
                                         * fluder, client being fluded 
                                         */
                 aClient * client_p, aChannel * channel_p,      /*
                                                                 * channel being fluded 
                                                                 */
                 int type)
{                               /*
                                 * for future use 
                                 */
  char *fludee;
  if (client_p)
    fludee = client_p->name;
  else
    fludee = channel_p->chname;
  sendto_realops_lev (FLOOD_LEV,
                      "Flooder %s [%s@%s] on %s target: %s",
                      fluder->name, fluder->user->username,
                      fluder->user->host, fluder->user->server, fludee);
}

/*
 * This is really just a "convenience" function.  I can only keep three
 * or * four levels of pointer dereferencing straight in my head.  This
 * remove * an entry in a fluders list.  Use this when working on a
 * fludees list :)
 */
struct fludbot *
remove_fluder_reference (struct fludbot **fluders, aClient * fluder)
{
  struct fludbot *current, *prev, *next;
  prev = NULL;
  current = *fluders;
  while (current)
  {
    next = current->next;
    if (current->fluder == fluder)
    {
      if (prev)
        prev->next = next;
      else
        *fluders = next;
      BlockHeapFree (free_fludbots, current);
    }
    else
      prev = current;
    current = next;
  }

  return (*fluders);
}

/*
 * Another function to unravel my mind. 
 */
Link *
remove_fludee_reference (Link ** fludees, void *fludee)
{
  Link *current, *prev, *next;
  prev = NULL;
  current = *fludees;
  while (current)
  {
    next = current->next;
    if (current->value.client_p == (aClient *) fludee)
    {
      if (prev)
        prev->next = next;
      else
        *fludees = next;
      BlockHeapFree (free_Links, current);
    }
    else
      prev = current;
    current = next;
  }

  return (*fludees);
}

int
check_for_fludblock (aClient * fluder,  /*
                                         * fluder being fluded 
                                         */
                     aClient * client_p,        /*
                                                 * client being fluded 
                                                 */
                     aChannel * channel_p,      /*
                                                 * channel being fluded 
                                                 */
                     int type)
{                               /*
                                 * for future use 
                                 */
  time_t now;
  int blocking;
  /*
   * If it's disabled, we don't need to process all of this 
   */
  if (flud_block == 0)
    return 0;
  /*
   * It's either got to be a client or a channel being fluded 
   */
  if ((client_p == NULL) && (channel_p == NULL))
    return 0;
  if (client_p && !MyFludConnect (client_p))
  {
    sendto_ops ("check_for_fludblock() called for non-local client");
    return 0;
  }

  /*
   * Are we blocking fluds at this moment? 
   */
  time (&now);
  if (client_p)
    blocking = (client_p->fludblock > (now - flud_block));
  else
    blocking = (channel_p->fludblock > (now - flud_block));
  return (blocking);
}

int
check_for_flud (aClient * fluder,       /*
                                         * fluder, client being fluded 
                                         */
                aClient * client_p, aChannel * channel_p,       /*
                                                                 * channel being fluded 
                                                                 */
                int type)
{                               /*
                                 * for future use 
                                 */
  time_t now;
  struct fludbot *current, *prev, *next;
  int blocking, count, found;
  Link *newfludee;
  /*
   * If it's disabled, we don't need to process all of this 
   */
  if (flud_block == 0)
    return 0;
  /*
   * It's either got to be a client or a channel being fluded 
   */
  if ((client_p == NULL) && (channel_p == NULL))
    return 0;
  if (client_p && !MyFludConnect (client_p))
  {
    sendto_ops ("check_for_flud() called for non-local client");
    return 0;
  }

  /*
   * Are we blocking fluds at this moment? 
   */
  time (&now);
  if (client_p)
    blocking = (client_p->fludblock > (now - flud_block));
  else
    blocking = (channel_p->fludblock > (now - flud_block));
  /*
   * Collect the Garbage 
   */
  if (!blocking)
  {
    if (client_p)
      current = client_p->fluders;
    else
      current = channel_p->fluders;
    prev = NULL;
    while (current)
    {
      next = current->next;
      if (current->last_msg < (now - flud_time))
      {
        if (client_p)
          remove_fludee_reference (&current->fluder->fludees,
                                   (void *) client_p);
        else
          remove_fludee_reference (&current->fluder->fludees,
                                   (void *) channel_p);
        if (prev)
          prev->next = current->next;
        else if (client_p)
          client_p->fluders = current->next;
        else
          channel_p->fluders = current->next;
        BlockHeapFree (free_fludbots, current);
      }
      else
        prev = current;
      current = next;
    }
  }
  /*
   * Find or create the structure for the fluder, and update the
   * counter * and last_msg members.  Also make a running total count
   */
  if (client_p)
    current = client_p->fluders;
  else
    current = channel_p->fluders;
  count = found = 0;
  while (current)
  {
    if (current->fluder == fluder)
    {
      current->last_msg = now;
      current->count++;
      found = 1;
    }
    if (current->first_msg < (now - flud_time))
      count++;
    else
      count += current->count;
    current = current->next;
  }
  if (!found)
  {
    if ((current = BlockHeapALLOC (free_fludbots, struct fludbot)) != NULL)
    {
      current->fluder = fluder;
      current->count = 1;
      current->first_msg = now;
      current->last_msg = now;
      if (client_p)
      {
        current->next = client_p->fluders;
        client_p->fluders = current;
      }
      else
      {
        current->next = channel_p->fluders;
        channel_p->fluders = current;
      }

      count++;
      if ((newfludee = BlockHeapALLOC (free_Links, Link)) != NULL)
      {
        if (client_p)
        {
          newfludee->flags = 0;
          newfludee->value.client_p = client_p;
        }
        else
        {
          newfludee->flags = 1;
          newfludee->value.channel_p = channel_p;
        }
        newfludee->next = fluder->fludees;
        fluder->fludees = newfludee;
      }
      else
        outofmemory ();
      /*
       * If we are already blocking now, we should go ahead * and
       * announce the new arrival
       */
      if (blocking)
        announce_fluder (fluder, client_p, channel_p, type);
    }
    else
      outofmemory ();
  }
  /*
   * Okay, if we are not blocking, we need to decide if it's time to *
   * begin doing so.  We already have a count of messages received in *
   * the last flud_time seconds
   */
  if (!blocking && (count > flud_num))
  {
    blocking = 1;
    ircstp->is_flud++;
    /*
     * if we are going to say anything to the fludee, now is the *
     * time to mention it to them.
     */
    if (client_p)
      sendto_one (client_p,
                  ":%s NOTICE %s :*** Notice -- Server flood protection activated for %s",
                  me.name, client_p->name, client_p->name);
    else
      sendto_channel_butserv (channel_p, &me,
                              ":%s NOTICE %s :*** Notice -- Server flood protection activated for %s",
                              me.name, channel_p->chname, channel_p->chname);
    /*
     * Here we should go back through the existing list of * fluders
     * and announce that they were part of the game as * well.
     */
    if (client_p)
      current = client_p->fluders;
    else
      current = channel_p->fluders;
    while (current)
    {
      announce_fluder (current->fluder, client_p, channel_p, type);
      current = current->next;
    }
  }
  /*
   * update blocking timestamp, since we received a/another CTCP
   * message
   */
  if (blocking)
  {
    if (client_p)
      client_p->fludblock = now;
    else
      channel_p->fludblock = now;
  }

  return (blocking);
}

void
free_fluders (aClient * client_p, aChannel * channel_p)
{
  struct fludbot *fluders, *next;
  if ((client_p == NULL) && (channel_p == NULL))
  {
    sendto_ops ("free_fluders(NULL, NULL)");
    return;
  }

  if (client_p && !MyFludConnect (client_p))
    return;
  if (client_p)
    fluders = client_p->fluders;
  else
    fluders = channel_p->fluders;
  while (fluders)
  {
    next = fluders->next;
    if (client_p)
      remove_fludee_reference (&fluders->fluder->fludees, (void *) client_p);
    else
      remove_fludee_reference (&fluders->fluder->fludees, (void *) channel_p);
    BlockHeapFree (free_fludbots, fluders);
    fluders = next;
  }
}

void
free_fludees (aClient * badguy)
{
  Link *fludees, *next;
  if (badguy == NULL)
  {
    sendto_ops ("free_fludees(NULL)");
    return;
  }
  fludees = badguy->fludees;
  while (fludees)
  {
    next = fludees->next;
    if (fludees->flags)
      remove_fluder_reference (&fludees->value.channel_p->fluders, badguy);
    else
    {
      if (!MyFludConnect (fludees->value.client_p))
        sendto_ops ("free_fludees() encountered non-local client");
      else
        remove_fluder_reference (&fludees->value.client_p->fluders, badguy);
    }

    BlockHeapFree (free_Links, fludees);
    fludees = next;
  }
}
#endif /*
        * FLUD 
        */


/* is_silenced - Returns 1 if a source_p is silenced by target_p */
static int
is_silenced (aClient * source_p, aClient * target_p)
{
  Link *lp;
  anUser *user;
  char sender[HOSTLEN + NICKLEN + USERLEN + 5];
  if (!(target_p->user) || !(lp = target_p->user->silence)
      || !(user = source_p->user))
  {
    return 0;
  }
  ircsprintf (sender, "%s!%s@%s", source_p->name, user->username, user->host);
  for (; lp; lp = lp->next)
  {
    if (!match (lp->value.cp, sender))
    {
      if (!MyConnect (source_p))
      {
        sendto_one (source_p->from, ":%s SILENCE %s :%s",
                    target_p->name, source_p->name, lp->value.cp);
        lp->flags = 1;
      }
      return 1;
    }
  }
  /*
   * If the source has a hiddenhost.. we need to check against the hiddenhost aswell.
   */
  if (IsHidden (source_p))
  {
    ircsprintf (sender, "%s!%s@%s", source_p->name, user->username,
                user->virthost);
    for (; lp; lp = lp->next)
    {
      if (!match (lp->value.cp, sender))
      {
        if (!MyConnect (source_p))
        {
          sendto_one (source_p->from, ":%s SILENCE %s :%s",
                      target_p->name, source_p->name, lp->value.cp);
          lp->flags = 1;
        }
        return 1;
      }
    }
  }

  return 0;
}

int
del_silence (aClient * source_p, char *mask)
{
  Link **lp, *tmp;
  for (lp = &(source_p->user->silence); *lp; lp = &((*lp)->next))
    if (irccmp (mask, (*lp)->value.cp) == 0)
    {
      tmp = *lp;
      *lp = tmp->next;
      MyFree (tmp->value.cp);
      free_link (tmp);
      return 0;
    }
  return 1;
}

static int
add_silence (aClient * source_p, char *mask)
{
  Link *lp;
  int cnt = 0, len = 0;
  for (lp = source_p->user->silence; lp; lp = lp->next)
  {
    len += strlen (lp->value.cp);
    if (MyClient (source_p))
    {
      if ((len > MAXSILELENGTH) || (++cnt >= MAXSILES))
      {
        sendto_one (source_p, err_str (ERR_SILELISTFULL), me.name,
                    source_p->name, mask);
        return -1;
      }
      else
      {
        if (!match (lp->value.cp, mask))
          return -1;
      }
    }
    else if (irccmp (lp->value.cp, mask) == 0)
      return -1;
  }
  lp = make_link ();
  lp->next = source_p->user->silence;
  lp->value.cp = (char *) MyMalloc (strlen (mask) + 1);
  strcpy (lp->value.cp, mask);
  source_p->user->silence = lp;
  return 0;
}

/* m_silence
 * parv[0] = sender prefix
 * From local client:
 * parv[1] = mask (NULL sends the list)
 * From remote client:
 * parv[1] = nick that must be silenced
 * parv[2] = mask
 */
int
m_silence (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  Link *lp;
  aClient *target_p = NULL;
  char c, *cp;
  if (check_registered_user (source_p))
    return 0;
  if (MyClient (source_p))
  {
    target_p = source_p;
    if (parc < 2 || *parv[1] == '\0'
        || (target_p = find_person (parv[1], NULL)))
    {
      if (!(target_p->user))
        return 0;
      for (lp = target_p->user->silence; lp; lp = lp->next)
        sendto_one (source_p, rpl_str (RPL_SILELIST), me.name,
                    source_p->name, target_p->name, lp->value.cp);
      sendto_one (source_p, rpl_str (RPL_ENDOFSILELIST), me.name,
                  target_p->name);
      return 0;
    }
    cp = parv[1];
    c = *cp;
    if (c == '-' || c == '+')
      cp++;
    else
      if (!(strchr (cp, '@') || strchr (cp, '.') ||
            strchr (cp, '!') || strchr (cp, '*')))
    {
      sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0],
                  parv[1]);
      return 0;
    }
    else
      c = '+';
    cp = pretty_mask (cp);
    if ((c == '-' && !del_silence (source_p, cp)) ||
        (c != '-' && !add_silence (source_p, cp)))
    {
      sendto_prefix_one (source_p, source_p, ":%s SILENCE %c%s", parv[0],
                         c, cp);
      if (c == '-')
        sendto_serv_butone (NULL, ":%s SILENCE * -%s", source_p->name, cp);
    }
  }
  else if (parc < 3 || *parv[2] == '\0')
  {
    sendto_one (source_p, err_str (ERR_NEEDMOREPARAMS), me.name, parv[0],
                "SILENCE");
    return -1;
  }
  else if ((c = *parv[2]) == '-' || (target_p = find_person (parv[1], NULL)))
  {
    if (c == '-')
    {
      if (!del_silence (source_p, parv[2] + 1))
        sendto_serv_butone (client_p, ":%s SILENCE %s :%s",
                            parv[0], parv[1], parv[2]);
    }
    else
    {
      add_silence (source_p, parv[2]);
      if (!MyClient (target_p))
        sendto_one (target_p, ":%s SILENCE %s :%s", parv[0], parv[1],
                    parv[2]);
    }
  }
  else
  {
    sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name, parv[0],
                parv[1]);
    return 0;
  }
  return 0;
}

int
add_dccallow (aClient * source_p, aClient * optr)
{
  Link *lp;
  int cnt = 0;
  for (lp = source_p->user->dccallow; lp; lp = lp->next)
  {
    if (lp->flags != DCC_LINK_ME)
      continue;
    if (++cnt >= MAXDCCALLOW)
    {
      sendto_one (source_p, err_str (ERR_TOOMANYDCC), me.name,
                  source_p->name, optr->name, MAXDCCALLOW);
      return 0;
    }
    else if (lp->value.client_p == optr)
    {
      /* silently return */
      return 0;
    }
  }

  lp = make_link ();
  lp->value.client_p = optr;
  lp->flags = DCC_LINK_ME;
  lp->next = source_p->user->dccallow;
  source_p->user->dccallow = lp;
  lp = make_link ();
  lp->value.client_p = source_p;
  lp->flags = DCC_LINK_REMOTE;
  lp->next = optr->user->dccallow;
  optr->user->dccallow = lp;
  sendto_one (source_p, rpl_str (RPL_DCCSTATUS), me.name, source_p->name,
              optr->name, "added to");
  return 0;
}

int
del_dccallow (aClient * source_p, aClient * optr)
{
  Link **lpp, *lp;
  int found = 0;
  for (lpp = &(source_p->user->dccallow); *lpp; lpp = &((*lpp)->next))
  {
    if ((*lpp)->flags != DCC_LINK_ME)
      continue;
    if ((*lpp)->value.client_p == optr)
    {
      lp = *lpp;
      *lpp = lp->next;
      free_link (lp);
      found++;
      break;
    }
  }

  if (!found)
  {
    sendto_one (source_p, ":%s %d %s :%s is not in your DCC allow list",
                me.name, RPL_DCCINFO, source_p->name, optr->name);
    return 0;
  }

  for (found = 0, lpp = &(optr->user->dccallow); *lpp; lpp = &((*lpp)->next))
  {
    if ((*lpp)->flags != DCC_LINK_REMOTE)
      continue;
    if ((*lpp)->value.client_p == source_p)
    {
      lp = *lpp;
      *lpp = lp->next;
      free_link (lp);
      found++;
      break;
    }
  }

  if (!found)
    sendto_realops_lev (DEBUG_LEV,
                        "%s was in dccallowme list of %s but not in dccallowrem list!",
                        optr->name, source_p->name);
  sendto_one (source_p, rpl_str (RPL_DCCSTATUS), me.name, source_p->name,
              optr->name, "removed from");
  return 0;
}

int
allow_dcc (aClient * to, aClient * from)
{
  Link *lp;
  for (lp = to->user->dccallow; lp; lp = lp->next)
  {
    if (lp->flags == DCC_LINK_ME && lp->value.client_p == from)
      return 1;
  }
  return 0;
}

int
m_dccallow (aClient * client_p, aClient * source_p, int parc, char *parv[])
{
  Link *lp;
  char *p, *s;
  char *cn;
  aClient *target_p, *lastclient_p = NULL;
  int didlist = 0, didhelp = 0, didanything = 0;
  char **ptr;
  static char *dcc_help[] = {
    "/DCCALLOW [<+|->nick[,<+|->nick, ...]] [list] [help]",
    "You may allow DCCs of filetypes which are otherwise blocked by the IRC server",
    "by specifying a DCC allow for the user you want to recieve files from.",
    "For instance, to allow the user bob to send you file.exe, you would type:",
    "/dccallow +bob",
    "and bob would then be able to send you files. bob will have to resend the file",
    "if the server gave him an error message before you added him to your allow list.",
    "/dccallow -bob",
    "Will do the exact opposite, removing him from your dcc allow list.",
    "/dccallow list",
    "Will list the users currently on your dcc allow list.", NULL
  };
  if (!MyClient (source_p))
    return 0;                   /* don't accept dccallows from servers or clients that aren't mine.. */
  if (parc < 2)
  {
    sendto_one (source_p,
                ":%s NOTICE %s :No command specified for DCCALLOW. Type /dccallow help for more information.",
                me.name, source_p->name);
    return 0;
  }

  for (p = NULL, s = strtoken (&p, parv[1], ", "); s;
       s = strtoken (&p, NULL, ", "))
  {
    if (*s == '+')
    {
      didanything++;
      cn = s + 1;
      if (*cn == '\0')
        continue;
      target_p = find_person (cn, NULL);
      if (target_p == source_p)
        continue;
      if (!target_p)
      {
        sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name,
                    source_p->name, cn);
        continue;
      }

      if (lastclient_p == target_p)
        sendto_realops_lev (SPY_LEV,
                            "User %s (%s@%s) may be flooding dccallow: add %s",
                            source_p->name, source_p->user->username,
                            source_p->user->host, target_p->name);
      lastclient_p = target_p;
      add_dccallow (source_p, target_p);
    }
    else if (*s == '-')
    {
      didanything++;
      cn = s + 1;
      if (*cn == '\0')
        continue;
      target_p = find_person (cn, NULL);
      if (target_p == source_p)
        continue;
      if (!target_p)
      {
        sendto_one (source_p, err_str (ERR_NOSUCHNICK), me.name,
                    source_p->name, cn);
        continue;
      }

      if (lastclient_p == target_p)
        sendto_realops_lev (SPY_LEV,
                            "User %s (%s@%s) may be flooding dccallow: del %s",
                            source_p->name, source_p->user->username,
                            source_p->user->host, target_p->name);
      lastclient_p = target_p;
      del_dccallow (source_p, target_p);
    }
    else
    {
      if (!didlist && ircncmp (s, "list", 4) == 0)
      {
        didanything++;
        didlist++;
        sendto_one (source_p,
                    ":%s %d %s :The following users are on your dcc allow list:",
                    me.name, RPL_DCCINFO, source_p->name);
        for (lp = source_p->user->dccallow; lp; lp = lp->next)
        {
          if (lp->flags == DCC_LINK_REMOTE)
            continue;
          sendto_one (source_p, ":%s %d %s :%s (%s@%s)", me.name,
                      RPL_DCCLIST, source_p->name,
                      lp->value.client_p->name,
                      lp->value.client_p->user->username,
                      IsHidden (client_p) ? lp->value.client_p->user->
                      virthost : lp->value.client_p->user->host);
        }
        sendto_one (source_p, rpl_str (RPL_ENDOFDCCLIST), me.name,
                    source_p->name, s);
      }
      else if (!didhelp && ircncmp (s, "help", 4) == 0)
      {
        didanything++;
        didhelp++;
        for (ptr = dcc_help; *ptr; ptr++)
          sendto_one (source_p, ":%s %d %s :%s", me.name, RPL_DCCINFO,
                      source_p->name, *ptr);
        sendto_one (source_p, rpl_str (RPL_ENDOFDCCLIST), me.name,
                    source_p->name, s);
      }
    }
  }

  if (!didanything)
  {
    sendto_one (source_p,
                ":%s NOTICE %s :Invalid syntax for DCCALLOW. Type /dccallow help for more information.",
                me.name, source_p->name);
    return 0;
  }

  return 0;
}


void
set_oper_access (aClient * source_p, aConfItem * aconf)
{
  char temp[128];
  int islocal = 0;
  int level = 0;

  /* First, is our would-be oper, a global or a local, because that makes all the difference */
  if (!(aconf->port & OFLAG_ISGLOBAL))
  {
    SetLocOp (source_p);
    islocal = 1;
    level = 1;
    ircsnprintf (temp, 128, "is now a Local Operator (o)");
  }
  else if ((aconf->port & OFLAG_ISGLOBAL) && (islocal == 0))
  {
    /* They have a global O and they DON'T have a small 'o' */
    SetOper (source_p);
    level = 2;
    ircsnprintf (temp, 128, "is now an IRC Operator (O)");
  }

  /* Okay, they have the basic oper-access, now check what they have in terms of levels of access */
  if (islocal == 0)
  {
    /* Skip this if they're a local, we don't need to check the rest of it */
    if (aconf->port & OFLAG_NETADMIN)
    {
      SetNetAdmin (source_p);
      level = 9;
      ircsnprintf (temp, 128, "is now a Network Administrator (N)");
    }
    else if (aconf->port & OFLAG_NETCOADMIN)
    {
      SetNetCoAdmin (source_p);
      level = 8;
      ircsnprintf (temp, 128, "is now a Network Co Administrator (n)");
    }
    else if (aconf->port & OFLAG_TECHADMIN)
    {
      SetTechAdmin (source_p);
      level = 7;
      ircsnprintf (temp, 128, "is now a Technical Administrator (T)");
    }
    else if (aconf->port & OFLAG_TECHCOADMIN)
    {
      SetTechCoAdmin (source_p);
      level = 6;
      ircsnprintf (temp, 128, "is now a Technical Co Administrator (t)");
    }
    else if (aconf->port & OFLAG_SERVADMIN)
    {
      SetServerAdmin (source_p);
      level = 5;
      ircsnprintf (temp, 128, "is now a Server Administrator (A)");
    }
    else if (aconf->port & OFLAG_SERVCOADMIN)
    {
      SetServerCoAdmin (source_p);
      level = 4;
      ircsnprintf (temp, 128, "is now a Server Co Administrator (J)");
    }
    else if (aconf->port & OFLAG_GUESTADMIN)
    {
      SetGuestAdmin (source_p);
      level = 3;
      ircsnprintf (temp, 128, "is now a Guest Administrator (j)");
    }
  }                             /* islocal == 0 */

  /* We have our levels in place and parts of the oper-notice in place, set any modes needed */

  if ((OPER_AUTOPROTECT == 1) && (aconf->port & OFLAG_PROTECTEDOPER))
  {
    SetProtectedOper (source_p);
  }

#ifdef DEFAULT_HELP_MODE
  if (aconf->port & OFLAG_HELPOP)
  {
    SetHelpOp (source_p);
  }
#endif

  /*
   * Set default notice modes
   */
  SetChatops (source_p);
  SetServerNotice (source_p);
  SetWallops (source_p);
  SetFloodNotice (source_p);
  SetNetInfo (source_p);
  SetNetGlobal (source_p);
  SetRoutingNotice (source_p);
  SetGlobops (source_p);
  SetSpamNotice (source_p);
  SetWantsWhois (source_p);

  /* Lastly, the hosts and then the notices */

  make_opervirthost_acc (source_p->user->virthost, level);

  /*
   * Propogate the hiddenhost
   */
  sendto_serv_butone (source_p, ":%s SETHOST %s %s", me.name,
                      source_p->name, source_p->user->virthost);

  /*
   * Send out Network Global notices
   */
  sendto_serv_butone (source_p,
                      ":%s NETGLOBAL :%s (%s@%s) [%s] %s",
                      me.name, source_p->name, source_p->user->username,
                      source_p->user->host, aconf->name, temp);
  sendto_netglobal
    ("from %s: %s (%s@%s) [%s] %s", me.name,
     source_p->name, source_p->user->username, source_p->user->host,
     aconf->name, temp);
  ilog (LOGF_OPER, "%s (%s@%s) [%s] %s",
        source_p->name, source_p->user->username, source_p->user->host,
        aconf->name, temp);

  /*
   * Do we really need to tell non opers about things like this?
   * Do we even need +s anymore at all? - ShadowMaster
   */
  sendto_ops ("%s (%s@%s) %s ", source_p->name,
              source_p->user->username, source_p->user->virthost, temp);
}
