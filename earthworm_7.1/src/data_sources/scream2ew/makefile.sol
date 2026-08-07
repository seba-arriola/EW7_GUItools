#
# makefile.sol:
#
# Copyright (c) 2003 Guralp Systems Limited
# Author James McKenzie, contact <software@guralp.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id: makefile.sol,v 1.3 2006/05/16 17:14:35 paulf Exp $
#
# $Log: makefile.sol,v $
# Revision 1.3  2006/05/16 17:14:35  paulf
# removed all Windows issues from the .c and .h and updated the makefile.sol
#
# Revision 1.2  2003/03/27 18:36:54  lucky
# Fixed install line
#
# Revision 1.1  2003/03/27 18:07:18  alex
# Initial revision
#
# Revision 1.5  2003/03/03 12:25:04  root
# #
#
# Revision 1.4  2003/02/28 17:05:37  root
# #
#
# Revision 1.3  2003/02/19 16:00:18  root
# #
#
#
#
PROG=scream2ew

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

BINDIR = ${EW_HOME}/${EW_VERSION}/bin
LIBDIR = ${EW_HOME}/${EW_VERSION}/lib

HSRCS=config.h dispatch.h ewc.h gcf.h gputil.h mainloop.h \
	map.h oscompat.h project.h scream.h util.h

SRCS=config.c dispatch.c ewc.c gcf.c gputil.c \
	main.c mainloop.c map.c scream.c util.c

OBJS=config.o dispatch.o ewc.o gcf.o gputil.o \
	main.o mainloop.o map.o scream.o util.o

LIBS=-lnsl -lsocket -lposix4 -lm

LOBJS=${LIBDIR}/kom.o ${LIBDIR}/getutil.o ${LIBDIR}/socket_ew.o  \
	${LIBDIR}/time_ew.o ${LIBDIR}/chron3.o ${LIBDIR}/logit.o \
	${LIBDIR}/transport.o ${LIBDIR}/sleep_ew.o ${LIBDIR}/swap.o 

default: install

install: ${BINDIR}/${PROG}

${BINDIR}/${PROG}:${PROG}
#	./install-sh -m 755 -c ${PROG} ${BINDIR}
	cp ${PROG} ${BINDIR}

${PROG} : ${OBJS} ${LOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${LOBJS} ${LIBS} 

lint:
	lint ${SRCS} ${GLOBALFLAGS}

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~ ${PROG}  *.BAK core.*

clean_bin:
	rm -f ${BINDIR}/${PROG}

checkin:
	ci -l -m# ${SRCS} ${HSRCS} makefile.sol makefile.nt

tidy: checkin
	indent -i2 -ts0 ${CSRCS} ${HSRCS}

# dependancies

config.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
dispatch.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
ewc.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
gcf.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
gputil.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
main.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
mainloop.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
map.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
scream.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h
util.o: project.h config.h dispatch.h ewc.h gcf.h gputil.h \
	mainloop.h map.h oscompat.h scream.h util.h

