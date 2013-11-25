/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/va_copy.h
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
 *  $Id: va_copy.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

#include "config.h"

/* Most systems will have va_copy */
#if defined (HAVE_VA_COPY)
# define VA_COPY va_copy
/* Older systems will have __va_copy due to the double underline being part of
 * the original spec
*/
#elif defined(HAVE___VA_COPY)
# define VA_COPY __va_copy
/* And there are always some (quite a few) systems that wont have either */
#else
# define VA_COPY(x, y) x = y
#endif
