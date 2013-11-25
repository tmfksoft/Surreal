#/*
# *  UltimateIRCd - an Internet Relay Chat Daemon, src/Makefile.in
# *
# *  Copyright (C) 1990-2007 by the past and present ircd coders, and others.
# *  Refer to the documentation within doc/authors/ for full credits and copyrights.
# *
# *  This program is free software; you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 2 of the License, or
# *  (at your option) any later version.
# *
# *  This program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with this program; if not, write to the Free Software
# *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
# *  USA
# *
# *  $Id: Makefile.in 985 2007-02-04 20:51:36Z shadowmaster $
# *
# */

CC		= gcc
INSTALL		= /usr/bin/install -c
INSTALL_BIN	= ${INSTALL}
INSTALL_DATA	= ${INSTALL} -m 644
INSTALL_SUID	= ${INSTALL} -o root -m 4755
RM		= /bin/rm
LEX		= @LEX@
LEXLIB		= @LEXLIB@
CCDV	= @/home/dev/Surreal-0.0.1/ccdv 
CFLAGS		= -O2 -g  -DIRCD_PREFIX=\"/home/dev/SurrealIRCD\"
LDFLAGS   	= 
MKDEP		= gcc -MM -DIRCD_PREFIX=\"/home/dev/SurrealIRCD\"
MV		= /bin/mv
RM		= /bin/rm
prefix		= /home/dev/SurrealIRCD
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
libexecdir	= ${bindir}
sysconfdir	= ${prefix}/etc
localstatedir	= ${prefix}

DOLLAR = $$

IRCD_EXE	= ircd

PROGS		= $(IRCD_EXE)

SSL_LIBS	= 
SSL_INCLUDES	= 

IRCDLIBS	= ../zlib/libz.a -lcrypt -lresolv -lnsl  $(SSL_LIBS)

INCLUDES	= -I../include -I../zlib $(SSL_INCLUDES)
CPPFLAGS	= ${INCLUDES} 

default:	all

RES_SRC         =
#This is only for very old systems that NEED this
#RES_SRC        = res_mkquery.c res_init.c res_comp.c

BASE_SRCS =			\
blalloc.c			\
bsd.c				\
channel.c			\
class.c				\
clientlist.c			\
dbuf.c				\
dich_conf.c			\
dynconf.c			\
drone.c				\
fdlist.c			\
fds.c				\
hash.c				\
inet_addr.c			\
inet_ntop.c			\
inet_pton.c			\
ircd.c				\
ircsprintf.c			\
list.c				\
match.c				\
m_nick.c			\
m_who.c				\
packet.c			\
parse.c				\
res.c				\
s_auth.c			\
s_bsd.c				\
s_conf.c			\
s_debug.c			\
s_err.c				\
s_help.c			\
s_hidehost.c			\
s_misc.c			\
s_numeric.c			\
s_services.c			\
s_serv.c			\
s_ultimate.c			\
s_user.c			\
send.c				\
socketengine_devpoll.c		\
socketengine_epoll.c		\
socketengine_kqueue.c		\
socketengine_poll.c		\
socketengine_select.c		\
support.c			\
scache.c			\
ssl.c				\
throttle.c			\
userban.c			\
whowas.c			\
zlink.c				\
$(ENGINE_S)			\



SRCS = ${BASE_SRCS} ${RES_SRC}
OBJS = ${SRCS:.c=.o} version.o

all: ircd

build: all

ircd: $(OBJS)
	${CCDV}${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${IRCDLIBS}
	mv version.c version.c.last
	
install-mkdirs:
	-@if test ! -d $(prefix); then \
		echo "mkdir $(prefix)"; \
		mkdir $(prefix); \
	fi
	-@if test ! -d $(exec_prefix); then \
		echo "mkdir $(exec_prefix)"; \
		mkdir $(exec_prefix); \
	fi
	-@if test ! -d $(bindir); then \
		echo "mkdir $(bindir)"; \
		mkdir $(bindir); \
	fi
	-@if test ! -d $(sysconfdir); then \
		echo "mkdir $(sysconfdir)"; \
		mkdir $(sysconfdir); \
	fi
	-@if test ! -d $(localstatedir)/logs; then \
		echo "mkdir $(localstatedir)/logs"; \
		mkdir $(localstatedir)/logs; \
	fi

install: install-mkdirs build
	@for i in $(PROGS); do \
		if test -f $(bindir)/$$i; then \
			echo $(MV) $(bindir)/$$i $(bindir)/$$i.old; \
			$(MV) $(bindir)/$$i $(bindir)/$$i.old; \
		fi; \
		echo $(INSTALL_BIN) $$i $(bindir); \
		$(INSTALL_BIN) $$i $(bindir); \
	done
	
version.c: version.c.SH
	@/bin/sh ./version.c.SH

# this is really the default rule for c files
.c.o:
	${CCDV}${CC} ${CPPFLAGS} ${CFLAGS} -c $<
	
depend:
	${MKDEP} ${CPPFLAGS} ${BASE_SRCS} ${EXTRA_SRCS} > .depend

clean:
	${RM} -f *.o ircd.core core ircd
	
distclean: clean
	${RM} -f Makefile version.c version.c.last


include .depend
