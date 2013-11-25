#/*
# *  UltimateIRCd - an Internet Relay Chat Daemon, tools/Makefile.in
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
CCDV	= @/root/irc/Surreal0.0.1/ccdv 
CFLAGS		= -O2 -g  -DIRCD_PREFIX=\"/root/SurrealIRCD\"
LDFLAGS		= 
MKDEP		= ${CC} -MM
MV		= /bin/mv
RM		= /bin/rm
CP		= /bin/cp

prefix		= /root/SurrealIRCD
exec_prefix	= ${prefix}
exec_suffix	= @exec_suffix@
bindir		= ${exec_prefix}/bin
libexecdir	= ${bindir}
sysconfdir	= ${prefix}/etc
localstatedir	= ${prefix}

SCRIPTS		= ircd rehash kill
PROGS		= mkpasswd

SSL_LIBS	= 
SSL_INCLUDES	= 

IRCDLIBS	= -lcrypt -lresolv -lnsl  $(SSL_LIBS)

INCLUDES	= -I../include $(SSL_INCLUDES)
CPPFLAGS	= ${INCLUDES} 

mkpasswd_SOURCES=mkpasswd.c
mkpasswd_OBJECTS=mkpasswd.o


all_OBJECTS=$(mkpasswd_OBJECTS)

all: mkpasswd

build: all


mkpasswd: $(mkpasswd_OBJECTS)
	$(CCDV)$(CC) ${CFLAGS} ${LDFLAGS} -o mkpasswd $(mkpasswd_OBJECTS) $(IRCDLIBS)


clean:
	$(RM) -f $(all_OBJECTS) mkpasswd *~ core

distclean: clean
	$(RM) -f Makefile

.c.o:
	$(CCDV)${CC} ${CPPFLAGS} ${CFLAGS} -c $<

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
	@for i in $(SCRIPTS); do \
		if test -f $(prefix)/$$i; then \
		        echo "$(prefix)/$$i exsists... skipping"; \
		else \
			echo $(INSTALL_BIN) $$i $(prefix); \
			$(INSTALL_BIN) $$i $(prefix); \
		fi; \
	done
	
		@for i in $(PROGS); do \
		if test -f $(bindir)/$$i; then \
			echo $(MV) $(bindir)/$$i $(bindir)/$$i.old; \
			$(MV) $(bindir)/$$i $(bindir)/$$i.old; \
		fi; \
		echo $(INSTALL_BIN) $$i $(bindir); \
		$(INSTALL_BIN) $$i $(bindir); \
	done


