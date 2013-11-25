/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/config.h
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
 *  $Id: config.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


#ifndef	__config_include__
#define	__config_include__

#ifdef _WIN32
# include "setup_win32.h"
#else
# include "setup.h"
#endif
#include "dynconf.h"
#include "defs.h"

/* DONT_USE_REGISTER: this tells the compiler not to try to make
 * variables 'register', almost always a good idea on a modern
 * compiler */
#define DONT_USE_REGISTER

/* STATS_NOTICE - See a notice when a user does a /stats */
#define STATS_NOTICE

/*
 * MAXSENDQLENGTH - Max amount of internal send buffering Max amount of
 * internal send buffering when socket is stuck (bytes)
 */
#define MAXSENDQLENGTH 5050000

/*
 * BUFFERPOOL - the maximum size of the total of all sendq's.
 * Recommended value is four times MAXSENDQLENGTH.
 */
#define	BUFFERPOOL     (4 * MAXSENDQLENGTH)

/* ADD_LINES
 * Enables the /ADDCNLINE /ADDULINE /ADDHLINE /ADDQLINE
 *	       /DELCNLINE /DELULINE /DELHLINE /DELQLINE
 * commands.
 * These commands are used to add config lines.
 * An oper needs +E in his o:line to use this commands.
 * Define here to enable this commands.
 * 
 */
#define ADD_LINES


/*
 * File Descriptor limit
 *
 * These limits are ultra-low to guarantee the server will work properly
 * in as many environments as possible.  They can probably be increased
 * significantly, if you know what you are doing.
 *
 * Note that HARD_FDLIMIT_ is modified with the maxclients setting in
 * configure.  You should only touch this if you know what you are doing!!!
 *
 * If you are using select for the IO loop, you may also need to increase
 * the value of FD_SETSIZE by editting the Makefile.  However, using
 * --enable-kqueue or --enable-poll if at all possible,
 * is highly recommended.
 *
 * VMS should be able to use any suitable value here, other operating
 * systems may require kernel patches, configuration tweaks, or ulimit
 * adjustments in order to exceed certain limits (e.g. 1024, 4096 fds).
 */
#define HARD_FDLIMIT_    MAX_CLIENTS + 10 + 20

/* XXX - MAX_BUFFER is mostly ignored. */
/*
 * Maximum number of connections to allow.
 *
 * MAX_BUFFER  is the number of fds to reserve, e.g. for clients
 *             exempt from limit.
 *
 * 10 file descriptors are reserved for logging, DNS lookups, etc.,
 * so MAX_CLIENTS + MAX_BUFFER + 10 must not exceed HARD_FDLIMIT.
 * NOTE: MAX_CLIENTS is set with configure now
 */
#define MAX_BUFFER      10


/* OS Depentant ifdefs */

#ifdef OS_SOLARIS
#define NO_PRIORITY
#else
#undef NO_PRIORITY
#endif

/* dirs */
#define DPATH		IRCD_PREFIX                                                     
#define BINPATH		IRCD_PREFIX "/bin/"
#define ETCPATH		IRCD_PREFIX "/etc"
#define LOGPATH		IRCD_PREFIX "/logs"

/* files */
#define SPATH		BINPATH "/ircd"			/* ircd executable */

#define CPATH		ETCPATH "/ircd.conf"		/* ircd.conf file */
#define DCONF		ETCPATH "/ircd.ini"		/* Dynamic Configuration File */
#define KPATH		ETCPATH "/kline.conf"		/* kline file */
#define VPATH		ETCPATH "/vhost.conf"		/* V-Line file */
#define IRCD_RESTRICT 	ETCPATH "/channels.conf"	/* Channel Restrict file */
#define	MPATH		ETCPATH "/ircd.motd"		/* Message Of The Day */
#define	SMPATH		ETCPATH "/ircd.smotd"		/* Short Message Of The Day */
#define OMPATH		ETCPATH "/ircd.opermotd"	/* Operator Message Of The Day */
#define RPATH		ETCPATH "/ircd.rules"		/* Rules File */
#define	PPATH		ETCPATH "/ircd.pid"		/* IRCd PID */
#define IRCDSSL_CPATH 	ETCPATH "/ircd.crt"		/* Client SSL */
#define IRCDSSL_KPATH 	ETCPATH "/ircd.key"		/* Client SSL */
#define LOGTIMEFORMAT "%d%b%y" /* Time format for saved logs */
#define CLOGFILEFORMAT LOGPATH "/%s-%s.log" /* Format for cycled logs */
#define LOGFILEFORMAT LOGPATH "/%s.log" /* Format for non-cycled logs */

/*
 * USE_SYSLOG - log errors and such to syslog() If you wish to have the
 * server send 'vital' messages about server through syslog, define
 * USE_SYSLOG. Only system errors and events critical to the server are
 * logged although if this is defined with FNAME_USERLOG, syslog() is
 * used instead of the above file. It is not recommended that this
 * option is used unless you tell the system administrator beforehand
 * and obtain their permission to send messages to the system log
 * files.
 * 
 * If you have access to the SYSLOG files IT IS STRONGLY RECOMENDED
 * THAT YOU *DO* USE SYSLOG.  Many fatal ircd errors are only logged to syslog.
 */
#undef	USE_SYSLOG

#ifdef	USE_SYSLOG
/*
 * SYSLOG_KILL SYSLOG_SQUIT SYSLOG_CONNECT SYSLOG_USERS SYSLOG_OPER If
 * you use syslog above, you may want to turn some (none) of the
 * spurious log messages for KILL,SQUIT,etc off.
 */
#undef	SYSLOG_KILL		/* log all operator kills */
#undef	SYSLOG_SQUIT		/* log all remote squits */
#undef	SYSLOG_CONNECT		/* log remote connect messages */
#undef	SYSLOG_USERS		/* send userlog stuff to syslog */
#undef	SYSLOG_OPER		/* log all users who successfully oper */
#undef  SYSLOG_BLOCK_ALLOCATOR	/* debug block allocator */
#undef	SYSLOG_ADDLINES		/* log/debug add addline commands */

/*
 * LOG_FACILITY - facility to use for syslog() Define the facility you
 * want to use for syslog().  Ask your sysadmin which one you should
 * use.
 *
 * LOG_LOCAL0 - LOG_LOCAL7	are reserved for local use.
 * LOG_USER			is default
 */

#define LOG_FACILITY LOG_LOCAL4
#endif /* USE_SYSLOG */



/*
 * FNAME_USERLOG and FNAME_OPERLOG - logs of local USERS and OPERS
 * Define this filename to maintain a list of persons who log into this
 * server. Logging will stop when the file does not exist. Logging will
 * be disable also if you do not define this. FNAME_USERLOG just logs
 * user connections, FNAME_OPERLOG logs every successful use of /oper.
 * These are either full paths or files within DPATH.
 * 
 */
#define FNAME_USERLOG "logs/users.log"
#define FNAME_OPERLOG "logs/opers.log"


/* Don't change this... */
#define HARD_FDLIMIT	(HARD_FDLIMIT_ - 10)
#define MASTER_MAX	(HARD_FDLIMIT - MAX_BUFFER)

/*
 * TS_MAX_DELTA and TS_WARN_DELTA -  allowed delta for TS when another
 * server connects.
 * 
 * If the difference between my clock and the other server's clock is
 * greater than TS_MAX_DELTA, I send out a warning and drop the links.
 * 
 * If the difference is less than TS_MAX_DELTA, I just sends out a warning
 * but don't drop the link.
 * 
 * TS_MAX_DELTA currently set to 30 minutes to deal with older timedelta
 * implementation.  Once pre-hybrid5.2 servers are eradicated, we can
 * drop this down to 90 seconds or so. --Rodder
 */
#define TS_MAX_DELTA 300	/* seconds */
#define TS_WARN_DELTA 30	/* seconds */

/*
 * DEFAULT_KLINE_TIME
 *
 * Define this to the default time for a kline (in minutes) for klines with
 * unspecified times. undefine this for all klines with unspecified times
 * to be perm. (if defined, a kline with zero time will be perm). -- lucas
 */
#define DEFAULT_KLINE_TIME 30


 /*
 * Pretty self explanatory: These are shown in server notices and to the
 * recipient of a "you are banned" message.
 */
 #define LOCAL_BAN_NAME "K-Line"
 #define NETWORK_BAN_NAME "Autokill"
 #define LOCAL_BANNED_NAME "K-Lined"
 #define NETWORK_BANNED_NAME "Autokilled"

/*
 * LOCKFILE - Exclusive use of ircd.conf and kline.conf during writes
 * 
 * This creates a lockfile prior to writes to ircd.conf or kline.conf, and
 * can be used in conjunction with viconf (included in the tools
 * directory). This prevents loss of data when klines are added while
 * someone is manually editting the file.  File writes will be retried
 * at the next KLINE, REHASH, or after CHECK_PENDING_KLINES
 * minutes have elapsed.
 *
 * If you do not wish to use this feature, leave LOCKFILE #undef
 */
#define LOCKFILE "/tmp/ircd.conf.lock"
#define	CHECK_PENDING_KLINES	10	/* in minutes */

/*
 * FOLLOW_IDENT_RFC
 * 
 * From RFC 1413, "The Identification Protocol is not intended as an
 * authorization or access control protocol ... The use of the
 * information returned by this protocol for other than auditing is
 * strongly discouraged."
 * 
 * Defining this allows all users to set whatever username they wish,
 * despite what may be discovered by ident.  While we may get a valid
 * ident response with a different username than submitted by the
 * client, this option will cause the ident response to be discarded.
 */
#undef FOLLOW_IDENT_RFC

/*
 * NO_IDENT_SYSTYPE_OTHER
 *
 * From RFC1413:
 *
 * "OTHER" indicates the identifier is an unformatted
 * character string consisting of printable characters in
 * the specified character set.  "OTHER" should be
 * specified if the user identifier does not meet the
 * constraints of the previous paragraph.  Sending an
 * encrypted audit token, or returning other non-userid
 * information about a user (such as the real name and
 * phone number of a user from a UNIX passwd file) are
 * both examples of when "OTHER" should be used.
 *
 * However, it seems lots of programs actually default to
 * returning other as a system type. (stock identd with
 * redhat, for example)
 */
#undef NO_IDENT_SYSTYPE_OTHER

/*
 * RFC1035_ANAL Defining this causes ircd to reject hostnames with
 * non-compliant chars. undef'ing it will allow hostnames with _ or /
 * to connect
 */
#define RFC1035_ANAL

/*
 * NO_MIXED_CASE - reject mixed case usernames define this if you wish
 * to reject usernames like: FuckYou which don't have all one case
 */
#undef NO_MIXED_CASE

/*
 * IGNORE_FIRST_CHAR - define this for NO_MIXED_CASE if you wish to
 * ignore the first character
 */
#define IGNORE_FIRST_CHAR

/*
 * HIGHEST_CONNECTION - track highest connection count Define this if
 * you want to keep track of your max connections.
 */
#define HIGHEST_CONNECTION

/*
 * USERNAMES_IN_TRACE - show usernames in trace Define this if you want
 * to see usernames in /trace.
 */
#define USERNAMES_IN_TRACE

/*
 * DO_IDENTD - check identd if you undefine this, ircd will never check
 * identd regardless of @'s in I:lines.  You must still use @'s in your
 * I: lines to get ircd to do ident lookup even if you define this.
 */
#define DO_IDENTD


/* IDENTD_COMPLAIN - yell at users that don't have identd installed */
#undef IDENTD_COMPLAIN

/*
 * CLIENT_COUNT - keep a running count of Local & Global users also
 * redefine the /USERS command
 */
#define CLIENT_COUNT
#ifdef CLIENT_COUNT
#undef HIGHEST_CONNECTION
#endif

/*
 * MOTD_WAIT - minimum seconds between use of MOTD, INFO, HELP, LINKS * 
 * before max use count is reset * -Dianora 
 *
 * We now use this for minumum seconds between the use of RULES, OPERMOTD
 * and WHOIS on opers - ShadowMaster
 */

#define MOTD_WAIT 10

/* MOTD_MAX * max use count before delay above comes into effect */
#define MOTD_MAX 3

/*
 * NO_OPER_FLOOD - disable flood control for opers define this to
 * remove flood control for opers
 */
#define NO_OPER_FLOOD

/*
 * OPER_CAN_FLLOD - disables the message throttling for opers
 * only define here if you know what you are doing
 * Remeber that your client may throttle too and that this is
 * a very powerfull thingy.
 */
#undef OPER_CAN_FLOOD

/* SHOW_UH - show the user@host everywhere */
#define SHOW_UH
#ifdef SHOW_UH
#define USERNAMES_IN_TRACE
#endif

/*
 * SHOW_INVISIBLE_LUSERS - show invisible clients in LUSERS As defined
 * this will show the correct invisible count for anyone who does
 * LUSERS on your server. On a large net this doesnt mean much, but on
 * a small net it might be an advantage to undefine it.
 */
#define	SHOW_INVISIBLE_LUSERS

/*
 * DEFAULT_HELP_MODE - default your opers to +h helper mode.  This
 * is strongly recommended
 */
#define DEFAULT_HELP_MODE

/*
 * MAXIMUM LINKS - max links for class 0 if no Y: line configured
 * 
 * This define is useful for leaf nodes and gateways. It keeps you from
 * connecting to too many places. It works by keeping you from
 * connecting to more than "n" nodes which you have C:blah::blah:6667
 * lines for.
 * 
 * Note that any number of nodes can still connect to you. This only
 * limits the number that you actively reach out to connect to.
 * 
 * Leaf nodes are nodes which are on the edge of the tree. If you want to
 * have a backup link, then sometimes you end up connected to both your
 * primary and backup, routing traffic between them. To prevent this,
 * #define MAXIMUM_LINKS 1 and set up both primary and secondary with
 * C:blah::blah:6667 lines. THEY SHOULD NOT TRY TO CONNECT TO YOU, YOU
 * SHOULD CONNECT TO THEM.
 * 
 * Gateways such as the server which connects Australia to the US can do a
 * similar thing. Put the American nodes you want to connect to in with
 * C:blah::blah:6667 lines, and the Australian nodes with C:blah::blah
 * lines. Have the Americans put you in with C:blah::blah lines. Then
 * you will only connect to one of the Americans.
 * 
 * This value is only used if you don't have server classes defined, and a
 * server is in class 0 (the default class if none is set).
 * 
 */
#define MAXIMUM_LINKS 1

/*
 * IRCII_KLUDGE - leave it defined Define this if you want the server
 * to accomplish ircII standard Sends an extra NOTICE in the beginning
 * of client connection
 */
#undef IRCII_KLUDGE

/* NOISY_HTM - should HTM be noisy by default should be YES or NO */
#define NOISY_HTM YES

/*
 * CMDLINE_CONFIG - allow conf-file to be specified on command line
 * NOTE: defining CMDLINE_CONFIG and installing ircd SUID or SGID is a
 * MAJOR security problem - they can use the "-f" option to read any
 * files that the 'new' access lets them.
 */
#define	CMDLINE_CONFIG

/*
 * FAILED_OPER_NOTICE - send a notice to all opers when someone tries
 * to /oper and uses an incorrect password.
 */
#define FAILED_OPER_NOTICE


/*
 * ANTI_NICK_FLOOD - prevents nick flooding define if you want to block
 * local clients from nickflooding
 */
#define ANTI_NICK_FLOOD

/*
 * defaults allow 4 nick changes in 20 seconds 
 */
#define MAX_NICK_TIME 20
#define MAX_NICK_CHANGES 4

/* NO_AWAY_FLUD
 * reallow propregation of AWAY messages, but do not allow AWAY flooding
 * I reccomend a max of 5 AWAY's in 3 Minutes
 */
#define NO_AWAY_FLUD

#ifdef NO_AWAY_FLUD
# define MAX_AWAY_TIME 180	/* time in seconds */
# define MAX_AWAY_COUNT 5
#endif

/*
 * X_LINES_OPER_ONLY - Allow only local opers to see these stats
 * 
 * E_LINES - ComStud's E-lines (exception lines) 
 * F_LINES - Taner's F-lines (super-exception  lines) Any one with an F
 * line can almost always get on the server, as some file descriptors are
 * reserved for people with this F line especially useful for your opers
 * G_LINES - Bahamut Realname Ban Lines
 * K_LINES - Kill Lines and Auto Kills
 * Q_LINES - Nickname Quarantenes
 * V_LINES - VHOST lines
 */
#define E_LINES_OPER_ONLY
#define F_LINES_OPER_ONLY
#define G_LINES_OPER_ONLY
#define K_LINES_OPER_ONLY
#define Q_LINES_OPER_ONLY
#define V_LINES_OPER_ONLY

/* If you want your server to save and restore the local/max client count,
 * define this here.  It's neat, non-load-producing, and makes you
 * appear more attractive to the opposite sex. --wd */
#define SAVE_MAXCLIENT_STATS

/* Define the time (in seconds) that you want to save the statistics after. 
 * Don't undef this unless you have undeffed SAVE_MAXCLIENT_STATS --sed */
#define SAVE_MAXCLIENT_STATS_TIME	3600

/*
 * UNKLINE - /quote unkline - remove klines on the fly if you choose to
 * support this, an oper can do a /quote UNKLINE of an exact matching
 * KLINE to remove the kline
 */
#define UNKLINE

/*
 * WARN_NO_NLINE Define this if you want ops to get noticed about
 * "things" trying to connect as servers that don't have N: lines.
 * Twits with misconfigured servers can get really annoying with
 * enabled.
 */
#define WARN_NO_NLINE

/* 
 * HTM_LOCK_ON_NETBURST Define this if you want your server to
 * stay in HTM while the server is synching to the network 
 * after a connect
*/
#undef HTM_LOCK_ON_NETBURST




/******************************************************************
 * STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP
 *
 * You shouldn't change anything below this line, unless absolutely
 * needed.
 */

/*
 * PING_NAZI
 *
 * be nazi-ish about pings (re-check every client connect, 
 * user registration, etc)
 */
#undef PING_NAZI

/*
 * ALWAYS_SEND_DURING_SPLIT
 * on a large network, if your server is carrying large amounts of clients,
 * and your server splits from the main network, the amount of allocated
 * dbufs will skyrocket as buffers fill up with QUIT messages. This code
 * attempts to combat this by sending out data whenever possible during a
 * split. - lucas
 */
#define ALWAYS_SEND_DURING_SPLIT

/* INITIAL_DBUFS - how many dbufs to preallocate */
#define INITIAL_DBUFS 1024	/* preallocate 2 megs of dbufs */

/*
 * MAXBUFFERS - increase socket buffers
 * 
 * Increase send & receive socket buffer up to 64k, keeps clients at 8K
 * and only raises servers to 64K
 */
#define MAXBUFFERS


/*
 * MAXCONNECTIONS - don't touch - change the HARD_FDLIMIT_ instead
 * Maximum number of network connections your server will allow.  This
 * should never exceed max. number of open file descrpitors and wont
 * increase this. Should remain LOW as possible. Most sites will
 * usually have under 30 or so connections. A busy hub or server may
 * need this to be as high as 50 or 60. Making it over 100 decreases
 * any performance boost gained from it being low. if you have a lot of
 * server connections, it may be worth splitting the load over 2 or
 * more servers. 1 server = 1 connection, 1 user = 1 connection. This
 * should be at *least* 3: 1 listen port, 1 dns port + 1 client
 *
 * Change the HARD_FDLIMIT_ instead 
 */
#define MAXCONNECTIONS	HARD_FDLIMIT

/*
 * NICKNAMEHISTORYLENGTH - size of WHOWAS array this defines the length
 * of the nickname history.  each time a user changes nickname or signs
 * off, their old nickname is added to the top of the list. NOTE: this
 * is directly related to the amount of memory ircd will use whilst
 * resident and running - it hardly ever gets swapped to disk!  Memory
 * will be preallocated for the entire whowas array when ircd is
 * started.
 */
#define NICKNAMEHISTORYLENGTH 2048

/*
 * TIMESEC - Time interval to wait and if no messages have been
 * received, then check for PINGFREQUENCY and CONNECTFREQUENCY
 */
#define TIMESEC  5		/* Recommended value: 5 */

/*
 * PINGFREQUENCY - ping frequency for idle connections If daemon
 * doesn't receive anything from any of its links within PINGFREQUENCY
 * seconds, then the server will attempt to check for an active link
 * with a PING message. If no reply is received within (PINGFREQUENCY *
 * 2) seconds, then the connection will be closed.
 */
#define PINGFREQUENCY    120	/* Recommended value: 120 */

/*
 * CONNECTFREQUENCY - time to wait before auto-reconencting If the
 * connection to to uphost is down, then attempt to reconnect every
 * CONNECTFREQUENCY  seconds.
 */
#define CONNECTFREQUENCY 600	/* Recommended value: 600 */

/*
 * HANGONGOODLINK and HANGONGOODLINK Often net breaks for a short time
 * and it's useful to try to establishing the same connection again
 * faster than CONNECTFREQUENCY would allow. But, to keep trying on bad
 * connection, we require that connection has been open for certain
 * minimum time (HANGONGOODLINK) and we give the net few seconds to
 * steady (HANGONRETRYDELAY). This latter has to be long enough that
 * the other end of the connection has time to notice it broke too.
 * 1997/09/18 recommended values by ThemBones for modern Efnet
 */

#define HANGONRETRYDELAY 60	/* Recommended value: 30-60 seconds */
#define HANGONGOODLINK 3600	/* Recommended value: 30-60 minutes */

/*
 * WRITEWAITDELAY - Number of seconds to wait for write to complete if
 * stuck.
 */
#define WRITEWAITDELAY     10	/* Recommended value: 15 */

/*
 * CONNECTTIMEOUT - Number of seconds to wait for a connect(2) call to
 * complete. NOTE: this must be at *LEAST* 10.  When a client connects,
 * it has CONNECTTIMEOUT - 10 seconds for its host to respond to an
 * ident lookup query and for a DNS answer to be retrieved.
 */
#define	CONNECTTIMEOUT	30	/* Recommended value: 30 */

/*
 * KILLCHASETIMELIMIT - Max time from the nickname change that still
 * causes KILL automaticly to switch for the current nick of that user.
 * (seconds)
 */
#define KILLCHASETIMELIMIT 90	/* Recommended value: 90 */

/*
 * FLUD - CTCP Flood Detection and Protection
 * 
 * This enables server CTCP flood detection and protection for local
 * clients. It works well against fludnets and flood clones.  The
 * effect of this code on server CPU and memory usage is minimal,
 * however you may not wish to take the risk, or be fundamentally
 * opposed to checking the contents of PRIVMSG's (though no privacy is
 * breached).  This code is not useful for routing only servers (ie,
 * HUB's with little or no local client base), and the hybrid team
 * strongly recommends that you do not use FLUD with HUB. The following
 * default thresholds may be tweaked, but these seem to work well.
 */
#define FLUD

/* ANTI_DRONE_FLOOD - anti flooding code for drones
 * This code adds server side ignore for a client who gets
 * messaged more than drone_count times within drone_time seconds
 * unfortunately, its a great DOS, but at least the client won't flood off.
 * I have no idea what to use for values here, trying 8 privmsgs
 * within 1 seconds. (I'm told it is usually that fast)
 * I'll do better next time, this is a Q&D -Dianora
 */
#define ANTI_DRONE_FLOOD
#define DRONE_TIME  1
#define DRONE_COUNT 8
#undef DRONE_WARNINGS

/*
 * ANTI_SPAMBOT if ANTI_SPAMBOT is defined try to discourage spambots
 * The defaults =should= be fine for the timers/counters etc. but you
 * can play with them. -Dianora
 * 
 * Defining this also does a quick check whether the client sends us a
 * "user foo x x :foo" where x is just a single char.  More often than
 * not, it's a bot if it did. -ThemBones
 */
#define ANTI_SPAMBOT

/*
 * ANTI_SPAMBOT parameters, don't touch these if you don't understand
 * what is going on.
 * 
 * if a client joins MAX_JOIN_LEAVE_COUNT channels in a row, but spends
 * less than MIN_JOIN_LEAVE_TIME seconds on each one, flag it as a
 * possible spambot. disable JOIN for it and PRIVMSG but give no
 * indication to the client that this is happening. every time it tries
 * to JOIN OPER_SPAM_COUNTDOWN times, flag all opers on local server.
 * If a client doesn't LEAVE a channel for at least 2 minutes the
 * join/leave counter is decremented each time a LEAVE is done
 * 
 */
#define MIN_JOIN_LEAVE_TIME  60
#define MAX_JOIN_LEAVE_COUNT  25
#define OPER_SPAM_COUNTDOWN   5
#define JOIN_LEAVE_COUNT_EXPIRE_TIME 120

/*
 * If ANTI_SPAMBOT_WARN_ONLY is #define'd Warn opers about possible
 * spambots only, do not disable JOIN and PRIVMSG if possible spambot
 * is noticed Depends on your policies.
 */
#undef ANTI_SPAMBOT_WARN_ONLY

/*
 * ANTI_SPAM_EXIT_MESSAGE will prevent users from connecting to the server
 * and instantly quitting with spam in their quit message.
 * The IRCd will simply not display quit messages unless the client have been
 * connected to the server for more than ANTI_SPAM_EXIT_MESSAGE seconds.
 */

#define ANTI_SPAM_EXIT_MESSAGE
#define ANTI_SPAM_EXIT_MESSAGE_TIME 75

/*
** This would have to be changed to run time since we are reading HUB setting from ircd.ini - ShadowMaster
**
#ifdef HUB
# undef FLUD
#endif
*/

#ifdef FLUD
# define FLUD_NUM	   4	/* Number of flud messages to trip alarm */
# define FLUD_TIME	3	/* Seconds in which FLUD_NUM msgs must occur */
# define FLUD_BLOCK	15	/* Seconds to block fluds */
#endif

/*
 * OLD_Y_LIMIT
 *
 * #define this if you prefer the old behaviour of I lines the default
 * behaviour is to limit the total number of clients using the max
 * client limit in the corresponding Y line (class) The old behaviour
 * was to limit the client count per I line without regard to the total
 * class limit. Each have advantages and disadvantages. In an open I
 * line server, the default behaviour i.e. #undef OLD_Y_LIMIT makes
 * more sense, because you can limit the total number of clients in a
 * class. In a closed I line server The old behaviour can make more
 * sense.
 * 
 * -Dianora
 */
#undef OLD_Y_LIMIT

/*
 * If the OS has SOMAXCONN use that value, otherwise Use the value in
 * HYBRID_SOMAXCONN for the listen(); backlog try 5 or 25. 5 for AIX
 * and SUNOS, 25 should work better for other OS's
 */
#define HYBRID_SOMAXCONN 25

/*
 * Throttling support:
 *
 * Enabled and disabled by config
 *
 * THROTTLE_TRIGCOUNT - number of connections to triggle throttle action
 * THROTTLE_TRIGTIME - number of seconds in which THROTTLE_TRIGCOUNT must
 *           happen
 * THROTTLE_RECORDTIME- length to keep records for each ip (since last connect
            from this ip)
 * THROTTLE_HASHSIZE - size of the throttle hashtable, also tuneable
 *
 * Recommended values: 3, 15, 1800. 3+ connections in 15 or less seconds will
 * result in a connection throttle z:line. These are also
 * z: line time grows, pseudo-exponentially
 * first zline : 2 minutes
 * second zline: 5 minutes
 * third zline : 15 minutes
 * fourth zline: 30 minutes
 * anything more is an hour
 * tuneable at runtime. -wd */
#define THROTTLE_TRIGCOUNT 3
#define THROTTLE_TRIGTIME 15
#define THROTTLE_RECORDTIME 1800
#define THROTTLE_HASHSIZE 25147

 /*
  * Message-throttling support.
  * MSG_TARGET_LIMIT: if defined, imposes limits on message targets
  * MSG_TARGET_MIN: initial number of message targets allowed (recommend 5 or less)
  * MSG_TARGET_MAX: maximum number of message targets stored (recommend 5 or
  *                 less)
  * MSG_TARGET_MINTOMAXTIME: number of seconds a user must be online
  *                          before given MSG_TARGET_MAX targets
  * MSG_TARGET_TIME: time before message targets expire (this is what you should
  *                  tweak)
  */
 #define MSG_TARGET_MIN  5
 #define MSG_TARGET_MAX  8 /* MUST BE >= MSG_TARGET_MIN!!! */
 #define MSG_TARGET_MINTOMAXTIME 300
 #define MSG_TARGET_TIME 45

/*
 * ----------------- archaic and/or broken secion --------------------
 */

/*
 * USE_REJECT_HOLD clients that reconnect but are k-lined will have
 * their connections "held" for REJECT_HOLD_TIME seconds, they cannot
 * PRIVMSG. The idea is to keep a reconnecting client from forcing the
 * ircd to re-scan dich_conf. There is one possible serious hitch with
 * this... If it is a mass cloner, your attacker can "REJECT_HOLD" a
 * number of local fd's on your server. Against normal bots this code
 * will be a "win", against mass cloners, it could lose.
 *
 * this is still not working yet
 */
#undef USE_REJECT_HOLD
#define REJECT_HOLD_TIME 60

/*#define DNS_DEBUG*/


/* DONT_CHECK_QLINE_REMOTE
 * Don't check for Q:lines on remote clients.  We can't do anything
 * if a remote client is using a nick q:lined locally, so
 * why check?  If you don't care about the wasted CPU, and you're
 * curious, feel free to #define this.  I recommend you don't
 * on a client server unless it's got a lot of power.
 * -wd */
#define DONT_CHECK_QLINE_REMOTE

/* ------------------------- END CONFIGURATION SECTION -------------------- */
#ifdef APOLLO
#define RESTARTING_SYSTEMCALLS
#endif /*
        * read/write are restarted after signals
        * defining this 1, gets siginterrupt call
        * compiled, which attempts to remove this
        * behaviour (apollo sr10.1/bsd4.3 needs this) 
        */

#define HELPFILE HPATH
#define MOTD MPATH
#define SHORTMOTD SMPATH
#define MYNAME SPATH
#define CONFIGFILE CPATH
#ifdef  KPATH
#define KLINEFILE  KPATH
#endif
#ifdef VPATH
#define VLINEFILE  VPATH
#endif
#define IRCD_PIDFILE PPATH
#define RULES RPATH
#define OMOTD OMPATH

#define MAX_CLIENTS INIT_MAXCLIENTS

#if defined(DEBUGMODE) || defined(DNS_DEBUG)
extern void debug (int level, char *pattern, ...);
# define Debug(x) debug x
#else
# define Debug(x) ;
#endif

#ifdef INET6_DEV
#define RECV(fd, buf, size) recvfrom(fd, buf, size, 0, 0, 0)
#define SEND(fd, buf) sendto(fd, buf, 0, 0, 0, 0)
#else
#define RECV(fd, buf, size) recv(fd, buf, size, 0)
#define SEND(fd, buf) send(fd, buf, 0, 0)
#endif

#define CONFIG_H_LEVEL_12
#endif /* __config_include__ */

