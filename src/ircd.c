/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, src/ircd.c
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
 *  $Id: ircd.c 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "config.h"
#ifndef _WIN32
# include <sys/file.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <pwd.h>
# include <signal.h>
#else
# include <winsock2.h>
#endif
#include <fcntl.h>
#ifdef PROFILING
#include <sys/gmon.h>
#define monstartup __monstartup
#endif
#include "inet.h"
#include "h.h"
#include "version.h"
#include "dh.h"
#include "dynconf.h"
#include "sock.h"
#include "res.h"
#include "dich_conf.h"
#include "throttle.h"
#include "channel.h"
#include "userban.h"
#include "blalloc.h"
#include "fds.h"
#include "send.h"
#include "sock.h"
#include "res.h"


/*
** Allows for backtrace dumps to file on most glibc systems. Disabled by default - ShadowMaster
*/

#undef BACKTRACE_DEBUG

int booted = FALSE;



aConfList EList1 = { 0, NULL }; /* ordered */
aConfList EList2 = { 0, NULL }; /* ordered, reversed */
aConfList EList3 = { 0, NULL }; /* what we can't sort */

aConfList FList1 = { 0, NULL }; /* ordered */
aConfList FList2 = { 0, NULL }; /* ordered, reversed */
aConfList FList3 = { 0, NULL }; /* what we can't sort */

aMotd *motd;
aRules *rules;
aOMotd *omotd;

aMotd *helpfile;                /* misnomer, aMotd could be generalized */

aMotd *shortmotd;               /* short motd */

int find_fline (aClient *);

struct tm *motd_tm;
struct tm *rules_tm;
struct tm *omotd_tm;
struct tm log_tm;

/* this stuff by mnystrom@mit.edu */
#include "fdlist.h"

fdlist default_fdlist;          /* just the number of the entry */

int MAXCLIENTS = MAX_CLIENTS;   /* semi-configurable if
                                 *  QUOTE_SET is def  */
struct Counter Count;
int R_do_dns, R_fin_dns, R_fin_dnsc, R_fail_dns, R_do_id, R_fin_id, R_fail_id;

time_t NOW;
time_t last_stat_save;
aClient me;                     /* That's me */
aClient *client = &me;          /* Pointer to beginning of Client list */

float curSendK = 0, curRecvK = 0;

#ifdef  LOCKFILE
extern time_t pending_kline_time;
extern struct pkl *pending_klines;
extern void do_pending_klines (void);
#endif

void server_reboot ();
void restart (char *);
void s_restart ();
void s_die ();
void do_recvqs ();
void send_safelists ();
static inline char *log_head (int);
void log_open (aLogfile *, struct tm *);
void log_cycle ();
#if defined(_WIN32) && defined(W32_STATUSCONSOLE)
void win32_updateconsole (void);
#endif

static void open_debugfile (), setup_signals ();
static void io_loop ();

/* externally needed functions */

extern void init_fdlist (fdlist *);     /* defined in fdlist.c */
extern void dbuf_init ();       /* defined in dbuf.c */
extern void read_motd (char *); /* defined in s_serv.c */
extern void read_rules (char *);        /* defined in s_serv.c */
extern void read_opermotd (char *);     /* defined in s_serv.c */
extern void read_shortmotd (char *);    /* defined in s_serv.c */

void ilog (int, const char *, ...);

char **myargv;
int portnum = -1;               /* Server port number, listening this */
char *configfile = CONFIGFILE;  /* Server configuration file */
#ifdef KPATH
char *klinefile = KLINEFILE;    /* Server kline file */
# ifdef ZLINES_IN_KPATH
char *zlinefile = KLINEFILE;
# else
char *zlinefile = CONFIGFILE;
# endif
#else
char *klinefile = CONFIGFILE;
char *zlinefile = CONFIGFILE;
#endif

#ifdef VPATH
char *vlinefile = VLINEFILE;
#else
char *vlinefile = CONFIGFILE;
#endif

int debuglevel = -1;            /* Server debug level */
int bootopt = 0;                /* Server boot option flags */
char *debugmode = "";           /* -"-    -"-   -"-  */
char *sbrk0;                    /* initial sbrk(0) */
static int dorehash = 0;
static char *etcpath = ETCPATH;
int rehashed = 1;
int zline_in_progress = 0;      /* killing off matching D lines */
int noisy_htm = NOISY_HTM;      /* Is high traffic mode noisy or not? */
time_t nextconnect = 1;         /* time for next try_connections call */
time_t nextping = 1;            /* same as above for check_pings() */
time_t nextdnscheck = 0;        /* next time to poll dns to force timeout */
time_t nextexpire = 1;          /* next expire run on the dns cache */
time_t nextbanexpire = 1;       /* next time to expire the throttles/userbans */
time_t nextlogcycle = 0;

aLogfile iLogfiles[] = {
  {LOGF_CRITICAL, "ircd", NULL},
  {LOGF_CLIENTS, "clients", NULL},
  {LOGF_ROUTING, "routing", NULL},
  {LOGF_OPER, "oper", NULL},
  {LOGF_OPERACT, "operact", NULL},
  {LOGF_KILLS, "kills", NULL},
  {LOGF_OPCHAT, "opchat", NULL},
  {LOGF_AUTOKILL, "autokill", NULL},
  {LOGF_STATS, "stats", NULL},
  {LOGF_DEBUG, "debug", NULL},
  {0, NULL, NULL}
};

#ifdef PROFILING
extern void _start, etext;

static int profiling_state = 1;
static int profiling_newmsg = 0;
static char profiling_msg[512];

void
s_dumpprof ()
{
  char buf[32];

  sprintf (buf, "gmon.%d", (int) time (NULL));
  setenv ("GMON_OUT_PREFIX", buf, 1);
  _mcleanup ();
  monstartup ((u_long) & _start, (u_long) & etext);
  setenv ("GMON_OUT_PREFIX", "gmon.auto", 1);
  sprintf (profiling_msg, "Reset profile, saved past profile data to %s",
           buf);
  profiling_newmsg = 1;
}

void
s_toggleprof ()
{
  char buf[32];

  if (profiling_state == 1)
  {
    sprintf (buf, "gmon.%d", (int) time (NULL));
    setenv ("GMON_OUT_PREFIX", buf, 1);
    _mcleanup ();
    sprintf (profiling_msg,
             "Turned profiling OFF, saved profile data to %s", buf);
    profiling_state = 0;
  }
  else
  {
    __monstartup ((u_long) & _start, (u_long) & etext);
    setenv ("GMON_OUT_PREFIX", "gmon.auto", 1);
    profiling_state = 1;
    sprintf (profiling_msg, "Turned profiling ON");
  }
  profiling_newmsg = 1;
}
#endif

void
s_die ()
{
#ifdef SAVE_MAXCLIENT_STATS
  FILE *fp;
#endif
  dump_connections (me.fd);
#ifdef	USE_SYSLOG
  (void) syslog (LOG_CRIT, "Server killed By SIGTERM");
#endif
#ifdef SAVE_MAXCLIENT_STATS
  fp = fopen (ETCPATH "/.maxclients", "w");
  if (fp != NULL)
  {
    fprintf (fp, "%d %d %li %li %li %ld %ld %ld %ld", Count.max_loc,
             Count.max_tot, Count.weekly, Count.monthly,
             Count.yearly, Count.start, Count.week, Count.month, Count.year);
    fclose (fp);
  }
#endif
  exit (0);
}

#ifndef _WIN32
static void
s_rehash ()
{
  struct sigaction act;
  dorehash = 1;
  act.sa_handler = s_rehash;
  act.sa_flags = 0;
  (void) sigemptyset (&act.sa_mask);
  (void) sigaddset (&act.sa_mask, SIGHUP);
  (void) sigaction (SIGHUP, &act, NULL);
}
#endif

void
restart (char *mesg)
{
  static int was_here = NO;     /* redundant due to restarting flag below */
  if (was_here)
    abort ();
  was_here = YES;

#ifdef	USE_SYSLOG

  (void) syslog (LOG_WARNING,
                 "Restarting Server because: %s, sbrk(0)-etext: %d", mesg,
                 (u_int) sbrk ((size_t) 0) - (u_int) sbrk0);
#endif
  ilog (LOGF_CRITICAL, "Server restarting: %s", mesg);
  server_reboot ();
}

void
s_restart ()
{
  static int restarting = 0;

#ifdef	USE_SYSLOG
  (void) syslog (LOG_WARNING, "Server Restarting on SIGINT");
#endif
  if (restarting == 0)
  {
    /* Send (or attempt to) a dying scream to oper if present */
    restarting = 1;
    server_reboot ();
  }
}


void
server_reboot ()
{
  int i;
  sendto_ops ("Aieeeee!!!  Restarting server..."
#ifndef _WIN32
              " sbrk(0)-etext: %d", (u_int) sbrk ((size_t) 0) - (u_int) sbrk0
#endif
    );

  ilog (LOGF_CRITICAL, "Restarting server");

  Debug ((DEBUG_NOTICE, "Restarting server..."));
  dump_connections (me.fd);
  /*
   * fd 0 must be 'preserved' if either the -d or -i options have
   * been passed to us before restarting.
   */
#ifdef USE_SYSLOG
  (void) closelog ();
#endif
  for (i = 3; i < MAXCONNECTIONS; i++)
    (void) close (i);

  if (!(bootopt & (BOOT_TTY | BOOT_DEBUG)))
    (void) close (2);

  (void) close (1);

  if ((bootopt & BOOT_CONSOLE) || isatty (0))
    (void) close (0);

  if (!(bootopt & (BOOT_INETD | BOOT_OPER)))
    (void) execv (MYNAME, myargv);

#ifdef USE_SYSLOG
  /* Have to reopen since it has been closed above */
  openlog (myargv[0], LOG_PID | LOG_NDELAY, LOG_FACILITY);
  syslog (LOG_CRIT, "execv(%s,%s) failed: %m\n", MYNAME, myargv[0]);
  closelog ();
#endif

  Debug ((DEBUG_FATAL, "Couldn't restart server: %s", strerror (errno)));
  exit (-1);
}

/*
 * try_connections
 *
 *      Scan through configuration and try new connections. 
 *   Returns  the calendar time when the next call to this
 *      function should be made latest. (No harm done if this 
 *      is called earlier or later...)
 */
static time_t
try_connections (time_t currenttime)
{
  aConfItem *aconf, **pconf, *con_conf = (aConfItem *) NULL;
  aClient *client_p;
  aClass *cltmp;
  int connecting, confrq, con_class = 0;
  time_t next = 0;

  connecting = FALSE;

  Debug ((DEBUG_NOTICE, "Connection check at   : %s", myctime (currenttime)));

  for (aconf = conf; aconf; aconf = aconf->next)
  {
    /* Also when already connecting! (update holdtimes) --SRB */
    if (!(aconf->status & CONF_CONNECT_SERVER) || aconf->port <= 0)
      continue;
    cltmp = Class (aconf);

    /*
     * * Skip this entry if the use of it is still on hold until 
     * future. Otherwise handle this entry (and set it on hold 
     * until next time). Will reset only hold times, if already 
     * made one successfull connection... [this algorithm is a bit
     * fuzzy... -- msa >;) ]
     */

    if ((aconf->hold > currenttime))
    {
      if ((next > aconf->hold) || (next == 0))
        next = aconf->hold;
      continue;
    }

    confrq = get_con_freq (cltmp);
    aconf->hold = currenttime + confrq;

    /* Found a CONNECT config with port specified, scan clients
     * and see if this server is already connected?
     */

    client_p = find_name (aconf->name, (aClient *) NULL);

    if (!client_p && (Links (cltmp) < MaxLinks (cltmp)) &&
        (!connecting || (Class (cltmp) > con_class)))
    {
      con_class = Class (cltmp);

      con_conf = aconf;
      /* We connect only one at time... */
      connecting = TRUE;
    }

    if ((next > aconf->hold) || (next == 0))
      next = aconf->hold;
  }

  if (connecting)
  {
    if (con_conf->next)         /* are we already last? */
    {
      for (pconf = &conf; (aconf = *pconf); pconf = &(aconf->next))
        /*
         * put the current one at the end and make sure we try all
         * connections
         */
        if (aconf == con_conf)
          *pconf = aconf->next;
      (*pconf = con_conf)->next = 0;
    }
#ifdef USE_ADNS
    if (connect_server (con_conf, (aClient *) NULL) == 0)
#else
    if (connect_server (con_conf, (aClient *) NULL, (struct hostent *) NULL)
        == 0)
#endif
      sendto_gnotice ("from %s: Connection to %s activated.", me.name,
                      con_conf->name);
  }
  Debug ((DEBUG_NOTICE, "Next connection check : %s", myctime (next)));
  return (next);
}

/* dianora's code in the new checkpings is slightly wasteful.
 * however, upon inspection (thanks seddy), when we close a connection,
 * the ordering of local[i] is NOT reordered; simply local[highest_fd] becomes
 * local[i], so we can just i--;  - lucas
 */

static time_t
check_pings (time_t currenttime)
{
  aClient *client_p;
  int ping = 0, i;
  time_t oldest = 0;            /* timeout removed, see EXPLANATION below */
  char fbuf[512], *errtxt = "No response from %s, closing link";
  char ping_time_out_buffer[64];


  for (i = 0; i <= highest_fd; i++)
  {
    if (!(client_p = local[i]) || IsMe (client_p) || IsLog (client_p))
      continue;

    /* Note: No need to notify opers here. It's
     * already done when "FLAGS_DEADSOCKET" is set.
     */

    if (client_p->flags & FLAGS_DEADSOCKET)
    {
      (void) exit_client (client_p, client_p, &me,
                          (client_p->
                           flags & FLAGS_SENDQEX) ? "SendQ exceeded" :
                          "Dead socket");
      i--;
      continue;
    }

    if (IsRegistered (client_p))
      ping = client_p->pingval;
    else
      ping = CONNECTTIMEOUT;

    /*
     * Ok, so goto's are ugly and can be avoided here but this code
     * is already indented enough so I think its justified. -avalon
     *
     * justified by what? laziness? <g>
     * If the client pingtime is fine (ie, not larger than the client ping)
     * skip over all the checks below. - lucas
     */

    if (ping < (currenttime - client_p->lasttime))
    {
      /*
       * If the server hasnt talked to us in 2*ping seconds and it has
       * a ping time, then close its connection. If the client is a
       * user and a KILL line was found to be active, close this
       * connection too.
       */
      if (((client_p->flags & FLAGS_PINGSENT)
           && ((currenttime - client_p->lasttime) >= (2 * ping)))
          ||
          ((!IsRegistered (client_p)
            && (currenttime - client_p->since) >= ping)))
      {
        if (!IsRegistered (client_p)
            && (DoingDNS (client_p) || DoingAuth (client_p)))
        {
          if (client_p->authfd >= 0)
          {
            del_fd (client_p->authfd);
#ifdef _WIN32
            closesocket (client_p->authfd);
#else
            close (client_p->authfd);
#endif
            client_p->authfd = -1;
            client_p->count = 0;
            *client_p->buffer = '\0';
          }
#ifdef SHOW_HEADERS
#ifdef USE_SSL
          if (!IsSSL (client_p))
          {
#endif
            if (DoingDNS (client_p))
              sendto_one (client_p, REPORT_FAIL_DNS);
            if (DoingAuth (client_p))
              sendto_one (client_p, REPORT_FAIL_ID);
#ifdef USE_SSL
          }
#endif

#endif
          Debug ((DEBUG_NOTICE, "DNS/AUTH timeout %s",
                  get_client_name (client_p, TRUE)));
#ifdef USE_ADNS
          delete_adns_queries (client_p->dnslookup);
#else
          del_queries ((char *) client_p);
#endif

          ClearAuth (client_p);
          ClearDNS (client_p);

          check_client_fd (client_p);
          client_p->since = currenttime;
          continue;
        }

        if (IsServer (client_p) || IsConnecting (client_p)
            || IsHandshake (client_p))
        {
          ircsprintf (fbuf, "from %s: %s", me.name, errtxt);
          sendto_gnotice (fbuf, get_client_name (client_p, HIDEME));
          ircsprintf (fbuf, ":%s GNOTICE :%s", me.name, errtxt);
          sendto_serv_butone (client_p, fbuf,
                              get_client_name (client_p, HIDEME));
        }

        (void) ircsprintf (ping_time_out_buffer,
                           "Ping timeout: %d seconds",
                           currenttime - client_p->lasttime);

        (void) exit_client (client_p, client_p, &me, ping_time_out_buffer);
        i--;                    /* subtract out this fd so we check it again.. */
        continue;
      }                         /* don't send pings during a burst, as we send them already. */

      else if (!(client_p->flags & (FLAGS_PINGSENT | FLAGS_BURST)) &&
               !(IsConnecting (client_p) || IsHandshake (client_p)))
      {
        /*
         * if we havent PINGed the connection and we havent heard from
         * it in a while, PING it to make sure it is still alive.
         */
        client_p->flags |= FLAGS_PINGSENT;
        /*
         * not nice but does the job 
         */
        client_p->lasttime = currenttime - ping;
        sendto_one (client_p, "PING :%s", me.name);
      }
    }

    /* see EXPLANATION below

     * timeout = client_p->lasttime + ping;
     * while (timeout <= currenttime)
     *  timeout += ping;
     * if (timeout < oldest || !oldest)
     *   oldest = timeout;
     */

    /*
     * Check UNKNOWN connections - if they have been in this state
     * for > 100s, close them.
     */
    if (IsUnknown (client_p))
      if (client_p->firsttime ? ((timeofday - client_p->firsttime) > 100) : 0)
        (void) exit_client (client_p, client_p, &me, "Connection Timed Out");
  }

  rehashed = 0;
  zline_in_progress = 0;

  /* EXPLANATION
   * on a server with a large volume of clients, at any given point
   * there may be a client which needs to be pinged the next second,
   * or even right away (a second may have passed while running
   * check_pings). Preserving CPU time is more important than
   * pinging clients out at exact times, IMO. Therefore, I am going to make
   * check_pings always return currenttime + 9. This means that it may take
   * a user up to 9 seconds more than pingfreq to timeout. Oh well.
   * Plus, the number is 9 to 'stagger' our check_pings calls out over
   * time, to avoid doing it and the other tasks ircd does at the same time
   * all the time (which are usually done on intervals of 5 seconds or so). 
   * - lucas
   *
   *  if (!oldest || oldest < currenttime)
   *     oldest = currenttime + PINGFREQUENCY;
   */

  oldest = currenttime + 9;

  Debug ((DEBUG_NOTICE, "Next check_ping() call at: %s, %d %d %d",
          myctime (oldest), ping, oldest, currenttime));

  return oldest;
}

/*
 * * bad_command
 *    This is called when the commandline is not acceptable.
 *    Give error message and exit without starting anything.
 */
static int
bad_command ()
{
  (void) fprintf (stderr, "Usage: ircd [OPTIONS] ... \n" "\n"
#ifdef CMDLINE_CONFIG
                  "	-f	path to config file\n"
# ifdef KPATH
                  "	-k	path to kline file\n"
# endif
#endif
                  "\n"
                  "	-a	autodie\n"
                  "	-c	dont fork\n"
                  "	-q	boot quick\n"
                  "	-d	fork\n"
                  "	-h	servername\n"
                  "	-i	inetd mode (includes autodie)\n"
                  "	-p	portnumber\n"
                  "\n"
                  "	-t	keep stderr open\n"
                  "	-s	keep stderr open (additional debuging information)\n"
                  "\n" "	-v	prints the version and exits\n" "\n"
#ifdef DEBUGMODE
                  "	-x	sets the debuglevel\n"
#endif
                  "\n" "Report bugs at http://www.shadow-realm.org/\n");

  exit (-1);
}

#ifndef TRUE
#define TRUE 1
#endif

/*
 * code added by mika nystrom (mnystrom@mit.edu)
 * this flag is used to signal globally that the server is heavily
 * loaded, something which can be taken into account when processing
 * e.g. user commands and scheduling ping checks
 * Changed by Taner Halicioglu (taner@CERF.NET)
 */

#define LOADCFREQ 5             /* every 5s */
#define LOADRECV 40             /* 40k/s    */

int lifesux = 1;
int LRV = LOADRECV;
time_t LCF = LOADCFREQ;
int currlife = 0;
int HTMLOCK = NO;

char REPORT_DO_DNS[256], REPORT_FIN_DNS[256], REPORT_FIN_DNSC[256],
  REPORT_FAIL_DNS[256], REPORT_DO_ID[256], REPORT_FIN_ID[256],
  REPORT_FAIL_ID[256];

FILE *dumpfp = NULL;

int
main (int argc, char *argv[])
{
#ifndef _WIN32
  uid_t uid, euid;
#else
  WORD wVersionRequired = MAKEWORD (2, 2);
  WSADATA wsaData;
  char ctitle[256];
#endif
  int portarg = 0, fd;
#ifdef SAVE_MAXCLIENT_STATS
  FILE *mcsfp;
#endif
#ifdef USE_SSL
  extern int ssl_capable;
#endif

  if ((timeofday = time (NULL)) == -1)
  {
    fprintf (stderr,
             "========================================================\n");
    fprintf (stderr, "===== ERROR =====\n");
    fprintf (stderr, "Clock Failure (%d)\n", errno);
    fprintf (stderr,
             "========================================================\n");
    exit (errno);
  }

  /*
   * Initialize the Blockheap allocator
   */
  initBlockHeap ();


  build_version ();
  build_umodestr ();
  mode_sort (umodestring);
  build_cmodestr ();
  mode_sort (cmodestring);

  Count.server = 1;             /* us */
  Count.oper = 0;
  Count.chan = 0;
  Count.local = 0;
  Count.total = 0;
  Count.invisi = 0;
  Count.unknown = 0;
  Count.max_loc = 0;
  Count.max_tot = 0;
  Count.today = 0;
  Count.weekly = 0;
  Count.monthly = 0;
  Count.yearly = 0;
  Count.start = NOW;
  Count.day = NOW;
  Count.week = NOW;
  Count.month = NOW;
  Count.year = NOW;

#ifdef SAVE_MAXCLIENT_STATS
  mcsfp = fopen (ETCPATH "/.maxclients", "r");
  if (mcsfp != NULL)
  {
    fscanf (mcsfp, "%d %d %li %li %li %ld %ld %ld %ld", &Count.max_loc,
            &Count.max_tot, &Count.weekly, &Count.monthly, &Count.yearly,
            &Count.start, &Count.week, &Count.month, &Count.year);
    fclose (mcsfp);
  }
#endif



  /*
   * this code by mika@cs.caltech.edu
   * it is intended to keep the ircd from being swapped out. BSD
   * swapping criteria do not match the requirements of ircd
   */

#ifdef INITIAL_DBUFS
  dbuf_init ();                 /* set up some dbuf stuff to control paging */
#endif
#ifndef _WIN32
  sbrk0 = (char *) sbrk ((size_t) 0);
  uid = getuid ();
  euid = geteuid ();
#endif
#ifdef PROFILING
  setenv ("GMON_OUT_PREFIX", "gmon.out", 1);
  (void) signal (SIGUSR1, s_dumpprof);
  (void) signal (SIGUSR2, s_toggleprof);
#endif
  myargv = argv;
#ifdef _WIN32
  WSAStartup (wVersionRequired, &wsaData);
#else
  (void) umask (077);           /* better safe than sorry --SRB  */
#endif
  memset ((char *) &me, '\0', sizeof (me));

  setup_signals ();
  /*
   * * All command line parameters have the syntax "-fstring"  or "-f
   * string" (e.g. the space is optional). String may  be empty. Flag
   * characters cannot be concatenated (like "-fxyz"), it would
   * conflict with the form "-fstring".
   */
  while (--argc > 0 && (*++argv)[0] == '-')
  {
    char *p = argv[0] + 1;
    int flag = *p++;

    if (flag == '\0' || *p == '\0')
    {
      if (argc > 1 && argv[1][0] != '-')
      {
        p = *++argv;
        argc -= 1;
      }
      else
        p = "";
    }

    switch (flag)
    {
       case 'a':
         bootopt |= BOOT_AUTODIE;
         break;
       case 'c':
         bootopt |= BOOT_CONSOLE;
         break;
       case 'q':
         bootopt |= BOOT_QUICK;
         break;
#ifndef _WIN32
       case 'd':
         (void) setuid ((uid_t) uid);
         etcpath = p;
         break;
       case 'o':               /* Per user local daemon... */
         (void) setuid ((uid_t) uid);
         bootopt |= BOOT_OPER;
         break;
# ifdef CMDLINE_CONFIG
       case 'f':
         (void) setuid ((uid_t) uid);
         configfile = p;
         break;

#  ifdef KPATH
       case 'k':
         (void) setuid ((uid_t) uid);
         klinefile = p;
         break;
#  endif

# endif
#endif
       case 'h':
         strncpyzt (me.name, p, sizeof (me.name));
         break;
       case 'i':
         bootopt |= BOOT_INETD | BOOT_AUTODIE;
         break;
       case 'p':
         if ((portarg = atoi (p)) > 0)
           portnum = portarg;
         break;
       case 's':
         bootopt |= BOOT_STDERR;
         break;
       case 't':
#ifndef _WIN32
         (void) setuid ((uid_t) uid);
#endif
         bootopt |= BOOT_TTY;
         break;
       case 'v':
         (void) printf ("= ircd %s\n", version);
         exit (0);
       case 'x':
#ifdef	DEBUGMODE
# ifndef _WIN32
         (void) setuid ((uid_t) uid);
# endif
         debuglevel = atoi (p);
         debugmode = *p ? p : "0";
         bootopt |= BOOT_DEBUG;
         break;
#else
         fprintf (stderr,
                  "========================================================\n");
         fprintf (stderr, "===== ERROR =====\n");
         fprintf (stderr, "%s: DEBUGMODE must be defined for -x y\n",
                  myargv[0]);
         fprintf (stderr,
                  "========================================================\n");
         exit (0);
#endif
       default:
         bad_command ();
         break;
    }
  }

#if 0
  /* Why is this here? Breaks IRCD_PREFIX = "." -- Radiant */
  if (chdir (etcpath))
  {
    fprintf (stderr,
             "========================================================\n");
    fprintf (stderr, "===== ERROR =====\n");
    fprintf (stderr,
             "Unable to find the configuration directory specified during\n");
    fprintf (stderr, "configure: %s \n", etcpath);
    fprintf (stderr, "Please ensure that it exsists and is accessible.\n");
    fprintf (stderr,
             "========================================================\n");
    exit (-1);
  }
#endif

#ifndef _WIN32
  if ((uid != euid) && !euid)
  {
    fprintf (stderr,
             "========================================================\n");
    fprintf (stderr, "===== ERROR =====\n");
    fprintf (stderr, "Do NOT run ircd setuid root.\n");
    fprintf (stderr, "Make it setuid a normal user \n");
    fprintf (stderr,"========================================================\n");
    exit (-1);
  }

  if (!uid)
  {
    fprintf (stderr,
             "========================================================\n");
    fprintf (stderr, "===== ERROR =====\n");
    fprintf (stderr, "You've started the IRCD as root!\n");
    fprintf (stderr, "This is HIGHLY discouraged as it can open you to\n");
    fprintf (stderr, "attacks and exploits!\n");
    fprintf (stderr,"========================================================\n");
    exit (-1);
  }
#endif

  if (argc > 0)
    return bad_command ();      /* This should exit out  */


  fprintf (stderr,"====================[ %s ]==================\n",version);
  fprintf (stderr, "Starting SurrealIRCD...\n");
#ifdef USE_SSL
  fprintf (stderr, "Built with %s\n", OPENSSL_VERSION_TEXT);
#endif
#ifdef HAVE_ENCRYPTION_ON
  if (dh_init () == -1)
    return 0;
#endif

  motd = (aMotd *) NULL;
  rules = (aRules *) NULL;
  omotd = (aOMotd *) NULL;
  helpfile = (aMotd *) NULL;
  motd_tm = NULL;
  rules_tm = NULL;
  omotd_tm = NULL;
  memset (&log_tm, 0, sizeof (struct tm));
  shortmotd = NULL;
  read_motd (MOTD);
  read_rules (RULES);
  read_opermotd (OMOTD);
  /* read_help (HELPFILE); */
  read_shortmotd (SHORTMOTD);

  clear_client_hash_table ();
  clear_channel_hash_table ();
  clear_scache_hash_table ();   /* server cache name table */
  clear_ip_hash_table ();       /* client host ip hash table */

  /* init the throttle system -wd */
  throttle_init ();

  /* init the file descriptor tracking system */
  init_fds ();

  /* init the kline/akill system */
  init_userban ();

  initlists ();
  initclass ();
  initwhowas ();
  initstats ();
  booted = FALSE;
  init_dynconf();
  load_conf (DCONF, 0);
  booted = TRUE;
  check_dynconf (0);
  (void) fprintf (stderr, "Config parser initialized\n");
  /* We have dynamic hubs... prevent hub's from starting in HTM */
  if (HUB == 1)
  {
    lifesux = 0;
  }
  /* Now we can build the protoctl */
  cr_loadconf (IRCD_RESTRICT, 0);
  init_tree_parse (msgtab);
  init_send ();
  NOW = time (NULL);
  open_debugfile ();
  NOW = time (NULL);

  init_fdlist (&default_fdlist);
  {
    int i;

    for (i = MAXCONNECTIONS + 1; i > 0; i--)
    {
      default_fdlist.entry[i] = i - 1;
    }
  }

  if ((timeofday = time (NULL)) == -1)
  {
#ifdef USE_SYSLOG
    syslog (LOG_WARNING, "Clock Failure (%d), TS can be corrupted", errno);
#endif

    fprintf (stderr,
             "Warning: Clock Failure (%d), TS can be corrupted!!\n", errno);
    sendto_realops ("Clock Failure (%d), TS can be corrupted", errno);
  }



/*      if (portnum < 0)
        portnum = 4444;
  */
  /*
   * We always have a portnumber in the M Line, so we take the default port from there always - ShadowMaster
   */
  me.port = portnum;

#ifdef USE_SSL
  fprintf (stderr, "Initializing Client SSL support\n");

  fd = open (IRCDSSL_CPATH, O_RDONLY);
  if (fd == -1)
  {
    fprintf (stderr, "SurrealIRCD Encountered Errors while starting:\n");
    fprintf (stderr, "\tIRCd SSL Error: Unable to open %s\n", IRCDSSL_CPATH);
    fprintf (stderr, "\t(did you generate your certificate ?)\n");
    fprintf (stderr, "========================================================\n");
    exit (-1);
  }
  close (fd);

  fd = open (IRCDSSL_KPATH, O_RDONLY);
  if (fd == -1)
  {
    fprintf (stderr, "SurrealIRCD Encountered Errors while starting:\n");
    fprintf (stderr, "\tIRCd SSL Error: Unable to open %s\n", IRCDSSL_KPATH);
    fprintf (stderr, "\t(did you generate your certificate ?)\n");
    fprintf (stderr, "========================================================\n");
    exit (-1);
  }
  close (fd);


  if (!(ssl_capable = initssl ()))
  {
    fprintf (stderr, "Client SSL initialization failure.\n");
    fprintf (stderr, "(did you generate your certificate ?)\n");
    fprintf (stderr, "Server running with Client SSL disabled, consult the above error log for details.\n");
  }
  else
  {
    fprintf (stderr, "Client SSL initialization complete\n");
  }
#endif

  /*
   * init_sys() will also launch the IRCd into the background so we wont be able to send anything to stderr and have
   * it seen by the user unless he is running it in console debugmode. So we want to do some checks here to be able to
   * tell the client about it - ShadowMaster
   */

  if ((fd = openconf(configfile)) == -1)
  {
    Debug ((DEBUG_FATAL, "Failed in reading configuration file %s",configfile));
    fprintf (stderr, "SurrealIRCD Encountered Errors while starting:\n");
    fprintf (stderr, "IRCd Fatal Error: Unable to open config %s file!\n", configfile);
    fprintf (stderr, "========================================================\n");
    exit (-1);
  }
#ifdef SEPARATE_QUOTE_KLINES_BY_DATE
  {
    struct tm *tmptr;
    char timebuffer[20], filename[200];

    tmptr = localtime (&NOW);
    strftime (timebuffer, 20, "%y%m%d", tmptr);
    ircsprintf (filename, "%s.%s", klinefile, timebuffer);
    if ((fd = openconf(filename)) == -1)
    {
      Debug ((DEBUG_ERROR, "Failed reading kline file %s", filename));
      printf ("Couldn't open kline file %s\n", filename);
    }
  }
#else
# ifdef KPATH
  if ((fd = openconf (klinefile)) == -1)
  {
    Debug ((DEBUG_ERROR, "Failed reading kline file %s", klinefile));
    printf ("Couldn't open kline file %s\n", klinefile);
  }
# endif
#endif

# ifdef VPATH
  if ((fd = openconf (vlinefile)) == -1)
  {
    Debug ((DEBUG_ERROR, "Failed reading vline file %s", vlinefile));
    printf ("Couldn't open vline file %s\n", vlinefile);
  }
# endif

  init_sys ();
  me.flags = FLAGS_LISTEN;
  if (bootopt & BOOT_INETD)
  {
    me.fd = 0;
    local[0] = &me;
    me.flags = FLAGS_LISTEN;
  }
  else
    me.fd = -1;

#ifdef USE_SYSLOG
# define SYSLOG_ME     "ircd"
  openlog (SYSLOG_ME, LOG_PID | LOG_NDELAY, LOG_FACILITY);
#endif

  if ((fd = openconf (configfile)) != -1)
    initconf (bootopt, fd, NULL);

# ifdef KPATH
  if ((fd = openconf (klinefile)) != -1)
    initconf (0, fd, NULL);
#endif

#ifdef VPATH
  if ((fd = openconf (vlinefile)) != -1)
    initconf (0, fd, NULL);
#endif

  if (!(bootopt & BOOT_INETD))
  {
    static char star[] = "*";
    aConfItem *aconf;
#ifndef	INET6
    u_long vaddr;
#else
    char vaddr[sizeof (struct IN_ADDR)];
#endif

    if ((aconf = find_me ()) && portarg <= 0 && aconf->port > 0)
    {
      portnum = aconf->port;
    }

    if (portnum <= 0)
    {
      fprintf (stderr, "Missing M:line port !\n");
      fprintf (stderr, "Your ircd.conf has no valid port in the M:line\n");
      fprintf (stderr, "========================================================\n");
      exit (1);
    }

    Debug ((DEBUG_ERROR, "Port = %d", portnum));

    if (!aconf)
    {
      fprintf (stderr, "Missing M:line !\n");
      fprintf (stderr, "Your ircd.conf has no valid M:line\n");
      fprintf (stderr, "========================================================\n");

#ifdef USE_SYSLOG
      (void) syslog (LOG_CRIT, "Couldnt find any M:line\n");
#endif
      ilog (LOGF_CRITICAL, "[ERROR] Could not find M:line in configuration");
      exit (1);
    }
    else
    {
      if ((aconf->passwd[0] != '\0') && (aconf->passwd[0] != '*'))
#ifndef INET6
        inet_pton (AFINET, aconf->passwd, &vaddr);
#else
        inet_pton (AFINET, aconf->passwd, vaddr);
#endif
      else
#ifndef INET6
        vaddr = 0;
#else
        memset (vaddr, 0x0, sizeof (vaddr));
#endif
    }

    if (inetport (&me, star, portnum, vaddr))
    {
      if (bootopt & BOOT_STDERR)
      {
        fprintf (stderr, "Couldn't bind to primary port %d\n", portnum);
        fprintf (stderr, "Please make sure no other application is using port %d on this IP\n", portnum);
        fprintf (stderr, "and that the IRCd is not already running.\n");
        fprintf (stderr, "========================================================\n");
      }
#ifdef USE_SYSLOG
      syslog (LOG_CRIT, "Couldn't bind to primary port %d\n", portnum);
#endif
      ilog (LOGF_CRITICAL, "[ERROR] Could not bind to primary port %d!",
            portnum);
      exit (1);
    }
  }
  else if (inetport (&me, "*", 0, 0))
  {
    if (bootopt & BOOT_STDERR)
    {
      fprintf (stderr, "Couldn't bind to port passed from inetd\n");
      fprintf (stderr,
               "========================================================\n");
    }
#ifdef USE_SYSLOG
    syslog (LOG_CRIT, "Couldn't bind to port passed from inetd\n");
#endif
    ilog (LOGF_CRITICAL, "[ERROR] Could not bind to port passed from inetd");
    exit (1);
  }

  set_non_blocking (me.fd, &me);
  add_fd (me.fd, FDT_LISTENER, &me);
  set_fd_flags (me.fd, FDF_WANTREAD);
  get_my_name (&me, me.sockhost, sizeof (me.sockhost) - 1);
  if (me.name[0] == '\0')
    strncpyzt (me.name, me.sockhost, sizeof (me.name));
  me.hopcount = 0;
  me.authfd = -1;
  me.confs = NULL;
  me.next = NULL;
  me.user = NULL;
  me.from = &me;
  SetMe (&me);
  make_server (&me);
  me.serv->up = me.name;
  me.lasttime = me.since = me.firsttime = NOW;
  (void) add_to_client_hash_table (me.name, &me);

  /* We don't want to calculate these every time they are used :) */


  sprintf (REPORT_DO_DNS, REPORT_DO_DNS_, me.name);
  sprintf (REPORT_FIN_DNS, REPORT_FIN_DNS_, me.name);
  sprintf (REPORT_FIN_DNSC, REPORT_FIN_DNSC_, me.name);
  sprintf (REPORT_FAIL_DNS, REPORT_FAIL_DNS_, me.name);
  sprintf (REPORT_DO_ID, REPORT_DO_ID_, me.name);
  sprintf (REPORT_FIN_ID, REPORT_FIN_ID_, me.name);
  sprintf (REPORT_FAIL_ID, REPORT_FAIL_ID_, me.name);

  R_do_dns = strlen (REPORT_DO_DNS);
  R_fin_dns = strlen (REPORT_FIN_DNS);
  R_fin_dnsc = strlen (REPORT_FIN_DNSC);
  R_fail_dns = strlen (REPORT_FAIL_DNS);
  R_do_id = strlen (REPORT_DO_ID);
  R_fin_id = strlen (REPORT_FIN_ID);
  R_fail_id = strlen (REPORT_FAIL_ID);

  check_class ();
  if (bootopt & BOOT_OPER)
  {
    aClient *tmp = add_connection (&me, 0);

    if (!tmp)
      exit (1);
    SetMaster (tmp);
  }
  else
    write_pidfile ();

  Debug ((DEBUG_NOTICE, "Server ready..."));
#ifdef USE_SYSLOG
  syslog (LOG_NOTICE, "Server Ready");
#endif
  NOW = time (NULL);

  if ((timeofday = time (NULL)) == -1)
  {
#ifdef USE_SYSLOG
    syslog (LOG_WARNING, "Clock Failure (%d), TS can be corrupted", errno);
#endif
    sendto_ops ("Clock Failure (%d), TS can be corrupted", errno);
  }

#ifdef DUMP_DEBUG
  dumpfp = fopen ("dump.log", "w");
#endif

#ifdef _WIN32
  sprintf (ctitle, "UltimateIRCd - %s", me.name);
  SetConsoleTitle (ctitle);
#endif

  ilog (LOGF_CRITICAL,
        "IRCd startup successful, %s listening on main port %d", me.name,
        portnum);
  io_loop ();
  return 0;
}

void
do_recvqs ()
{
  DLink *lp, *lpn;
  aClient *client_p;

  for (lp = recvq_clients; lp; lp = lpn)
  {
    lpn = lp->next;
    client_p = lp->value.client_p;

    if (DBufLength (&client_p->recvQ) && !NoNewLine (client_p))
    {
      if (do_client_queue (client_p) == FLUSH_BUFFER)
      {
        continue;
      }
    }

    if (!(DBufLength (&client_p->recvQ) && !NoNewLine (client_p)))
    {
      remove_from_list (&recvq_clients, client_p, lp);
      client_p->flags &= ~(FLAGS_HAVERECVQ);
    }
  }
}

void
send_safelists ()
{
  DLink *lp, *lpn;
  aClient *client_p;

  for (lp = listing_clients; lp; lp = lpn)
  {
    lpn = lp->next;

    client_p = lp->value.client_p;
    while (DoList (client_p) && IsSendable (client_p))
      send_list (client_p, 64);
  }
}

static time_t next_gc = 0;
void
io_loop ()
{
  char to_send[200];
  time_t lasttime = 0;
  long lastrecvK = 0;
  int lrv = 0;

  time_t next10sec = 0;         /* For events we do every 10 seconds */

  time_t lastbwcalc = 0;
  long lastbwSK = 0, lastbwRK = 0;
  time_t lasttimeofday;
  int delay = 0;
#if defined(_WIN32) && defined(W32_STATUSCONSOLE)
  static time_t lastconsoleupd = 0;
#endif

  while (1)
  {
    lasttimeofday = timeofday;

    if ((timeofday = time (NULL)) == -1)
    {
#ifdef USE_SYSLOG
      syslog (LOG_WARNING, "Clock Failure (%d), TS can be corrupted", errno);
#endif
      sendto_ops ("Clock Failure (%d), TS can be corrupted", errno);
    }

    if (timeofday < lasttimeofday)
    {
      ircsprintf (to_send,
                  "System clock is running backwards - (%d < %d)",
                  timeofday, lasttimeofday);
      report_error (to_send, &me);
    }

    NOW = timeofday;


    if (!next_gc)
    {
      next_gc = timeofday + 600;
    }

    /*
     * Calculate a moving average of our total traffic.
     * Traffic is a 4 second average, 'sampled' every 2 seconds.
     */

    if ((timeofday - lastbwcalc) >= 2)
    {
      long ilength = timeofday - lastbwcalc;

      curSendK += (float) (me.sendK - lastbwSK) / (float) ilength;
      curRecvK += (float) (me.receiveK - lastbwRK) / (float) ilength;
      curSendK /= 2;
      curRecvK /= 2;

      lastbwSK = me.sendK;
      lastbwRK = me.receiveK;
      lastbwcalc = timeofday;
    }

    /*
     * This chunk of code determines whether or not "life sucks", that
     * is to say if the traffic level is so high that standard server
     * commands should be restricted
     *
     * Changed by Taner so that it tells you what's going on as well as
     * allows forced on (long LCF), etc...
     */

    if ((timeofday - lasttime) >= LCF)
    {
      lrv = LRV * LCF;
      lasttime = timeofday;
      currlife = (me.receiveK - lastrecvK) / LCF;
      if ((me.receiveK - lrv) > lastrecvK || HTMLOCK == YES)
      {
        if (!lifesux)
        {
          lifesux = 1;

          if (noisy_htm)
            sendto_ops
              ("Entering high-traffic mode - (%dk/s > %dk/s)", currlife, LRV);
        }
        else
        {
          lifesux++;            /* Ok, life really sucks! */
          LCF += 2;             /* Wait even longer */
          if (noisy_htm)
            sendto_ops
              ("Still high-traffic mode %d%s (%d delay): %dk/s",
               lifesux, (lifesux > 9) ? " (TURBO)" : "", (int) LCF, currlife);

          /* Reset htm here, because its been on a little too long.
           * Bad Things(tm) tend to happen with HTM on too long -epi */

          if (lifesux > 15)
          {
            if (noisy_htm)
              sendto_ops
                ("Resetting HTM and raising limit to: %dk/s\n", LRV + 5);
            LCF = LOADCFREQ;
            lifesux = 0;
            LRV += 5;
          }
        }
      }
      else
      {
        LCF = LOADCFREQ;
        if (lifesux)
        {
          lifesux = 0;
          if (noisy_htm)
            sendto_ops ("Resuming standard operation . . . .");
        }
      }
      lastrecvK = me.receiveK;
    }
    /*
     * * We only want to connect if a connection is due, not every
     * time through.  Note, if there are no active C lines, this call
     * to Tryconnections is made once only; it will return 0. - avalon
     */

    if (nextconnect && timeofday >= nextconnect)
      nextconnect = try_connections (timeofday);

    /* DNS checks. One to timeout queries, one for cache expiries. */

#ifdef USE_ADNS
    if (timeofday >= nextdnscheck)
    {
      nextdnscheck = timeout_adns (timeofday);
    }
#else
    if (timeofday >= nextdnscheck)
    {
      nextdnscheck = timeout_query_list (timeofday);
    }
    if (timeofday >= nextexpire)
    {
      nextexpire = expire_cache (timeofday);
    }
#endif

    if (timeofday >= nextbanexpire)
    {
      /*
       * magic number: 31 seconds
       * space out these heavy tasks at semi-random intervals, so as not to coincide
       * with anything else ircd does regularly
       */
      nextbanexpire = NOW + 31;
      expire_userbans ();
#ifdef THROTTLE_ENABLE
      throttle_timer (NOW);
#endif
    }

    if (timeofday >= next10sec)
    {
      next10sec = timeofday + 10;
    }

    /*
     * * take the smaller of the two 'timed' event times as the time
     * of next event (stops us being late :) - avalon WARNING -
     * nextconnect can return 0!
     */

    if (nextconnect)
      delay = MIN (nextping, nextconnect);
    else
      delay = nextping;
    delay = MIN (nextdnscheck, delay);
    delay = MIN (nextexpire, delay);
    delay -= timeofday;

    /*
     * Parse people who have blocked recvqs
     */
    do_recvqs ();

    /*
     * Send people their /list replies, being careful
     * not to fill their sendQ
     */
    send_safelists ();

    /*
     * * Adjust delay to something reasonable [ad hoc values] (one
     * might think something more clever here... --msa) 
     * We don't really need to check that often and as long 
     * as we don't delay too long, everything should be ok. 
     * waiting too long can cause things to timeout... 
     * i.e. PINGS -> a disconnection :( 
     * - avalon
     */
    if (delay < 1)
    {
      delay = 1;
    }
    else
    {
      /* We need to get back here to do that recvq thing */
      if (recvq_clients != NULL)
      {
        delay = 1;
      }
      else
      {
        delay = MIN (delay, TIMESEC);
      }
    }

    /*
     * We want to read servers on every io_loop, as well as "busy"
     * clients (which again, includes servers. If "lifesux", then we
     * read servers AGAIN, and then flush any data to servers. -Taner
     */

    engine_read_message (delay);        /* check everything! */

    /*
     * ...perhaps should not do these loops every time, but only if
     * there is some chance of something happening (but, note that
     * conf->hold times may be changed elsewhere--so precomputed next
     * event time might be too far away... (similarly with ping
     * times) --msa
     */

    if ((timeofday >= nextping))
      nextping = check_pings (timeofday);

#ifdef PROFILING
    if (profiling_newmsg)
    {
      sendto_realops ("PROFILING: %s", profiling_msg);
      profiling_newmsg = 0;
    }
#endif

    if (dorehash && !lifesux)
    {
      (void) rehash (&me, &me, 1);
      dorehash = 0;
    }
    /*

     * Flush output buffers on all connections now if they 
     * have data in them (or at least try to flush)  -avalon
     *
     * flush_connections(me.fd);
     *
     * avalon, what kind of crack have you been smoking? why
     * on earth would we flush_connections blindly when
     * we already check to see if we can write (and do)
     * in read_message? There is no point, as this causes
     * lots and lots of unnecessary sendto's which 
     * 99% of the time will fail because if we couldn't
     * empty them in read_message we can't empty them here.
     * one effect: during htm, output to normal lusers
     * will lag.
     */

    /* Now we've made this call a bit smarter. */
    /* Only flush non-blocked sockets. */

    flush_connections (me.fd);

    if (timeofday >= next_gc)
    {
      block_garbage_collect ();
      next_gc = timeofday + 600;
    }


#ifdef	LOCKFILE
    /*
     * * If we have pending klines and CHECK_PENDING_KLINES minutes
     * have passed, try writing them out.  -ThemBones
     */

    if ((pending_klines) && ((timeofday - pending_kline_time)
                             >= (CHECK_PENDING_KLINES * 60)))
      do_pending_klines ();
#endif

#if defined(_WIN32) && defined(W32_STATUSCONSOLE) && !defined(DEBUGMODE)
    if ((timeofday - lastconsoleupd) >= 5)
    {
      win32_updateconsole ();
      lastconsoleupd = timeofday;
    }
#endif
  }
}

/*
 * open_debugfile
 * 
 * If the -t option is not given on the command line when the server is
 * started, all debugging output is sent to the file set by LPATH in
 * config.h Here we just open that file and make sure it is opened to
 * fd 2 so that any fprintf's to stderr also goto the logfile.  If the
 * debuglevel is not set from the command line by -x, use /dev/null as
 * the dummy logfile as long as DEBUGMODE has been defined, else dont
 * waste the fd.
 */
static void
open_debugfile ()
{
  /* Debug() is now handled by ilog() */
  return;
}

#ifdef BACKTRACE_DEBUG
#include <execinfo.h>

/*
 * lastdyingscream - signal handler function
 * Will output backtrace info.
 *
 * Written by solaris - mbusigin@amber.org.uk
 * code must be compiled with -rdynamic and some sort of -g flag
 */
void
lastdyingscream (int s)
{
  int fd;
  void *array[128];
  int r;

  fd = open ("lastdyingscream.log", O_CREAT | O_WRONLY);

  memset (array, '\0', sizeof (array));
  r = backtrace (array, sizeof (array) - 1);

  backtrace_symbols_fd (array, r, fd);
  shutdown (fd, 2);
  close (fd);
}
#endif

static void
setup_signals ()
{
#ifndef _WIN32
  struct sigaction act;

  act.sa_handler = SIG_IGN;
  act.sa_flags = 0;
  sigemptyset (&act.sa_mask);
  sigaddset (&act.sa_mask, SIGPIPE);
  sigaddset (&act.sa_mask, SIGALRM);
# ifdef	SIGWINCH
  sigaddset (&act.sa_mask, SIGWINCH);
  sigaction (SIGWINCH, &act, NULL);
# endif
  sigaction (SIGPIPE, &act, NULL);

  act.sa_handler = dummy;
  sigaction (SIGALRM, &act, NULL);

  act.sa_handler = s_rehash;
  sigemptyset (&act.sa_mask);
  sigaddset (&act.sa_mask, SIGHUP);
  sigaction (SIGHUP, &act, NULL);

  act.sa_handler = s_restart;
  sigaddset (&act.sa_mask, SIGINT);
  sigaction (SIGINT, &act, NULL);

  act.sa_handler = s_die;
  sigaddset (&act.sa_mask, SIGTERM);
  sigaction (SIGTERM, &act, NULL);

# ifdef RESTARTING_SYSTEMCALLS
  /*
   * * At least on Apollo sr10.1 it seems continuing system calls 
   * after signal is the default. The following 'siginterrupt' 
   * should change that default to interrupting calls.
   */
  siginterrupt (SIGALRM, 1);
# endif
#endif /* _WIN32 */
}


void
build_version (void)
{
  char *s = PATCHES;
  ircsprintf (version, "%s(%s)-%s.%s(%s)%s%s", BASE_VERSION, RELEASE_NAME,
              MAJOR_RELEASE, MINOR_RELEASE, REVISION,
              (*s != 0 ? PATCHES : ""),
#ifdef _WIN32
              "-Win32"
#else
              ""
#endif
    );
}

static inline char *
log_head (int logflags)
{
  aLogfile *lf = iLogfiles;
  for (; lf->logname; lf++)
  {
    if (lf->logflag == logflags)
    {
      return lf->logname;
    }
  }

  return "?";
}

void
log_open (aLogfile * lf, struct tm *mytime)
{
  char timestamp[128];
  char filename[128];
  struct tm now;

  if (!mytime)
    now = *localtime (&timeofday);
  else
    now = *mytime;

  if ((LOG_CYCLE) && (!LOG_CYCLE_RENAMEAFTER))
  {
    /* Some GCC versions throws a small fit and give warnings for %c and %y in strftime.
     * Such warnings can be safely ignored - ShadowMaster
     */
    strftime (timestamp, 128, LOGTIMEFORMAT, &now);
    sprintf (filename, CLOGFILEFORMAT, lf->logname, timestamp);
  }
  else
  {
    sprintf (filename, LOGFILEFORMAT, lf->logname);
  }

  lf->logfd = fopen (filename, "a");
  if (!lf->logfd)
  {
    sendto_realops ("Logfile open failed! %s (errno %d)", filename, errno);
  }
  else
    ilog (lf->logflag, "Logging started");
}

void
log_cycle (void)
{
  struct tm now;
  aLogfile *lf = iLogfiles;

  now = *localtime (&timeofday);

  if (now.tm_mday != log_tm.tm_mday)
  {
    do
    {
      if (!(LOG_FLAGS & lf->logflag))
        continue;

      if ((lf->logfd) && (LOG_CYCLE))
      {
        ilog (lf->logflag, "End of logfile");
        fclose (lf->logfd);
        lf->logfd = NULL;

        if (LOG_CYCLE_RENAMEAFTER)
        {
          /* Rename the logfile now */
          char timestamp[128];
          char filename[128], newfilename[128];
          sprintf (filename, LOGFILEFORMAT, lf->logname);

          strftime (timestamp, 128, LOGTIMEFORMAT, &log_tm);
          sprintf (newfilename, CLOGFILEFORMAT, lf->logname, timestamp);
          rename (filename, newfilename);
        }
        /* gzip archived log here? */
      }

      log_open (lf, &now);
      lf++;
    }
    while (LOG_MULTIFILES && lf->logname);

    log_tm = *localtime (&timeofday);
  }
}

void
ilog (int flags, const char *fmt, ...)
{
  /* NOTE: Do not use ilog() from any function called by ilog(),
   * this will cause the original log message to be lost, because
   * of a static buffer that is used.
   */
  static time_t lastcyclecheck = 0;
  char logline[1024];
  char timestamp[128];
  int len;
  va_list vargs;
  aLogfile *lf = iLogfiles;

  /*** FIXME ***/
  /*
   * This code needs more testing. There are known issues which i have not had time to track down.
   */
  return;

  if (LOG_FLAGS & flags)
  {
    if (LOG_MULTIFILES)
    {
      for (; lf->logname; lf++)
      {
        if (flags & lf->logflag)
          break;
      }
      if (!lf->logname)
        abort ();               /* Trying to log with unknown flag, bail! */
    }

    if ((!lf->logfd) || (LOG_CYCLE && (lastcyclecheck + 30 <= timeofday)))
    {
      lastcyclecheck = timeofday;
      log_cycle ();
      if (!lf->logfd)
        /* In case the flag was added to the config file while the ircd 
         * is running, log_cycle doesn't see that
         */
        log_open (lf, NULL);
    }

    va_start (vargs, fmt);
    /* First, print time in standard format */
    /* Some GCC versions throws a small fit and give warnings for %c and %y in strftime.
     * Such warnings can be safely ignored - ShadowMaster
     */
    strftime (timestamp, 1024, "%c", localtime (&timeofday));
    /* The actual log message */
    len = vsnprintf (logline, 1024, fmt, vargs);

    /* To the log file */
    if (LOG_MULTIFILES)
      fprintf (lf->logfd, "[%s] %s\n", timestamp, logline);
    else
      fprintf (lf->logfd, "[%s] [%s] %s\n", timestamp, log_head (flags),
               logline);
    fflush (lf->logfd);

    /* To stderr if it's left open */
    if (bootopt & BOOT_STDERR)
      fprintf (stderr, "[%s] [%s] %s\n", timestamp, log_head (flags),
               logline);

    va_end (vargs);
  }
}

void
mode_sort (char *str)
{
  char temp;
  int x, y;

  for (x = 0; x < (strlen (str) - 1); x++)
  {
    for (y = x + 1; y < strlen (str); y++)
    {
      if (MyIsUpper (str[y]) && MyIsLower (str[x]))
        continue;
      if ((MyIsLower (str[y]) && MyIsUpper (str[x])) || (str[y] < str[x]))
      {
        temp = str[x];
        str[x] = str[y];
        str[y] = temp;
      }
    }
  }
}

#if defined(_WIN32) && defined(W32_STATUSCONSOLE)
void
win32_updateconsole (void)
{
  system ("cls");               /* FIXME, use a better way to do this */
  printf
    ("-------------------------------------------------------------------------------\r\n");
  printf ("%s (%s)\r\n", me.name, IRCNETWORK);
  printf
    ("-------------------------------------------------------------------------------\r\n");
  printf ("Version:		%s\r\n", version);
  printf ("\r\n");
  printf ("Local users:		%d (%d invisible)\r\n", Count.local,
          Count.invisi);
  printf ("Max local users:	%d\r\n", Count.max_loc);
  printf ("Global users:		%d\r\n", Count.total);
  printf ("Max global users:	%d\r\n", Count.max_tot);
  printf ("\r\n");
  printf ("Channels formed:	%d\r\n", Count.chan);
  printf ("Operators online:	%d\r\n", Count.oper);
  printf ("Servers:		%d\r\n", Count.server);
  printf ("\r\n");
  printf ("Connections\r\n");
  printf ("	Today:		%d\r\n", Count.today);
  printf ("	This week:	%d\r\n", Count.weekly);
  printf ("	This month:	%d\r\n", Count.monthly);
  printf ("	This year:	%d\r\n", Count.yearly);
}
#endif
