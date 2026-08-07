#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2000/03/13 23:45:14 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2000/03/13 23:45:14  lombard
#     Initial revision
#
#
#

DCC_INCLUDE = ../include
DCC_LIB = ../lib

RANLIB = ranlib

INCS = -I$(DCC_INCLUDE)

CFLAGS += $(INCS) $(PROF)

# DCC library definitions

DCC_MISC = $(DCC_LIB)/libdcc_misc.a

OBJS = 	ctx_file.o	\
	getmyname.o	\
	envfile.o	\
	fixopen.o	\
	strfuns.o	\
	dataswaps.o	\
	wildmatch.o	\
	calctime.o	\
	log.o		\
	dcc_env.o	\
	chansubs.o	\
	readline.o	\
	bombopen.o	\
	bombout.o	\
	safemem.o	\
	itemlist.o	\
	watchdog.o	\
	compat_strerror.o 

all:	$(DCC_MISC)


$(DCC_MISC): $(OBJS)
	$(AR) rv $@ $(OBJS)
	${RANLIB} $(DCC_MISC)

clean:
	rm -f *.o *.a core *~ \#*\#
