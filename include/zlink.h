/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/zlink.h
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
 *  $Id: zlink.h 985 2007-02-04 20:51:36Z shadowmaster $
 *
 */

extern void *zip_create_input_session ();
extern void *zip_create_output_session ();
extern char *zip_input (void *session, char *buffer, int *len, int *err,
			char **nbuf, int *nlen);
/* largedata is err return */
extern char *zip_output (void *session, char *buffer, int *len,
			 int forceflush, int *largedata);
extern int zip_is_data_out (void *session);
extern void zip_out_get_stats (void *session, unsigned long *insiz,
			       unsigned long *outsiz, double *ratio);
extern void zip_in_get_stats(void *session, unsigned long *insiz,
			       unsigned long *outsiz, double *ratio);
extern void zip_destroy_input_session (void *session);
extern void zip_destroy_output_session (void *session);
