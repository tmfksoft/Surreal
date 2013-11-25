#/*
# *  UltimateIRCd - an Internet Relay Chat Daemon, Makefile.in
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

RM=/bin/rm

SHELL=/bin/sh
SUBDIRS=zlib src tools data 
SUBDIRS2=data/networks tools/ircdcron tools/ssl-cert
CLEANDIRS = ${SUBDIRS}

MAKE = make ${MFLAGS} 

all:	build

autoconf: autoconf/configure.in
	autoconf autoconf/configure.in >configure
	autoheader autoconf/configure.in > include/setup.h.in
	${RM} -f config.cache

build:
	@for i in $(SUBDIRS); do \
		echo "Building $$i";\
		cd $$i;\
		${MAKE} build; cd ..;\
	done
	@echo ' '
	@echo '================================================================='
	@echo 'Please remember to read the doc/README.FIRST and doc/README.'
	@echo ' '
	@echo 'SurrealIRCd FAQ and support forums at'
	@echo 'http://wiki.shadow-realm.org/index.php/UltimateIRCd/FAQ'
	@echo 'http://www.shadow-realm.org/forum/'
	@echo ' '
	@echo 'Read the README.FIRST and FAQ before asking for support'
	@echo 'or you will make getting support difficult and confusing for yourself.'
	@echo '================================================================='
	@echo ' '

profile:
	@for i in $(SUBDIRS); do \
		echo "Building $$i [profile]";\
		cd $$i;\
		${MAKEPROFILE} build; cd ..;\
	done

clean:
	${RM} -f *~ core
	@for i in $(SUBDIRS); do \
		echo "Cleaning $$i";\
		cd $$i;\
		${MAKE} clean; cd ..;\
	done
	-@if [ -f include/setup.h ] ; then \
	echo "To really restart installation, make distclean" ; \
	fi

distclean:
	${RM} -f Makefile Makefile.tmp *~ *.rej *.orig core ircd.core *.tmp ccdv
	${RM} -f config.status config.cache config.log
	cd include; ${RM} -f setup.h gmp_irc.h *~ *.rej *.orig options.h; cd ..
	@for i in $(SUBDIRS); do \
		echo "Cleaning $$i";\
		cd $$i;\
		${MAKE} distclean; cd ..;\
	done

depend:
	@for i in $(SUBDIRS); do \
		echo "depend ==> $$i";\
		cd $$i;\
		${MAKE} depend; cd ..;\
	done

install: all
	@for i in $(SUBDIRS); do \
		echo "install ==> $$i";\
		cd $$i;\
		${MAKE} install; \
		cd ..; \
	done

	@for i in $(SUBDIRS2); do \
		echo "install ==> $$i";\
		cd $$i;\
		${MAKE} install; \
		cd ../../; \
	done


