/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/ircsprintf.h
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
 *  $Id: ircsprintf.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


#ifndef IRCSPRINTF_H
#define IRCSPRINTF_H
#include <stdarg.h>
#include <stdio.h>
#include "config.h"

#ifdef INET6_DEV
# include "va_copy.h"
#endif

#ifdef _WIN32
#define vsnprintf _vsnprintf
#endif

/* define this if you intend to use ircsnprintf or ircvsnprintf */
/* It's not used, and sNprintf functions are not in all libraries */
#define WANT_SNPRINTF

int ircsprintf (char *str, const char *format, ...);
int ircvsprintf (char *str, const char *format, va_list ap);
#ifdef WANT_SNPRINTF
int ircvsnprintf (char *str, size_t size, const char *format, va_list ap);
int ircsnprintf (char *str, size_t size, const char *format, ...);
#endif
#endif
