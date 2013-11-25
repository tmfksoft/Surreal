/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/dh.h
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
 *  $Id: dh.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */


extern int dh_init ();
extern void dh_end_session (void *);
extern void *dh_start_session ();
extern char *dh_get_s_public (char *, int, void *);
extern int dh_get_s_shared(char *, int *, void *);
extern int dh_generate_shared (void *, char *);

extern int dh_hexstr_to_raw (char *string, unsigned char *hexout,
			     int *hexlen);

extern void rc4_process_stream_to_buf (void *rc4_context,
				       const unsigned char *istring,
				       unsigned char *ostring,
				       unsigned int stringlen);
extern void rc4_process_stream (void *rc4_context, unsigned char *istring,
				unsigned int stringlen);
extern void *rc4_initstate (unsigned char *key, int keylen);
extern void rc4_destroystate (void *a);
