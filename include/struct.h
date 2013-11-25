/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/struct.h
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
 *  $Id: struct.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#ifndef	__struct_include__
#define __struct_include__

#include "config.h"
#if !defined(CONFIG_H_LEVEL_12)
#error Incorrect config.h for this revision of ircd.
#endif

#include <stdio.h>
#include <sys/types.h>
#ifdef _FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE _FD_SETSIZE
#endif
#ifndef _WIN32
# include <netinet/in.h>
# include <netdb.h>
#else
# include <winsock2.h>
#endif
#if defined( HAVE_STDDEF_H )
#include <stddef.h>
#endif
#ifdef ORATIMING
#include <sys/time.h>
#endif

#ifdef USE_SYSLOG
#include <syslog.h>
#if defined( HAVE_SYS_SYSLOG_H )
#include <sys/syslog.h>
#endif
#endif

#ifdef USE_SSL
#include <openssl/rsa.h>       /* OpenSSL stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
/* we dont need/want kerberos support */ 
#ifndef OPENSSL_NO_KRB5
#define OPENSSL_NO_KRB5 1
#endif 
#endif

#ifdef USE_ADNS
#include "res.h"
#endif


#define REPORT_DO_DNS_		":%s NOTICE AUTH :*** Looking up your hostname..."
#define REPORT_FIN_DNS_		":%s NOTICE AUTH :*** Found your hostname"
#define REPORT_FIN_DNSC_	":%s NOTICE AUTH :*** Found your hostname, cached"
#define REPORT_FAIL_DNS_	":%s NOTICE AUTH :*** Couldn't look up your hostname"
#define REPORT_DO_ID_		":%s NOTICE AUTH :*** Checking Ident"
#define REPORT_FIN_ID_		":%s NOTICE AUTH :*** Got Ident response"
#define REPORT_FAIL_ID_		":%s NOTICE AUTH :*** No Ident response"

#define REPORT_DO_PROXY_ 	":%s NOTICE AUTH :*** Checking for insecure socks/proxy server..."
#define	REPORT_NO_PROXY_ 	":%s NOTICE AUTH :*** No socks/proxy server found (good)"
#define REPORT_GOOD_PROXY_ 	":%s NOTICE AUTH :*** Secure socks/proxy server found"
#define REPORT_PROXYEXEMPT_ 	":%s NOTICE AUTH :*** Exempt line found, Skipping socks/proxy check..."

extern char REPORT_DO_DNS[256], REPORT_FIN_DNS[256], REPORT_FIN_DNSC[256],
  REPORT_FAIL_DNS[256], REPORT_DO_ID[256], REPORT_FIN_ID[256],
  REPORT_FAIL_ID[256], REPORT_DO_PROXY[256], REPORT_NO_PROXY[256],
  REPORT_GOOD_PROXY[256], REPORT_PROXYEXEMPT[256];

#include "hash.h"

typedef struct ConfItem aConfItem;
typedef struct Client aClient;
#ifdef USE_ADNS
typedef struct DNSQuery aDNSQuery;
#endif
typedef struct Channel aChannel;
typedef struct User anUser;
typedef struct Server aServer;
typedef struct SLink Link;
typedef struct SLinkD DLink;
typedef struct ChanLink chanMember;
typedef struct SMode Mode;
typedef struct Watch aWatch;
typedef struct Ban aBan;
typedef struct Exempt anExempt;
typedef struct ListOptions LOpts;
typedef long ts_val;

typedef struct MotdItem aMotd;

typedef struct OMotdItem aOMotd;
typedef struct RulesItem aRules;

typedef struct t_crline aCRline;
typedef struct t_logfile aLogfile;


#include "class.h"
#include "dbuf.h"		/* THIS REALLY SHOULDN'T BE HERE!!! --msa */

#define SM_MAXMODES MAXMODEPARAMSUSER

#define	HOSTLEN		63	/* Length of hostname.  Updated to */
				/* comply with RFC1123 */
#define HOSTIPLEN	40
#define IPLEN		16	/* binary length of an ip6 number (128 bits) */

/* - Dianora  */


#define	NICKLEN		30

/* Necessary to put 9 here instead of 10  if
 * s_msg.c/m_nick has been corrected.  This
 * preserves compatibility with old * servers --msa
 */

#define MAX_DATE_STRING 32	/* maximum string length for a date string */

#define	USERLEN		10
#define	REALLEN	 	50
#define	TOPICLEN	307
#define	CHANNELLEN	32
#define	PASSWDLEN 	63
#define	KEYLEN		23
#define	BUFSIZE		512	/* WARNING: *DONT* CHANGE THIS!!!! */
#define	MAXRECIPIENTS 	20
#define	MAXBANS	 	60
#define MAXEXEMPTS	60

#define MOTDLINELEN	90

#define        MAXSILES        10
#define        MAXSILELENGTH   128

#define MAXDCCALLOW 5
#define DCC_LINK_ME	0x01	/* This is my dcc allow */
#define DCC_LINK_REMOTE 0x02	/* I need to remove these dcc allows from these clients when I die */

#define	USERHOST_REPLYLEN	(NICKLEN+HOSTLEN+USERLEN+5)

/*
 * 'offsetof' is defined in ANSI-C. The following definition 
 * is not absolutely portable (I have been told), but so far
 * it has worked on all machines I have needed it. The type 
 * should be size_t but...
 * --msa
 */

#ifndef offsetof
#define	offsetof(t,m) (int)((&((t *)0L)->m))
#endif

#define	elementsof(x) (sizeof(x)/sizeof(x[0]))

/* flags for bootup options (command line flags) */

#define	BOOT_CONSOLE 1
#define	BOOT_QUICK	 2
#define	BOOT_DEBUG	 4
#define	BOOT_INETD	 8
#define	BOOT_TTY	   16
#define	BOOT_OPER	   32
#define	BOOT_AUTODIE 64
#define BOOT_STDERR	 128
#define	STAT_LOG	   -6	/* logfile for -x */
#define	STAT_MASTER	-5	/* Local ircd master before identification */
#define	STAT_CONNECTING	-4
#define	STAT_HANDSHAKE	-3
#define	STAT_ME		      -2
#define	STAT_UNKNOWN	-1
/* the line of truth lies here (truth == registeredness) */
#define	STAT_SERVER	0
#define	STAT_CLIENT	    1

/* status macros. */

#define	IsRegisteredUser(x)	((x)->status == STAT_CLIENT)
#define	IsRegistered(x)		((x)->status >= STAT_SERVER)
#define	IsConnecting(x)		((x)->status == STAT_CONNECTING)
#define	IsHandshake(x)		((x)->status == STAT_HANDSHAKE)
#define	IsMe(x)			((x)->status == STAT_ME)
#define	IsUnknown(x)		((x)->status == STAT_UNKNOWN || (x)->status == STAT_MASTER)
#define	IsServer(x)		((x)->status == STAT_SERVER)
#define	IsClient(x)		((x)->status == STAT_CLIENT)
#define	IsLog(x)		((x)->status == STAT_LOG)

#define	SetMaster(x)		((x)->status = STAT_MASTER)
#define	SetConnecting(x)	((x)->status = STAT_CONNECTING)
#define	SetHandshake(x)		((x)->status = STAT_HANDSHAKE)
#define	SetMe(x)		((x)->status = STAT_ME)
#define	SetUnknown(x)		((x)->status = STAT_UNKNOWN)
#define	SetServer(x)		((x)->status = STAT_SERVER)
#define	SetClient(x)		((x)->status = STAT_CLIENT)
#define	SetLog(x)		((x)->status = STAT_LOG)

#define	FLAGS_PINGSENT     	0x00000001	/* Unreplied ping sent */
#define	FLAGS_DEADSOCKET   	0x00000002	/* Local socket is dead--Exiting soon */
#define	FLAGS_KILLED       	0x00000004	/* Prevents "QUIT" from being sent for this */
#define	FLAGS_BLOCKED      	0x00000008	/* socket is in a blocked condition */
#define FLAGS_REJECT_HOLD  	0x00000010	/* client has been klined */
#define	FLAGS_CLOSING      	0x00000020	/* set when closing to suppress errors */
#define	FLAGS_LISTEN       	0x00000040	/* used to mark clients which we listen() on */
#define FLAGS_HAVERECVQ		0x00000080	/* Client has full commands in their recvq */
#define	FLAGS_DOINGDNS	   	0x00000100	/* client is waiting for a DNS response */
#define	FLAGS_AUTH	   	0x00000200	/* client is waiting on rfc931 response */
#define	FLAGS_WRAUTH	   	0x00000400	/* set if we havent writen to ident server */
#define	FLAGS_LOCAL	   	0x00000800	/* set for local clients */
#define	FLAGS_GOTID	   	0x00001000	/* successful ident lookup achieved */
#define	FLAGS_DOID	   	0x00002000	/* I-lines say must use ident return */
#define	FLAGS_NONL	   	0x00004000	/* No \n in buffer */
#define FLAGS_NORMALEX     	0x00008000	/* Client exited normally */
#define FLAGS_SENDQEX      	0x00010000	/* Sendq exceeded */
#define FLAGS_IPHASH       	0x00020000	/* iphashed this client */
#define FLAGS_ULINE 	   	0x00040000	/* client is U-lined */
#define FLAGS_USERBURST	   	0x00080000	/* server in nick/channel netburst */
#define FLAGS_TOPICBURST   	0x00100000	/* server in topic netburst */
#define FLAGS_BURST		(FLAGS_USERBURST | FLAGS_TOPICBURST)
#define FLAGS_SOBSENT      	0x00200000	/* we've sent an SOB, just have to send an EOB */
#define FLAGS_EOBRECV      	0x00400000	/* we're waiting on an EOB */
#define FLAGS_MAP     	   	0x00800000	/* Show this entry in /map */
#define FLAGS_BAD_DNS	   	0x01000000	/* spoofer-guy */
#define FLAGS_SERV_NEGO	   	0x02000000	/* This is a server that has passed connection tests,
						   but is a stat < 0 for handshake purposes */
#define FLAGS_RC4IN        	0x04000000	/* This link is rc4 encrypted. */
#define FLAGS_RC4OUT       	0x08000000	/* This link is rc4 encrypted. */
#define FLAGS_ZIPPED_IN	   	0x10000000	/* This link is gzipped. */
#define FLAGS_ZIPPED_OUT   	0x20000000	/* This link is gzipped. */
#define FLAGS_IPV6         	0x40000000	/* This link use IPV6 */


/* #define      SetUnixSock(x)          ((x)->flags |= FLAGS_UNIX) */

#define	IsListening(x)		((x)->flags & FLAGS_LISTEN)

#define	IsLocal(x)		((x)->flags & FLAGS_LOCAL)

#define	IsDead(x)		((x)->flags & FLAGS_DEADSOCKET)

#define	SetDNS(x)		((x)->flags |= FLAGS_DOINGDNS)
#define	DoingDNS(x)		((x)->flags & FLAGS_DOINGDNS)
#define	DoingAuth(x)		((x)->flags & FLAGS_AUTH)
#define	NoNewLine(x)		((x)->flags & FLAGS_NONL)


#define	ClearDNS(x)		((x)->flags &= ~FLAGS_DOINGDNS)
#define	ClearAuth(x)		((x)->flags &= ~FLAGS_AUTH)

#define SetNegoServer(x)	((x)->flags |= FLAGS_SERV_NEGO)
#define IsNegoServer(x)		((x)->flags & FLAGS_SERV_NEGO)
#define ClearNegoServer(x)	((x)->flags &= ~FLAGS_SERV_NEGO)
#define IsRC4OUT(x)		((x)->flags & FLAGS_RC4OUT)
#define SetRC4OUT(x)		((x)->flags |= FLAGS_RC4OUT)
#define IsRC4IN(x)		((x)->flags & FLAGS_RC4IN)
#define SetRC4IN(x)		((x)->flags |= FLAGS_RC4IN)
#define RC4EncLink(x)		(((x)->flags & (FLAGS_RC4IN|FLAGS_RC4OUT)) == (FLAGS_RC4IN|FLAGS_RC4OUT))

#define ZipIn(x)		((x)->flags & FLAGS_ZIPPED_IN)
#define SetZipIn(x)		((x)->flags |= FLAGS_ZIPPED_IN)
#define ZipOut(x)		((x)->flags & FLAGS_ZIPPED_OUT)
#define SetZipOut(x)		((x)->flags |= FLAGS_ZIPPED_OUT)

#define FLAGS2_SSL         	0x00000008	/* This link uses SSL*/
#define FLAGS2_KLINEEXEMPT	0x00000010	/* This client is exempt from klines/akills/glines */
#define FLAGS2_SUPEREXEMPT	0x00000020	/* This client is exempt from klines/akills/glines and class limits */
#define FLAGS2_RECVBURST	0x00000040	/* This server is linking to the network and we are awaiting EOBURST */
#define FLAGS2_EOBURST		0x00000080	/* This server have Ended its burst and is synched to network */

/* FLAGS2_SSL */
#define IsSSL(x)		((x)->flags2 & FLAGS2_SSL)
#define SetSSL(x)		((x)->flags2 |= FLAGS2_SSL)

/* FLAGS2_KLINEEXEMPT */
#define IsKLineExempt(x)	((x)->flags2 & FLAGS2_KLINEEXEMPT)
#define SetKLineExempt(x)	((x)->flags2 |= FLAGS2_KLINEEXEMPT)
#define ClearKLineExempt(x)	((x)->flags2 &= ~FLAGS2_KLINEEXEMPT)

/* FLAGS2_SUPEREXEMPT */
#define IsSuperExempt(x)	((x)->flags2 & FLAGS2_SUPEREXEMPT)
#define SetSuperExempt(x)	((x)->flags2 |= FLAGS2_SUPEREXEMPT)
#define ClearSuperExempt(x)	((x)->flags2 &= ~FLAGS2_SUPEREXEMPT)

#define IsExempt(x)		(IsKLineExempt(x) || IsSuperExempt(x))

/* FLAGS2 RECVEBURST */
#define IsWaitingForEOBurst(x)			((x)->flags2 & FLAGS2_RECVEOBURST)
#define SetWaitingForEOBurst(x)			((x)->flags2 |= FLAGS2_RECVEOBURST)
#define ClearWaitingForEOBurst(x)		((x)->flags2 &= ~FLAGS2_RECVEOBURST)

/* FLAGS2 EBURST */
#define IsEOBurst(x)		((x)->flags2 & FLAGS2_EOBURST)
#define SetEOBurst(x)		((x)->flags2 |= FLAGS2_EOBURST)
#define ClearEOBurst(x)		((x)->flags2 &= ~FLAGS2_EOBURST)


/* Capabilities of the ircd */


#define CAPAB_NOQUIT  0x0000002	/* Supports NOQUIT */
#define CAPAB_NSJOIN  0x0000004	/* server supports new smart sjoin */
#define CAPAB_BURST   0x0000008	/* server supports BURST command */
#define CAPAB_UNCONN  0x0000010	/* server supports UNCONNECT */

#define CAPAB_DKEY    0x0000040	/* server supports dh-key exchange using "DKEY" */
#define CAPAB_ZIP     0x0000080	/* server supports gz'd links */
#define CAPAB_DOZIP   0x0000100	/* output to this link shall be gzipped */
#define CAPAB_DODKEY  0x0000200	/* do I do dkey with this link? */
/* */
#define CAPAB_TS5     0x0000800	/* Supports the TS5 Protocol */
#define CAPAB_CLIENT  0x0001000 /* Supports CLIENT */
#define CAPAB_TSMODE  0x0002000 /* MODE's parv[2] is channel_p->channelts for channel mode */
#define CAPAB_IPV6    0x0004000 /* Server is able to handle ipv6 address masks */

#define CAPAB_SSJ5    0x0010000 /* Server supports smart join protocol 5 */



#define SetNoQuit(x) 	((x)->capabilities |= CAPAB_NOQUIT)
#define IsNoQuit(x) 	((x)->capabilities & CAPAB_NOQUIT)

#define SetSSJoin(x)	((x)->capabilities |= CAPAB_NSJOIN)
#define IsSSJoin(x)	((x)->capabilities & CAPAB_NSJOIN)

#define SetBurst(x)	((x)->capabilities |= CAPAB_BURST)
#define IsBurst(x)	((x)->capabilities & CAPAB_BURST)

#define SetUnconnect(x)	((x)->capabilities |= CAPAB_UNCONN)
#define IsUnconnect(x)	((x)->capabilities & CAPAB_UNCONN)


#define SetDKEY(x)	((x)->capabilities |= CAPAB_DKEY)
#define CanDoDKEY(x)    ((x)->capabilities & CAPAB_DKEY)
#define WantDKEY(x)	((x)->capabilities & CAPAB_DODKEY)	/* N: line, flag E */

#define SetZipCapable(x) ((x)->capabilities |= CAPAB_ZIP)
#define IsZipCapable(x)	((x)->capabilities & CAPAB_ZIP)
#define DoZipThis(x) 	((x)->capabilities & CAPAB_DOZIP)	/* this is set in N: line, flag Z */

#define SetTS5(x)	((x)->capabilities |= CAPAB_TS5)
#define IsTS5(x)		((x)->capabilities & CAPAB_TS5)

#define SetClientCapable(x)	((x)->capabilities |= CAPAB_CLIENT)
#define IsClientCapable(x)	((x)->capabilities & CAPAB_CLIENT)

#define SetTSMODE(x)    ((x)->capabilities |= CAPAB_TSMODE)
#define IsTSMODE(x)     ((x)->capabilities & CAPAB_TSMODE)

#define SetIpv6(x)    ((x)->capabilities |= CAPAB_IPV6)
#define IsIpv6(x)     ((x)->capabilities & CAPAB_IPV6)

#define SetSSJoin5(x)	((x)->capabilities |= CAPAB_SSJ5)
#define IsSSJoin5(x)	((x)->capabilities & CAPAB_SSJ5)


/* flag macros. */
#define IsULine(x) ((x)->flags & FLAGS_ULINE)
#define	FLAGS_ID (FLAGS_DOID|FLAGS_GOTID)

/* User Modes */
#define UMODE_o     	0x00000001	/* umode +o - Oper */
#define UMODE_O     	0x00000002	/* umode +O - Local Oper */
#define UMODE_i     	0x00000004	/* umode +i - Invisible */
#define UMODE_r     	0x00000008	/* umode +r - registered nick */
#define UMODE_a     	0x00000020	/* umode +a - Services Operator */
#define UMODE_h     	0x00000040	/* umode +h - Helper */
#define UMODE_x     	0x00000080	/* umode +x - hidden hostname */
#define UMODE_Z		0x00000100	/* umode +Z - Services Root Admin */
#define UMODE_P		0x00000200	/* umode +P - Services Admin */
#define UMODE_S		0x00000400	/* umode +S - Services Client */
#define UMODE_p		0x00000800	/* umode +p - Protected Oper */
#define UMODE_D		0x00001000	/* umode +D - has seen dcc warning message */
#define UMODE_d		0x00002000	/* umode +d - user is deaf to channel messages */
#define UMODE_R		0x00004000	/* umode +R - user only accept messages from registered users */
#define UMODE_W		0x00008000	/* umode +R - Oper will see /WHOIS notices */

#define	SEND_UMODES (UMODE_i|UMODE_o|UMODE_O|UMODE_r|UMODE_a|UMODE_h|UMODE_x|UMODE_Z|UMODE_P|UMODE_S|UMODE_p|UMODE_D|UMODE_d|UMODE_R|UMODE_W)
#define ALL_UMODES (SEND_UMODES|UMODE_D)

/* UMODE_O */
#define	IsLocOp(x)		((x)->umode & UMODE_O)
#define	SetLocOp(x)    		((x)->umode |= UMODE_O)
#define ClearLocOp(x)		((x)->umode &= ~UMODE_O)

/* UMODE_o */
#define	IsOper(x)		((x)->umode & UMODE_o)
#define	SetOper(x)		((x)->umode |= UMODE_o)
#define	ClearOper(x)		((x)->umode &= ~UMODE_o)

/* UMODE_a */
#define IsServicesOper(x)   	((x)->umode & UMODE_a)
#define SetServicesOper(x)  	((x)->umode |= UMODE_a)
#define ClearServicesOper(x)  	((x)->umode &= ~UMODE_a)

/* UMODE_P */
#define IsServicesAdmin(x)	((x)->umode & UMODE_P)
#define SetServicesAdmin(x)	((x)->umode |= UMODE_P)
#define ClearServicesAdmin(x)	((x)->umode &= ~UMODE_P)

/* UMODE_Z */
#define IsServicesRoot(x)	((x)->umode & UMODE_Z)
#define SetServicesRoot(x)	((x)->umode |= UMODE_Z)
#define ClearServicesRoot(x)	((x)->umode &= ~UMODE_Z)

/* UMODE_i */
#define	IsInvisible(x)		((x)->umode & UMODE_i)
#define	SetInvisible(x)		((x)->umode |= UMODE_i)
#define	ClearInvisible(x)	((x)->umode &= ~UMODE_i)

/* UMODE_r */
#define IsARegNick(x)   	((x)->umode & (UMODE_r))
#define IsRegNick(x)    	((x)->umode & UMODE_r)
#define SetRegNick(x)   	((x)->umode |= UMODE_r)

/* UMODE_h */
#define IsHelpOp(x)		((x)->umode & UMODE_h)
#define SetHelpOp(x)		((x)->umode |= UMODE_h)
#define ClearHelpOp(x)		((x)->umode &= ~UMODE_h)

/* UMODE_x */
#define IsHidden(x)             ((x)->umode & UMODE_x)
#define SetHidden(x)            ((x)->umode |= UMODE_x)
#define ClearHidden(x)		((x)->umode &= ~UMODE_x)

/* UMODE_S */
#define IsServicesClient(x)	((x)->umode & UMODE_S)
#define SetServicesClient(x)	((x)->umode |= UMODE_S)
#define ClearServicesClient(x)	((x)->umode &= ~UMODE_S)

/* UMODE_p */
#define IsProtectedOper(x)	((x)->umode & UMODE_p)
#define SetProtectedOper(x)	((x)->umode |= UMODE_p)
#define ClearProtectedOper(x)	((x)->umode &= ~UMODE_p)

/* UMODE_D */
#define SendSeenDCCNotice(x)    ((x)->umode & UMODE_D)
#define SetSeenDCCNotice(x)   	((x)->umode |= UMODE_D)
#define ClearSeenDCCNotice(x)	((x)->umode &= ~UMODE_D)

/* UMODE_d */
#define IsDeaf(x)		((x)->umode & UMODE_d)
#define SetDeaf(x)		((x)->umode |= UMODE_d)
#define ClearDeaf(x)		((x)->umode &= ~UMODE_d)

/* UMODE_R */
#define IsNoNonReg(x)		((x)->umode & UMODE_R)
#define SetNoNonReg(x)		((x)->umode |= UMODE_R)
#define ClearNoNonReg(x)	((x)->umode &= ~UMODE_R)

/* UMODE_W */
#define WantsWhois(x)		((x)->umode & UMODE_W)
#define SetWantsWhois(x)	((x)->umode |= UMODE_W)
#define ClearWantsWhois(x)	((x)->umode &= ~UMODE_W)



/* sModes */
#define SMODE_s		0x00000001	/* smode +s - Client is connected using SSL */
#define SMODE_6		0x00000002	/* smode +6 - Client is connected via IPV6 */
#define SMODE_a		0x00000004	/* smode +a - Aerver Co Admin */
#define SMODE_A		0x00000008	/* smode +A - Server Admin */
#define SMODE_t		0x00000010	/* smode +t - Technical Co Admin */
#define SMODE_T		0x00000020	/* smode +T - Technical Admin */
#define SMODE_n		0x00000040	/* smode +n - Network Co Admin */
#define SMODE_N		0x00000080	/* smode +N - Network Admin */
#define SMODE_G		0x00000100	/* smode +G - Guest Admin */


#define SEND_SMODES (SMODE_s|SMODE_a|SMODE_A|SMODE_t|SMODE_T|SMODE_n|SMODE_N|SMODE_G)

/* SMODE_s */
#define IsSSLClient(x)		((x)->smode & SMODE_s)
#define SetSSLClient(x)		((x)->smode |= SMODE_s)

/* SMODE_6 */
#define IsIPV6Client(x)		((x)->smode & SMODE_6)
#define SetIPV6Client(x)	((x)->smode |= SMODE_6)

/* SMODE_a */
#define IsServerCoAdmin(x)	((x)->smode & SMODE_a)
#define SetServerCoAdmin(x)	((x)->smode |= SMODE_a)
#define ClearServerCoAdmin(x)	((x)->smode &= ~SMODE_a)

/* SMODE_A */
#define IsServerAdmin(x)    	((x)->smode & SMODE_A)
#define SetServerAdmin(x)	((x)->smode |= SMODE_A)
#define ClearServerAdmin(x)   	((x)->smode &= ~SMODE_A)

/* SMODE_t */
#define IsTechCoAdmin(x)    	((x)->smode & SMODE_t)
#define SetTechCoAdmin(x)	((x)->smode |= SMODE_t)
#define ClearTechCoAdmin(x)   	((x)->smode &= ~SMODE_t)

/* SMODE_T */
#define IsTechAdmin(x)    	((x)->smode & SMODE_T)
#define SetTechAdmin(x)		((x)->smode |= SMODE_T)
#define ClearTechAdmin(x)   	((x)->smode &= ~SMODE_T)

/* SMODE_n */
#define IsNetCoAdmin(x)		((x)->smode & SMODE_n)
#define SetNetCoAdmin(x)	((x)->smode |= SMODE_n)
#define ClearNetCoAdmin(x)	((x)->smode &= ~SMODE_n)

/* SMODE_N */
#define IsNetAdmin(x)		((x)->smode & SMODE_N)
#define SetNetAdmin(x)		((x)->smode |= SMODE_N)
#define ClearNetAdmin(x)	((x)->smode &= ~SMODE_N)

/* SMODE_G */
#define IsGuestAdmin(x)		((x)->smode & SMODE_G)
#define SetGuestAdmin(x)	((x)->smode |= SMODE_G)
#define ClearGuestAdmin(x)	((x)->smode &= ~SMODE_G)


/* NMODE_s */

#define NMODE_w     	0x00000001	/* nmode +w - Recieve Wallops */
#define NMODE_s     	0x00000002	/* nmode +s - Server Notices */
#define NMODE_c     	0x00000004	/* nmode +c - Local Client Connect/Exit Notices */
#define NMODE_C		0x00000008	/* nmode +C - Global Client Connect/Exit Notices */
#define NMODE_k     	0x00000010	/* nmode +k - Server Originating Kill Notices */
#define NMODE_f     	0x00000020	/* nmode +f - Server Flood Notices */
#define NMODE_d		0x00000040	/* nmode +d - Debug Notices */
#define NMODE_n     	0x00000080	/* nmode +n - Network Info Notices */
#define NMODE_N     	0x00000100	/* nmode +N - Network Global Notices */
#define NMODE_r		0x00000200	/* nmode +r - Routing Notices */
#define NMODE_R		0x00000400	/* nmode +R - Reject Notices */
#define NMODE_g		0x00000800	/* nmode +g - Various Oper Chat Notices */
#define NMODE_G		0x00001000	/* nmode +G - Globops Notices */
#define NMODE_b		0x00002000	/* nmode +b - Blocked DCC Send Notices */
#define NMODE_B		0x00004000	/* nmode +B - Spam Bot Notices */
#define NMODE_S		0x00008000	/* nmode +S - "Spy" Notices */

/* NMODE_w */
#define	SendWallops(x)		((x)->nmode & NMODE_w)
#define	SetWallops(x)  		((x)->nmode |= NMODE_w)
#define	ClearWallops(x)		((x)->nmode &= ~NMODE_w)

/* NMODE_s */
#define	SendServerNotice(x)	((x)->nmode & NMODE_s)
#define SetServerNotice(x)	((x)->nmode |= NMODE_s)
#define ClearServerNotice(x)	((x)->nmode &= ~NMODE_s)

/* NMODE_c */
#define SendConnectNotice(x)	((x)->nmode & NMODE_c)
#define SetConnectNotice(x)	((x)->nmode |= NMODE_c)
#define ClearConnectNotice(x)  	((x)->nmode &= ~NMODE_c)

/* NMODE_C */
#define SendGConnectNotice(x)	((x)->nmode & NMODE_C)
#define SetGConnectNotice(x)	((x)->nmode |= NMODE_C)
#define ClearGConnectNotice(x)  ((x)->nmode &= ~NMODE_C)

/* NMODE_k */
#define SendSkillNotice(x)	((x)->nmode & NMODE_k)
#define SetSkillNotice(x)	((x)->nmode |= NMODE_k)
#define ClearSkillNotice(x)	((x)->nmode &= ~NMODE_k)

/* NMODE_f */
#define SendFloodNotice(x)  	((x)->nmode & NMODE_f)
#define SetFloodNotice(x)	((x)->nmode |= NMODE_f)
#define ClearFloodNotice(x)  	((x)->nmode &= ~NMODE_f)

/* NMODE_d */
#define SendDebugNotice(x)	((x)->nmode & NMODE_d)
#define SetDebugNotice(x)	((x)->nmode |= NMODE_d)
#define ClearDebugNotice(x)  	((x)->nmode &= ~NMODE_d)

/* NMODE_n */
#define SendNetInfo(x)   	((x)->nmode & NMODE_n)
#define SetNetInfo(x)		((x)->nmode |= NMODE_n)
#define ClearNetInfo(x)  	((x)->nmode &= ~NMODE_n)

/* NMODE_N */
#define SendNetGlobal(x)   	((x)->nmode & NMODE_N)
#define SetNetGlobal(x)		((x)->nmode |= NMODE_N)
#define ClearNetGlobal(x)  	((x)->nmode &= ~NMODE_N)

/* NMODE_r */
#define SendRoutingNotice(x)   	((x)->nmode & NMODE_r)
#define SetRoutingNotice(x)	((x)->nmode |= NMODE_r)
#define ClearRoutingNotice(x)  	((x)->nmode &= ~NMODE_r)

/* NMODE_R */
#define SendRejectNotice(x)	((x)->nmode & NMODE_R)
#define SetRejectNotice(x)	((x)->nmode |= NMODE_R)
#define ClearRejectNotice(x)  	((x)->nmode &= ~NMODE_R)

/* NMODE_g */
#define SendChatops(x) 		((x)->nmode & NMODE_g)
#define SetChatops(x)		((x)->nmode |= NMODE_g)
#define ClearChatops(x)  	((x)->nmode &= ~NMODE_g)

/* NMODE_G */
#define SendGlobops(x) 		((x)->nmode & NMODE_G)
#define SetGlobops(x)		((x)->nmode |= NMODE_G)
#define ClearGlobops(x)		((x)->nmode &= ~NMODE_G)

/* NMODE_b */
#define SendDCCNotice(x)	((x)->nmode & NMODE_b)
#define SetDCCNotice(x)		((x)->nmode |= NMODE_b)
#define ClearDCCNotice(x)	((x)->nmode &= ~NMODE_b)

/* NMODE_B */
#define SendSpamNotice(x)	((x)->nmode & NMODE_B)
#define SetSpamNotice(x)	((x)->nmode |= NMODE_B)
#define ClearSpamNotice(x)	((x)->nmode &= ~NMODE_B)

/* NMODE_S */
#define SendSpyNotice(x)	((x)->nmode & NMODE_S)
#define SetSpyNotice(x)		((x)->nmode |= NMODE_S)
#define ClearSpyNotice(x)	((x)->nmode &= ~NMODE_S)

#define ALL_NMODES (NMODE_w|NMODE_s|NMODE_c|NMODE_C|NMODE_k|NMODE_f|NMODE_d|NMODE_n|NMODE_N|NMODE_r|NMODE_R|NMODE_g|NMODE_G|NMODE_b|NMODE_B|NMODE_S)


#define LOCOP_UMODES (UMODE_O|UMODE_D|UMODE_x|UMODE_h)


/* for sendto_realops_lev */

#define CCONN_LEV	1
#define REJ_LEV		2
#define SKILL_LEV	3
#define SPY_LEV		4
#define DEBUG_LEV	5
#define FLOOD_LEV 	6
#define SPAM_LEV 	7
#define DCCSEND_LEV	8

/* Collective Macros */
#define	IsAnOper(x)		((x)->umode & (UMODE_o|UMODE_O))
#define IsSkoAdmin(x) 		(IsServerCoAdmin(x) || IsServerAdmin(x) || IsTechCoAdmin(x) || IsTechAdmin(x)|| IsNetCoAdmin(x) || IsNetAdmin(x) || IsGuestAdmin(x))
#define IsSkoServicesStaff(x)	(IsServicesOper(x) || IsServicesAdmin(x) || IsServicesRoot(x))
#define IsSkoTechAdmin(x)	(IsTechAdmin(x) || IsTechCoAdmin(x))
#define IsSkoNetAdmin(x)	(IsNetAdmin(x) || IsNetCoAdmin(x))


#define	IsPerson(x)		((x)->user && IsClient(x))

#define	IsPrivileged(x)		(IsAnOper(x) || IsServer(x))

/* flags2 macros. */

#define IsRestricted(x)		((x)->flags & FLAGS_RESTRICTED)
#define SetRestricted(x)	((x)->flags |= FLAGS_RESTRICTED)
/* Oper flags */

/* defined operator access levels */
#define OFLAG_REHASH		0x00000001	/* Oper can /rehash server */
#define OFLAG_DIE		0x00000002	/* Oper can /die the server */
#define OFLAG_RESTART		0x00000004	/* Oper can /restart the server */
#define OFLAG_CANEDCONF		0x00000008	/* Oper can edit the config */
#define OFLAG_HELPOP		0x00000010	/* Oper can send /HelpOps */
#define OFLAG_WALLOP		0x00000020	/* Oper can send /WallOps */
#define OFLAG_LROUTE		0x00000040	/* Oper can do local routing */
#define OFLAG_GROUTE		0x00000080	/* Oper can do global routing */
#define OFLAG_LKILL		0x00000100	/* Oper can do local kills */
#define OFLAG_GKILL		0x00000200	/* Oper can do global kills */
#define OFLAG_KLINE		0x00000400	/* Oper can /kline users */
#define OFLAG_LNOTICE		0x00000800	/* Oper can send local serv notices */
#define OFLAG_GNOTICE		0x00001000	/* Oper can send global notices */
#define OFLAG_PROTECTEDOPER	0x00002000	/* Protectec Operator */
#define OFLAG_SERVCOADMIN	0x00004000	/* Server Co Admin */
#define OFLAG_SERVADMIN		0x00008000	/* Server Admin */
#define OFLAG_NETADMIN		0x00010000	/* Network Admin */
#define OFLAG_NETCOADMIN	0x00020000	/* Network Co Admin */
#define OFLAG_GUESTADMIN	0x00040000	/* Guest Admin */
#define OFLAG_SERVICESOPER	0x00080000	/* Oper can be a services operator */
#define OFLAG_SERVICESADMIN	0x00100000	/* Services Admin */
#define OFLAG_SERVICESROOT	0x00200000	/* Services Root */
#define OFLAG_TECHCOADMIN	0x00400000	/* Network Tech Co Admin */
#define OFLAG_TECHADMIN	0x00800000	/* Network Tech Admin */


#define OPCanRehash(x)		((x)->oflag & OFLAG_REHASH)
#define OPSetRehash(x)		((x)->oflag |= OFLAG_REHASH)
#define OPClearRehash(x)	((x)->oflag &= ~OFLAG_REHASH)

#define OPCanDie(x)		((x)->oflag & OFLAG_DIE)
#define OPSetDie(x)		((x)->oflag |= OFLAG_DIE)
#define OPClearDie(x)		((x)->oflag &= ~OFLAG_DIE)

#define OPCanRestart(x)		((x)->oflag & OFLAG_RESTART)
#define OPSetRestart(x)		((x)->oflag |= OFLAG_RESTART)
#define OPClearRestart(x)	((x)->oflag &= ~OFLAG_RESTART)

#define OPCanHelpOp(x)		((x)->oflag & OFLAG_HELPOP)
#define OPSetHelpOp(x)		((x)->oflag |= OFLAG_HELPOP)
#define OPClearHelpOp(x)	((x)->oflag &= ~OFLAG_HELPOP)


#define OPCanWallOps(x)		((x)->oflag & OFLAG_WALLOP)
#define OPSetWallOps(x)		((x)->oflag |= OFLAG_WALLOP)
#define OPClearWallOps(x)	((x)->oflag &= ~OFLAG_WALLOP)


#define OPCanLRoute(x)		((x)->oflag & OFLAG_LROUTE)
#define OPSetLRoute(x)		((x)->oflag |= OFLAG_LROUTE)
#define OPClearLRoute(x)	((x)->oflag &= ~OFLAG_LROUTE)

#define OPCanGRoute(x)		((x)->oflag & OFLAG_GROUTE)
#define OPSetGRoute(x)		((x)->oflag |= OFLAG_GROUTE)
#define OPClearGRoute(x)	((x)->oflag &= ~OFLAG_GROUTE)

#define OPCanLKill(x)		((x)->oflag & OFLAG_LKILL)
#define OPSetLKill(x)		((x)->oflag |= OFLAG_LKILL)
#define OPClearLKill(x)		((x)->oflag &= ~OFLAG_LKILL)

#define OPCanGKill(x)		((x)->oflag & OFLAG_GKILL)
#define OPSetGKill(x)		((x)->oflag |= OFLAG_GKILL)
#define OPClearGKill(x)		((x)->oflag &= ~OFLAG_GKILL)

#define OPCanKline(x)		((x)->oflag & OFLAG_KLINE)
#define OPSetKline(x)		((x)->oflag |= OFLAG_KLINE)
#define OPClearKline(x)		((x)->oflag &= ~OFLAG_KLINE)


#define OPCanLNotice(x)		((x)->oflag & OFLAG_LNOTICE)
#define OPSetLNotice(x)		((x)->oflag |= OFLAG_LNOTICE)
#define OPClearLNotice(x)	((x)->oflag &= ~OFLAG_LNOTICE)

#define OPCanGNotice(x)		((x)->oflag & OFLAG_GNOTICE)
#define OPSetGNotice(x)		((x)->oflag |= OFLAG_GNOTICE)
#define OPClearGNotice(x)	((x)->oflag &= ~OFLAG_GNOTICE)



#define OPIsProtected(x)	((x)->oflag & OFLAG_PROTECTEDOPER)
#define OPSetProtected(x)	((x)->oflag |= OFLAG_PROTECTEDOPER)
#define OPClearProtected(x)	((x)->oflag &= ~OFLAG_PROTECTEDOPER)

#define OPCanEditConf(x)	((x)->oflag & OFLAG_CANEDCONF)
#define OPSetCanEditConf(x)	((x)->oflag |= OFLAG_CANEDCONF)
#define OPClearCanEditConf(x)	((x)->oflag &= ~OFLAG_CANEDCONF)

#define OPIsGuestAdmin(x)	((x)->oflag & OFLAG_GUESTADMIN)
#define OPSetGuestAdmin(x)	((x)->oflag |= OFLAG_GUESTADMIN)
#define OPClearGuestAdmin(x)	((x)->oflag &= ~OFLAG_GUESTADMIN)

#define OPIsServerCoAdmin(x)	((x)->oflag & OFLAG_SERVCOADMIN)
#define OPSetServerCoAdmin(x)	((x)->oflag |= OFLAG_SERVCOADMIN)
#define OPClearServerCoAdmin(x)	((x)->oflag &= ~OFLAG_SERVCOADMIN)

#define OPIsServerAdmin(x)	((x)->oflag & OFLAG_SERVADMIN)
#define OPSetServerAdmin(x)	((x)->oflag |= OFLAG_SERVADMIN)
#define OPClearServerAdmin(x)	((x)->oflag &= ~OFLAG_SERVADMIN)

#define OPIsNetCoAdmin(x)	((x)->oflag & OFLAG_NETCOADMIN)
#define OPSetNetCoAdmin(x)	((x)->oflag |= OFLAG_NETCOADMIN)
#define OPClearNetCoAdmin(x)	((x)->oflag &= ~OFLAG_NETCOADMIN)

#define OPIsNetAdmin(x)		((x)->oflag & OFLAG_NETADMIN)
#define OPSetNetAdmin(x)	((x)->oflag |= OFLAG_NETADMIN)
#define OPClearNetAdmin(x)	((x)->oflag &= ~OFLAG_NETADMIN)

#define OPIsServicesOper(x)	((x)->oflag & OFLAG_SERVICESOPER)
#define OPSetServicesOper(x) 	((x)->oflag |= OFLAG_SERVICESOPER)
#define OPClearServicesOper(x)	((x)->oflag &= ~OFLAG_SERVICESOPER)

#define OPIsServicesAdmin(x)	((x)->oflag & OFLAG_SERVICESADMIN)
#define OPSetServicesAdmin(x)	((x)->oflag |= OFLAG_SERVICESADMIN)
#define OPClearServicesAdmin(x)	((x)->oflag &= ~ OFLAG_SERVICESADMIN)

#define OPIsServicesRoot(x)	((x)->oflag & OFLAG_SERVICESROOT)
#define OPSetServicesRoot(x)	((x)->oflag |= OFLAG_SERVICESROOT)
#define OPClearServicesRoot(x)	((x)->oflag &= ~OFLAG_SERVICESROOT)

#define OpIsTechCoAdmin(x)	((x)->oflag & OFLAG_TECHCOADMIN)
#define OpSetTechCoAdmin(x)	((x)->oflag |= OFLAG_TECHCOADMIN)
#define OpClearTechCoAdmin(x)	((x)->oflag &= ~OFLAG_TECHCOADMIN

#define OpIsTechAdmin(x)	((x)->oflag & OFLAG_TECHADMIN)
#define OpSetTechAdmin(x)	((x)->oflag |= OFLAG_TECHADMIN)
#define OpClearTechAdmin(x)	((x)->oflag &= ~OFLAG_TECHADMIN

#define OFLAG_LOCAL	(OFLAG_REHASH|OFLAG_HELPOP|OFLAG_WALLOP|OFLAG_LROUTE|OFLAG_LKILL|OFLAG_KLINE|OFLAG_LNOTICE)
#define OFLAG_GLOBAL	(OFLAG_REHASH|OFLAG_HELPOP|OFLAG_WALLOP|OFLAG_LROUTE|OFLAG_LKILL|OFLAG_KLINE|OFLAG_LNOTICE|OFLAG_GROUTE|OFLAG_GKILL|OFLAG_GNOTICE)
#define OFLAG_ISGLOBAL	(OFLAG_GROUTE|OFLAG_GKILL|OFLAG_GNOTICE)


/* defined debugging levels */
#define	DEBUG_FATAL  0
#define	DEBUG_ERROR  1		/* report_error() and other errors that are found */
#define	DEBUG_NOTICE 3
#define	DEBUG_DNS    4		/* used by all DNS related routines - a *lot* */
#define	DEBUG_INFO   5		/* general usful info */
#define	DEBUG_NUM    6		/* numerics */
#define	DEBUG_SEND   7		/* everything that is sent out */
#define	DEBUG_DEBUG  8		/* anything to do with debugging, ie unimportant :) */
#define	DEBUG_MALLOC 9		/* malloc/free calls */
#define	DEBUG_LIST  10		/* debug list use */
/* defines for curses in client */
#define	DUMMY_TERM	0
#define	CURSES_TERM	1
#define	TERMCAP_TERM	2

/*
 *  IPv4/IPv6 ?
 */

char mydummy[64];
char mydummy2[64];

#ifdef INET6

# define WHOSTENTP(x) ((x)[0]|(x)[1]|(x)[2]|(x)[3]|(x)[4]|(x)[5]|(x)[6]|(x)[7]|(x)[8]|(x)[9]|(x)[10]|(x)[11]|(x)[12]|(x)[13]|(x)[14]|(x)[15])

# define	AFINET		AF_INET6
# define	SOCKADDR_IN	sockaddr_in6
# define	SOCKADDR	sockaddr
# define	SIN_FAMILY	sin6_family
# define	SIN_PORT	sin6_port
# define	SIN_ADDR	sin6_addr
# define	S_ADDR		s6_addr
# define	IN_ADDR		in6_addr
# define	INADDRANY_STR	"0::0"

# if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(bsdi)
#  ifndef s6_laddr
#   define s6_laddr        s6_addr32
#  endif
# endif

#else
# define	AFINET		AF_INET
# define	SOCKADDR_IN	sockaddr_in
# define	SOCKADDR	sockaddr
# define	SIN_FAMILY	sin_family
# define	SIN_PORT	sin_port
# define	SIN_ADDR	sin_addr
# define	S_ADDR		s_addr
# define	IN_ADDR		in_addr
# define	INADDRANY_STR	"0.0.0.0"

# define WHOSTENTP(x) (x)
#endif

struct Counter
{
  int server;			/* servers */
  int myserver;			/* my servers */
  int myulined;			/* my ulined servers */
  int oper;			/* Opers */
  int chan;			/* Channels */
  int local;			/* Local Clients */
  int total;			/* total clients */
  int invisi;			/* invisible clients */
  int unknown;			/* unknown connections */
  int max_loc;			/* MAX local clients */
  int max_tot;			/* MAX global clients */
  ts_val start;			/* when we started collecting info */
  u_long today;			/* Client Connections today */
  ts_val day;			/* when today started */
  u_long weekly;		/* connections this week */
  ts_val week;			/* when this week started */
  u_long monthly;		/* connections this month */
  ts_val month;			/* when this month started */
  u_long yearly;		/* this is gonna be big */
  ts_val year;			/* when this year started (HEH!) */
};

struct MotdItem
{
  char line[MOTDLINELEN];
  struct MotdItem *next;
};
struct OMotdItem
{
  char line[MOTDLINELEN];
  struct OMotdItem *next;
};
struct RulesItem
{
  char line[MOTDLINELEN];
  struct RulesItem *next;
};



/*
 * lets speed this up... also removed away information. *tough* Dianora
 */
typedef struct Whowas
{
  int hashv;
  char name[NICKLEN + 1];
  char username[USERLEN + 1];
  char hostname[HOSTLEN + 1];
  char virthostname[HOSTLEN + 1];
  char hostipname[HOSTLEN + 1];
  int washidden;
  char *servername;
  char realname[REALLEN + 1];
  time_t logoff;
  struct Client *online;	/* Pointer to new nickname for chasing or NULL */

  struct Whowas *next;		/* for hash table... */

  struct Whowas *prev;		/* for hash table... */
  struct Whowas *cnext;		/* for client struct linked list */
  struct Whowas *cprev;		/* for client struct linked list */
}
aWhowas;

struct ConfItem
{
  unsigned int status;		/* If CONF_ILLEGAL, delete when no clients */
  unsigned int flags;		/* i-lines and akills use this */
  int clients;			/* Number of *LOCAL* clients using this */
  struct IN_ADDR ipnum; 	/* ip number of host field */
  char *host;
  char *localhost;
  char *passwd;
  char *name;
  int port;
  time_t hold;			/* Hold action until this time (calendar time) */
#ifdef USE_ADNS
  struct DNSQuery *dnslookup;    /* if we are looking something up, this is the struct */
#endif
  aClass *class;		/* Class of connection */
  struct ConfItem *next;
};

#define	CONF_ILLEGAL	0x80000000
#define	CONF_MATCH		0x40000000

/* #define      CONF_QUARANTINED_SERVER 0x0001 */
#define	CONF_CLIENT		0x00000002
#define	CONF_CONNECT_SERVER	0x00000004
#define	CONF_NOCONNECT_SERVER	0x00000008
#define	CONF_LOCOP		0x00000010
#define	CONF_OPERATOR		0x00000020
#define	CONF_ME			0x00000040
#define	CONF_KILL		0x00000080
#define	CONF_ADMIN		0x00000100
#define	CONF_CLASS		0x00000200
#define	CONF_SERVICE		0x00000400
#define	CONF_LEAF		0x00000800
#define	CONF_LISTEN_PORT	0x00001000
#define	CONF_HUB		0x00002000
#define CONF_ELINE		0x00004000
#define CONF_FLINE		0x00008000

#define CONF_QUARANTINED_NICK 	0x00040000
#define CONF_ULINE 		0x00080000
#define CONF_DRPASS		0x00100000	/* die/restart pass, from df465 */

#define CONF_SQLINE     	0x00400000
#define CONF_GCOS               0x00800000
#define CONF_SGLINE             0x01000000

#define CONF_VHOST		0x04000000
#define CONF_QUARANTINED_CHAN	0x08000000
#define CONF_NETWORKADMIN	0x10000000
#define CONF_QUARANTINE         (CONF_QUARANTINED_NICK|CONF_QUARANTINED_CHAN)
#define	CONF_OPS		(CONF_OPERATOR | CONF_LOCOP)
#define	CONF_SERVER_MASK	(CONF_CONNECT_SERVER | CONF_NOCONNECT_SERVER)
#define	CONF_CLIENT_MASK	(CONF_CLIENT | CONF_SERVICE | CONF_OPS | CONF_SERVER_MASK)
#define	IsIllegal(x)	((x)->status & CONF_ILLEGAL)
#ifdef LITTLE_I_LINES
#define CONF_FLAGS_LITTLE_I_LINE	0x0001
#endif

/* Client structures */
struct User
{
  Link *channel;		/* chain of channel pointer blocks */
  Link *invited;		/* chain of invite pointer blocks */
  char *away;			/* pointer to away message */
  time_t last;
  int joined;			/* number of channels joined */
  char username[USERLEN + 1];
  char host[HOSTLEN + 1];
  char virthost[HOSTLEN + 1];
  char *server;			/* pointer to scached server name */
#ifdef OS_SOLARIS
  uint_t servicestamp;		/* solaris is gay -epi */
#else
  u_int32_t servicestamp;	/* Services id - Raistlin */
#endif
  /*
   * In a perfect world the 'server' name should not be needed, a
   * pointer to the client describing the server is enough. 
   * Unfortunately, in reality, server may not yet be in links while
   * USER is introduced... --msa
   */
  Link *silence;		/* chain of silenced users */
  LOpts *lopt;			/* Saved /list options */
  Link *dccallow;		/* chain of dcc send allowed users */
};

struct Server
{
  char *up;			/* Pointer to scache name */
  char bynick[NICKLEN + 1];
  char byuser[USERLEN + 1];
  char byhost[HOSTLEN + 1];
  aConfItem *nline;		/* N-line pointer for this server */
  int dkey_flags;		/* dkey flags */
#ifdef HAVE_ENCRYPTION_ON
  void *sessioninfo_in;		/* pointer to opaque sessioninfo structure */
  void *sessioninfo_out;	/* pointer to opaque sessioninfo structure */
  void *rc4_in;			/* etc */
  void *rc4_out;		/* etc */
#endif
  void *zip_out;
  void *zip_in;
};

/* Increment this number every time you change order or number of things in aclient */
#define ACLIENT_SERIAL 100001

struct Client
{
  struct Client *next, *prev, *hnext;
  anUser *user;			/* ...defined, if this is a User */
  aServer *serv;		/* ...defined, if this is a server */
  aWhowas *whowas;		/* Pointers to whowas structs */
  time_t lasttime;		/* ...should be only LOCAL clients? --msa */
  time_t firsttime;		/* time client was created */
  time_t since;			/* last time we parsed something */
  ts_val tsinfo;		/* TS on the nick, SVINFO on servers */
  long flags;			/* client flags */
  long flags2;			/* More client flags. Ran out of space - ShadowMaster */
  long umode;			/* uMode flags */
  long smode;			/* sMode flags */
  long nmode;			/* nMode flags */
  aClient *from;		/* == self, if Local Client, *NEVER* NULL! */
  aClient *uplink;		/* this client's uplink to the network */
  int fd;			/* >= 0, for local clients */
  int hopcount;			/* number of servers to this 0 = local */
  short status;			/* Client type */
  char nicksent;
  char name[HOSTLEN + 1];	/* Unique name of the client, nick or host */
  char info[REALLEN + 1];	/* Free form additional client information */
#ifdef FLUD
  Link *fludees;
#endif
  struct IN_ADDR ip;		/* keep real ip# too */
  char hostip[HOSTIPLEN + 1];	/* Keep real ip as string too - Dianora */

  Link *watch;			/* user's watch list */
  int watches;			/* how many watches this user has set */

#ifdef USE_SSL
    SSL *ssl;
    X509 *client_cert;
#endif /*SSL*/

  /*
   * The following fields are allocated only for local clients 
   * (directly connected to this server with a socket.  The first
   * of them MUST be the "count"--it is the field to which the
   * allocation is tied to! Never refer to  these fields, if (from != self).
   */

  int count;			/* Amount of data in buffer */
#ifdef FLUD
  time_t fludblock;
  struct fludbot *fluders;
#endif
#ifdef ANTI_SPAMBOT
  time_t last_join_time;	/* when this client last joined a channel */
  time_t last_leave_time;	/* when this client last left a channel */
  int join_leave_count;		/*
				 * count of JOIN/LEAVE in less 
				 * than MIN_JOIN_LEAVE_TIME seconds */
  int oper_warn_count_down;	/*
				 * warn opers of this possible spambot 
				 * every time this gets to 0 */
#endif
#ifdef ANTI_DRONE_FLOOD
  time_t            first_received_message_time;
  int               received_number_of_privmsgs;
  int               drone_noticed;
#endif
  char buffer[BUFSIZE];		/* Incoming message buffer */
  short lastsq;			/* # of 2k blocks when sendqueued called last */
  struct DBuf sendQ;		/* Outgoing message queue--if socket full */
  struct DBuf recvQ;		/* Hold for data incoming yet to be parsed */
  long sendM;			/* Statistics: protocol messages send */
  long sendK;			/* Statistics: total k-bytes send */
  long receiveM;		/* Statistics: protocol messages received */
  long receiveK;		/* Statistics: total k-bytes received */
  u_short sendB;		/* counters to count upto 1-k lots of bytes */
  u_short receiveB;		/* sent and received. */
  long lastrecvM;		/* to check for activity --Mika */
  int priority;
  aClient *acpt;		/* listening client which we accepted from */
  Link *confs;			/* Configuration record associated */
  int authfd;			/* fd for rfc931 authentication */

  char username[USERLEN + 1];	/* username here now for auth stuff */
  unsigned short port;		/* and the remote port# too :-) */
#ifndef USE_ADNS
  struct hostent *hostp;
#endif
#ifdef ANTI_NICK_FLOOD
  time_t last_nick_change;
  int number_of_nick_changes;
#endif
#ifdef NO_AWAY_FLUD
  time_t alas;			/* last time of away set */
  int acount;			/* count of away settings */
#endif

  char sockhost[HOSTLEN + 1];	/*
				 * This is the host name from
				 * the socket and after which the
				 * connection was accepted. */
  char passwd[PASSWDLEN + 1];
  /* try moving this down here to prevent weird problems... ? */
  int oflag;			/* Operator Flags */
  int sockerr;			/* what was the last error returned for this socket? */
  int capabilities;		/* what this server/client supports */
  int pingval;			/* cache client class ping value here */
  int sendqlen;			/* cache client max sendq here */
#ifdef USE_ADNS
  aDNSQuery *dnslookup;		/* adns callback structure */
#endif

#ifdef MSG_TARGET_LIMIT
     struct {
        struct Client *cli;
        time_t sent;
     } targets[MSG_TARGET_MAX];              /* structure for target rate limiting */
     time_t last_target_complain;
     unsigned int num_target_errors;
#endif

};


#define	CLIENT_LOCAL_SIZE sizeof(aClient)
#define	CLIENT_REMOTE_SIZE offsetof(aClient,count)
/* statistics structures */
struct stats
{
  unsigned int is_cl;		/* number of client connections */
  unsigned int is_sv;		/* number of server connections */
  unsigned int is_ni;		/* connection but no idea who it was */
  unsigned short is_cbs;	/* bytes sent to clients */
  unsigned short is_cbr;	/* bytes received to clients */
  unsigned short is_sbs;	/* bytes sent to servers */
  unsigned short is_sbr;	/* bytes received to servers */
  unsigned long is_cks;		/* k-bytes sent to clients */
  unsigned long is_ckr;		/* k-bytes received to clients */
  unsigned long is_sks;		/* k-bytes sent to servers */
  unsigned long is_skr;		/* k-bytes received to servers */
  time_t is_cti;		/* time spent connected by clients */
  time_t is_sti;		/* time spent connected by servers */
  unsigned int is_ac;		/* connections accepted */
  unsigned int is_ref;		/* accepts refused */
  unsigned int is_throt;	/* accepts throttled */
  unsigned int is_drone;	/* refused drones */
  unsigned int is_unco;		/* unknown commands */
  unsigned int is_wrdi;		/* command going in wrong direction */
  unsigned int is_unpf;		/* unknown prefix */
  unsigned int is_empt;		/* empty message */
  unsigned int is_num;		/* numeric message */
  unsigned int is_kill;		/* number of kills generated on collisions */
  unsigned int is_fake;		/* MODE 'fakes' */
  unsigned int is_asuc;		/* successful auth requests */
  unsigned int is_abad;		/* bad auth requests */
  unsigned int is_psuc;		/* successful proxy requests */
  unsigned int is_pbad;		/* bad proxy requests */
  unsigned int is_udp;		/* packets recv'd on udp port */
  unsigned int is_loc;		/* local connections made */
  unsigned int is_ref_1;	/* refused at kline stage 1 */
  unsigned int is_ref_2;	/* refused at kline stage 2 */
#ifdef FLUD
  unsigned int is_flud;		/* users/channels flood protected */
#endif				/* FLUD */
  char pad[16];			/* Make this an even 1024 bytes */
};

/* mode structure for channels */

struct SMode
{
  unsigned int mode;
  int limit;
  char key[KEYLEN + 1];
};

/* Message table structure */

struct Message
{
  char *cmd;
  int (*func) ();
  unsigned int count;		/* number of times command used */
  int parameters;
  char flags;

  /* bit 0 set means that this command is allowed to be used only on
   * the average of once per 2 seconds -SRB */

  /* I could have defined other bit maps to above instead of the next
     * two flags that I added. so sue me. -Dianora */

  char allow_unregistered_use;	/* flag if this command can be used 
				   * if unregistered */

  char reset_idle;		/* flag if this command causes idle time to be 
				   * reset */
  unsigned long bytes;
};

typedef struct msg_tree
{
  char *final;
  struct Message *msg;
  struct msg_tree *pointers[26];
}
MESSAGE_TREE;

/*
 * Move BAN_INFO information out of the SLink struct its _only_ used
 * for bans, no use wasting the memory for it in any other type of
 * link. Keep in mind, doing this that it makes it slower as more
 * Malloc's/Free's have to be done, on the plus side bans are a smaller
 * percentage of SLink usage. Over all, the th+hybrid coding team came
 * to the conclusion it was worth the effort.
 * 
 * - Dianora
 */

struct Ban
{
  char *banstr;
  char *who;
  time_t when;
  u_char type;
  aBan *next;
};

struct Exempt
{
  char *exemptstr;
  char *who;
  time_t when;
  u_char type;
  anExempt *next;
};

/* channel member link structure, used for chanmember chains */
struct ChanLink
{
  struct ChanLink *next;
  aClient *client_p;
  int flags;
  int bans;			/* for bquiet: number of bans against this user */
};

/* general link structure used for chains */

struct SLink
{
  struct SLink *next;
  union
  {
    aClient *client_p;
    aChannel *channel_p;
    aConfItem *aconf;
    aBan *banptr;
    anExempt *exemptptr;
    aWatch *wptr;
    char *cp;
  }
  value;
  int flags;
};

struct SLinkD
{
   struct SLinkD *next;
   struct SLinkD *prev;
   union
   {
     aClient   *client_p;
     aChannel  *channel_p;
     aConfItem  *aconf;
     aBan    *banptr;
     aWatch *wptr;
     char    *cp;
   }
   value;
   int     flags;
};

/* channel structure */

struct Channel
{
  struct Channel *nextch, *prevch, *hnextch;
  int hashv;			/* raw hash value */
  Mode mode;
  char topic[TOPICLEN + 1];
  char topic_info[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
  time_t topic_time;
  int users;
  chanMember *members;
  Link *invites;
  aBan *banlist;
  anExempt *exemptlist;
  ts_val channelts;
#ifdef FLUD
  time_t fludblock;
  struct fludbot *fluders;
#endif
  char chname[CHANNELLEN + 1];
};
#define	TS_CURRENT	5	/* current TS protocol version */
#define	TS_MIN		3	/* minimum supported TS protocol version */
#define	TS_DOESTS	0x20000000
#define	DoesTS(x)	((x)->tsinfo == TS_DOESTS)
/* Channel Related macros follow */

/* Channel related flags */

#define	CHFL_CHANOP     	0x0001	/* Channel operator */
#define	CHFL_VOICE      	0x0002	/* the power to speak */
#define	CHFL_DEOPPED 		0x0004	/* deopped by us, modes need to be bounced */
#define CHFL_HALFOP		0x0008	/* Channel Half Operator */
#define CHFL_DEHALFOPPED	0x0010	/* De Halfopped by us, mode needs to be bounced */
#define CHFL_CHANADMIN		0x0020	/* Channel Administrator */
#define CHFL_DEADMINED		0x0040	/* De Admined by us, mode needs to be bounced */
#define	CHFL_BAN		0x0080	/* ban channel flag */
#define CHFL_EXEMPT		0x0100	/* exempt ban channel flag */
/* ban mask types */

#define MTYP_FULL      0x01	/* mask is nick!user@host */
#define MTYP_USERHOST  0x02	/* mask is user@host */
#define MTYP_HOST      0x04	/* mask is host only */

/* Channel Visibility macros */

#define	MODE_CHANOP	    	CHFL_CHANOP
#define	MODE_VOICE	    	CHFL_VOICE
#define	MODE_DEOPPED  		CHFL_DEOPPED
#define MODE_HALFOP		CHFL_HALFOP
#define MODE_DEHALFOPPED	CHFL_DEHALFOPPED
#define MODE_CHANADMIN		CHFL_CHANADMIN
#define MODE_DEADMINED		CHFL_DEADMINED
#define	MODE_PRIVATE  		0x00000080
#define	MODE_SECRET   		0x00000100
#define	MODE_MODERATED  	0x00000200
#define	MODE_TOPICLIMIT 	0x00000400
#define	MODE_INVITEONLY 	0x00000800
#define	MODE_NOPRIVMSGS 	0x00001000
#define	MODE_KEY	      	0x00002000
#define	MODE_BAN	      	0x00004000
#define	MODE_LIMIT	    	0x00008000
#define MODE_REGISTERED		0x00010000
#define MODE_REGONLY		0x00020000
#define MODE_NOCOLOR		0x00040000
#define MODE_OPERONLY   	0x00080000
#define MODE_EXEMPT		0x00100000
#define MODE_NOINVITE		0x00200000
#define MODE_NOKNOCK		0x00400000
#define MODE_SECURE		0x00800000
#define MODE_ADMINONLY	0x01000000
#define MODE_MODREG		0x02000000
#define MODE_NOQUITREASON 0x04000000

/* the hell is this? - lucas */
/* #define	MODE_FLAGS	    0x3fff*/

/* mode flags which take another parameter (With PARAmeterS) */

#define	MODE_WPARAS	(MODE_CHANOP|MODE_VOICE|MODE_BAN|MODE_EXEMPT|MODE_KEY|MODE_LIMIT|MODE_HALFOP|MODE_CHANADMIN)

/*
 * Undefined here, these are used in conjunction with the above modes
 * in the source. #define       MODE_DEL       0x40000000 #define
 * MODE_ADD       0x80000000
 */

#define	HoldChannel(x)		(!(x))

/*name invisible */

#define	SecretChannel(x)	((x) && ((x)->mode.mode & MODE_SECRET))

/* channel not shown but names are */

#define	HiddenChannel(x)	((x) && ((x)->mode.mode & MODE_PRIVATE))

/* channel visible */

#define	ShowChannel(v,c)	(PubChannel(c) || IsMember((v),(c)))
#define	PubChannel(x)		((!x) || ((x)->mode.mode &(MODE_PRIVATE | MODE_SECRET)) == 0)

/* Channel Supressing Quit messages */
#define NoQuitReasonChan(x)	((x) && ((x)->mode.mode & MODE_NOQUITREASON))

/* #define IsMember(user,chan)
 * (find_user_link((chan)->members,user) ? 1 : 0) */

#define IsMember(blah,chan) ((blah && blah->user && find_channel_link((blah->user)->channel, chan)) ? 1 : 0)

#define	IsChannelName(name) ((name) && (*(name) == '#' || *(name) == '&'))

/* Misc macros */

#define	BadPtr(x) (!(x) || (*(x) == '\0'))

#define	isvalid(c) (((c) >= 'A' && (c) < '~') || MyIsDigit(c) || (c) == '-')

#define	MyConnect(x)			((x)->fd >= 0)
#define	MyClient(x)			(MyConnect(x) && IsClient(x))
#define	MyOper(x)			(MyConnect(x) && IsOper(x))

/* String manipulation macros */

/* strncopynt --> strncpyzt to avoid confusion, sematics changed N must
 * be now the number of bytes in the array --msa */

#define	strncpyzt(x, y, N) do{(void)strncpy(x,y,N);x[N-1]='\0';}while(0)
#define	StrEq(x,y)	(!strcmp((x),(y)))

/* used in SetMode() in channel.c and m_umode() in s_msg.c */

#define	MODE_NULL      0
#define	MODE_ADD       0x40000000
#define	MODE_DEL       0x20000000

/* return values for hunt_server() */

#define	HUNTED_NOSUCH	(-1)	/* if the hunted server is not found */
#define	HUNTED_ISME	0	/* if this server should execute the command */
#define	HUNTED_PASS	1	/* if message passed onwards successfully */

/* used when sending to #mask or $mask */

#define	MATCH_SERVER  1
#define	MATCH_HOST    2

/* used for async dns values */

#define	ASYNC_NONE	(-1)
#define	ASYNC_CLIENT	0
#define	ASYNC_CONNECT	1
#define	ASYNC_CONF	2
#define	ASYNC_SERVER	3

/* misc variable externs */

extern char version[128], protoctl[512], smodestring[128], umodestring[128], cmodestring[128],
  *creditstext[], *copyrighttext[];
extern char *generation, *creation;

/* misc defines */

#define ZIP_NEXT_BUFFER -4
#define RC4_NEXT_BUFFER -3
#define	FLUSH_BUFFER	-2
#define	UTMP		"/etc/utmp"
#define	COMMA		","

#ifdef ORATIMING
/*
 * Timing stuff (for performance measurements): compile with
 * -DORATIMING and put a TMRESET where you want the counter of time
 * spent set to 0, a TMPRINT where you want the accumulated results,
 * and TMYES/TMNO pairs around the parts you want timed -orabidoo
 */

extern struct timeval tsdnow, tsdthen;
extern unsigned long tsdms;

#define TMRESET tsdms=0;
#define TMYES gettimeofday(&tsdthen, NULL);
#define TMNO gettimeofday(&tsdnow, NULL); if (tsdnow.tv_sec!=tsdthen.tv_sec) tsdms+=1000000*(tsdnow.tv_sec-tsdthen.tv_sec); tsdms+=tsdnow.tv_usec; tsdms-=tsdthen.tv_usec;
#define TMPRINT sendto_ops("Time spent: %ld ms", tsdms);
#else
#define TMRESET
#define TMYES
#define TMNO
#define TMPRINT
#endif

/* allow 5 minutes after server rejoins the network before allowing
 * chanops new channel */

#ifdef NO_CHANOPS_WHEN_SPLIT
#define MAX_SERVER_SPLIT_RECOVERY_TIME 5
#endif

#ifdef FLUD
struct fludbot
{
  struct Client *fluder;
  int count;
  time_t first_msg, last_msg;
  struct fludbot *next;
};

#endif /* FLUD */

struct Watch
{
  aWatch *hnext;
  time_t lasttime;
  Link *watch;
  char nick[1];
};

struct ListOptions
{
  LOpts *next;
  Link *yeslist, *nolist;
  int starthash;
  short int showall;
  unsigned short usermin;
  int usermax;
  time_t currenttime;
  time_t chantimemin;
  time_t chantimemax;
  time_t topictimemin;
  time_t topictimemax;
};

typedef struct SearchOptions
{
  int umodes;
  char *nick;
  char *user;
  char *host;
  char *gcos;
  char *ip;
  char *vhost;
  int class;
  int class_value;
  unsigned int cidr4_ip;
  unsigned int cidr4_mask;
  int ts;
  int ts_value;
  aChannel *channel;
  aClient *server;
  unsigned int channelflags;
#if 0
/* compilers bitch about this, its not correct for ansi c */
  char umode_plus:1;
  char nick_plus:1;
  char user_plus:1;
  char host_plus:1;
  char gcos_plus:1;
  char ip_plus:1;
  char cidr4_plus:1;
  char vhost_plus:1;
  char chan_plus:1;
  char serv_plus:1;
  char away_plus:1;
  char check_away:1;
  char check_umode:1;
  char show_chan:1;
  char search_chan:1;
  char isoper:1;
  char spare:1; /* spare space for more stuff(?) */
#else 
  char umode_plus;
  char nick_plus;
  char user_plus;
  char host_plus;
  char gcos_plus;
  char ip_plus;
  char cidr4_plus;
  char vhost_plus;
  char chan_plus;
  char serv_plus;
  char away_plus;
  char check_away;
  char check_umode;
  char show_chan;
  char search_chan;
  char isoper;
  char spare; /* spare space for more stuff(?) */
#endif
}
SOpts;


struct t_crline
{
  char *channel;
  int type;
  aCRline *next, *prev;
};

struct t_logfile
{
  int logflag;
  char *logname;
  FILE *logfd;
};

/*
 * Send /LIST as long as their output isn't blocking
 * and we haven't used 2/3rds of their sendQ
 */
#define IsSendable(x)    (!((x)->flags & FLAGS_BLOCKED) && DBufLength(&(x)->sendQ) < (int) ((float) (x)->sendqlen / 1.5))
#define DoList(x)    (((x)->user) && ((x)->user->lopt))

/* internal defines for client_p->sockerr */
#define IRCERR_BUFALLOC		-11
#define IRCERR_ZIP		-12
#define IRCERR_SSL		-13

#endif /* __struct_include__ */
