/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/dynconf.h
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
 *  $Id: dynconf.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#ifndef	__dynconf_include__
#define __dynconf_include__ 1


#define DYNCONF_H

/* config level */
#define DYNCONF_CONF_VERSION "1.1.3"
#define DYNCONF_NETWORK_VERSION "2.1.6"

typedef struct zNetwork aNetwork;
struct zNetwork
{

  char *x_ircnetwork;
  char *x_ircnetwork_name;
  char *x_defserv;
  char *x_website;
  char *x_rules_page;
  char *x_network_kline_address;
  unsigned x_mode_x:1;
  unsigned x_mode_x_lock:1;
  unsigned x_clone_protection:1;
  long x_clone_limit;
  unsigned x_oper_clone_limit_bypass:1;
  unsigned x_hideulinedservs:1;
  unsigned x_global_connect_notices:1;
  unsigned x_oper_autoprotect:1;
  long x_oper_mode_override;
  unsigned x_oper_mode_override_notice:1;
  char *x_users_auto_join;
  char *x_opers_auto_join;
  char *x_services_server;
  char *x_statserv_server;
  char *x_hostserv_server;
  unsigned x_enable_nickserv_alias;
  char *x_nickserv_name;
  unsigned x_enable_chanserv_alias;
  char *x_chanserv_name;
  unsigned x_enable_memoserv_alias;
  char *x_memoserv_name;
  unsigned x_enable_botserv_alias;
  char *x_botserv_name;
  unsigned x_enable_rootserv_alias;
  char *x_rootserv_name;
  unsigned x_enable_operserv_alias;
  char *x_operserv_name;
  unsigned x_enable_statserv_alias;
  char *x_statserv_name;
  unsigned x_enable_hostserv_alias;
  char *x_hostserv_name;
  char *x_locop_host;
  char *x_oper_host;
  char *x_guestadmin_host;
  char *x_servcoadmin_host;
  char *x_servadmin_host;
  char *x_techcoadmin_host;
  char *x_techadmin_host;
  char *x_netcoadmin_host;
  char *x_netadmin_host;
  char *x_helpchan;


};

typedef struct zConfiguration aConfiguration;
struct zConfiguration
{
  unsigned hub:1;
  long client_flood;
  long max_channels_per_user;
  char *geo_location;
  char *server_kline_address;
  unsigned wingate_notice;
  char *monitor_host;
  char *monitor_url;
  unsigned def_mode_i:1;
  unsigned short_motd:1;
  char *include;
  unsigned log_cycle:1;
  unsigned log_multifiles:1;
  unsigned log_cycle_renameafter:1;
  int log_flags;
  aNetwork network;
};


#ifndef DYNCONF_C
extern aConfiguration iConf;
#endif

#define IRCNETWORK			iConf.network.x_ircnetwork
#define IRCNETWORK_NAME			iConf.network.x_ircnetwork_name
#define DEFSERV				iConf.network.x_defserv
#define WEBSITE				iConf.network.x_website
#define RULES_PAGE			iConf.network.x_rules_page
#define NETWORK_KLINE_ADDRESS		iConf.network.x_network_kline_address
#define MODE_x				iConf.network.x_mode_x
#define MODE_x_LOCK			iConf.network.x_mode_x_lock
#define CLONE_PROTECTION		iConf.network.x_clone_protection
#define CLONE_LIMIT			iConf.network.x_clone_limit
#define OPER_CLONE_LIMIT_BYPASS		iConf.network.x_oper_clone_limit_bypass
#define HIDEULINEDSERVS			iConf.network.x_hideulinedservs
#define GLOBAL_CONNECTS			iConf.network.x_global_connect_notices
#define OPER_AUTOPROTECT		iConf.network.x_oper_autoprotect
#define OPER_MODE_OVERRIDE		iConf.network.x_oper_mode_override
#define OPER_MODE_OVERRIDE_NOTICE	iConf.network.x_oper_mode_override_notice
#define USERS_AUTO_JOIN			iConf.network.x_users_auto_join
#define OPERS_AUTO_JOIN			iConf.network.x_opers_auto_join
#define SERVICES_SERVER			iConf.network.x_services_server
#define STATSERV_SERVER			iConf.network.x_statserv_server
#define HOSTSERV_SERVER			iConf.network.x_hostserv_server
#define ENABLE_NICKSERV_ALIAS		iConf.network.x_enable_nickserv_alias
#define NICKSERV_NAME			iConf.network.x_nickserv_name
#define ENABLE_CHANSERV_ALIAS		iConf.network.x_enable_chanserv_alias
#define CHANSERV_NAME			iConf.network.x_chanserv_name
#define ENABLE_MEMOSERV_ALIAS		iConf.network.x_enable_memoserv_alias
#define MEMOSERV_NAME			iConf.network.x_memoserv_name
#define ENABLE_BOTSERV_ALIAS		iConf.network.x_enable_botserv_alias
#define BOTSERV_NAME			iConf.network.x_botserv_name
#define ENABLE_ROOTSERV_ALIAS		iConf.network.x_enable_rootserv_alias
#define ROOTSERV_NAME			iConf.network.x_rootserv_name
#define ENABLE_OPERSERV_ALIAS		iConf.network.x_enable_operserv_alias
#define OPERSERV_NAME			iConf.network.x_operserv_name
#define ENABLE_STATSERV_ALIAS		iConf.network.x_enable_statserv_alias
#define STATSERV_NAME			iConf.network.x_statserv_name
#define ENABLE_HOSTSERV_ALIAS		iConf.network.x_enable_hostserv_alias
#define HOSTSERV_NAME			iConf.network.x_hostserv_name
#define LOCOP_HOST			iConf.network.x_locop_host
#define OPER_HOST			iConf.network.x_oper_host
#define GUESTADMIN_HOST			iConf.network.x_guestadmin_host
#define SERVCOADMIN_HOST		iConf.network.x_servcoadmin_host
#define SERVADMIN_HOST			iConf.network.x_servadmin_host
#define TECHCOADMIN_HOST		iConf.network.x_techcoadmin_host
#define TECHADMIN_HOST			iConf.network.x_techadmin_host
#define NETCOADMIN_HOST			iConf.network.x_netcoadmin_host
#define NETADMIN_HOST			iConf.network.x_netadmin_host
#define HELPCHAN			iConf.network.x_helpchan



#define HUB				iConf.hub
#define CLIENT_FLOOD			iConf.client_flood
#define MAXCHANNELSPERUSER		iConf.max_channels_per_user
#define GEO_LOCATION			iConf.geo_location
#define SERVER_KLINE_ADDRESS		iConf.server_kline_address
#define WINGATE_CHECK
#define WINGATE_REASON			"Insecure Proxy/Socks connections are not permitted on this server"
#define WINGATE_NOTICE			iConf.wingate_notice
#define MONITOR_HOST			iConf.monitor_host
#define MONITOR_URL			iConf.monitor_url
#define DEF_MODE_i			iConf.def_mode_i
#define SHORT_MOTD			iConf.short_motd
#define INCLUDE				iConf.include
#define LOG_CYCLE     iConf.log_cycle
#define LOG_FLAGS     iConf.log_flags
#define LOG_MULTIFILES iConf.log_multifiles
#define LOG_CYCLE_RENAMEAFTER iConf.log_cycle_renameafter


#define network_proxyscan 1



#endif /* __dynconf_include__ */
