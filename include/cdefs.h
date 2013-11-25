/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/cdefs.h
 *
 * Copyright (C) 1991, 1993 The Regents of the University of California
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
 *  $Id: cdefs.h 985 2007-02-04 20:51:36Z shadowmaster $
 */

/*
 * ++Copyright++ 1991, 1993 - Copyright (c) 1991, 1993 The Regents of
 * the University of California.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met: 1. Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the following
 * disclaimer. 2. Redistributions in binary form must reproduce the
 * above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with
 * the distribution. 3. All advertising materials mentioning features
 * or use of this software must display the following acknowledgement:
 * This product includes software developed by the University of
 * California, Berkeley and its contributors. 4. Neither the name of
 * the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. - Portions Copyright (c) 1993 by Digital Equipment
 * Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the
 * document or software without specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL
 * DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. -
 * --Copyright--
 */



#ifndef	_CDEFS_H_
# define	_CDEFS_H_

# if defined(__cplusplus)
#	define	__BEGIN_DECLS	extern "C" {
#	define	__END_DECLS	};
# else
#	define	__BEGIN_DECLS
#	define	__END_DECLS
# endif
/*
 * The __CONCAT macro is used to concatenate parts of symbol names,
 * e.g. with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces
 * oldfoo. The __CONCAT macro is a bit tricky -- make sure you don't
 * put spaces in between its arguments.  __CONCAT can also concatenate
 * double-quoted strings produced by the __STRING macro, but this only
 * works with ANSI C.
 */
# if defined(__STDC__) || defined(__cplusplus)
#	define	__P(protos)	protos	/* full-blown ANSI C */
#	define	__CONCAT(x,y)	x ## y
#	define	__STRING(x)	#x
# else /* !(__STDC__ || __cplusplus) */
#	define	__P(protos)	()	/* traditional C preprocessor */
#	define	__CONCAT(x,y)	x/**/y
#	define	__STRING(x)	"x"
#	ifdef __GNUC__
#	 define	const		__const	/* GCC: ANSI C with -traditional */
#	 define	inline		__inline
#	 define	signed		__signed
#	 define	volatile	__volatile
#	else /* !__GNUC__ */
#	 define	const		/* delete ANSI C keywords */
#	 define	inline
#	 define	signed
#	 define	volatile
#	endif /* !__GNUC__ */
# endif	/* !(__STDC__ || __cplusplus) */
/*
 * GCC has extensions for declaring functions as `pure' (always returns
 * the same value given the same inputs, i.e., has no external state
 * and no side effects) and `dead' (nonreturning).  These mainly affect
 * optimization and warnings.  Unfortunately, GCC complains if these
 * are used under strict ANSI mode (`gcc -ansi -pedantic'), hence we
 * need to define them only if compiling without this.
 */
# if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#	define __dead __volatile
#	define __pure __const
# else
#	define __dead
#	define __pure
# endif
#endif /* !_CDEFS_H_ */
