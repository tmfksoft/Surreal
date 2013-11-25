/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/fds.h
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
 *  $Id: fds.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


#undef FDSDEBUG
#ifdef FDSDEBUG
# ifdef _WIN32
#  define fdfprintf fprintf
# else
#  define fdfprintf(x, y, ...) if(isatty(2)) fprintf(x, y);
# endif
#else
# ifdef _WIN32
#  define fdfprintf(x, y, z, a, b, c, d, e, f)
# else
#  define fdfprintf(x, y, ...)
# endif
#endif


#define FDT_NONE      0
#define FDT_AUTH      1
#define FDT_RESOLVER  2
#define FDT_CLIENT    3
#define FDT_LISTENER  4
#define FDT_PROXY     5

#define FDF_WANTREAD  0x01
#define FDF_WANTWRITE 0x02

void init_fds();

void add_fd(int fd, int type, void *value);
void del_fd(int fd);

void get_fd_info(int fd, int *type, unsigned int *flags, void **value);
void set_fd_flags(int fd, unsigned int flags);
void unset_fd_flags(int fd, unsigned int flags);

void set_fd_internal(int fd, void *ptr);
void *get_fd_internal(int fd);

void check_client_fd(aClient *client_p);

void report_fds(aClient *client_p);
