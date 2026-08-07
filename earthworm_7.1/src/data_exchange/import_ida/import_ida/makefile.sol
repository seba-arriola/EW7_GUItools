#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2007/01/22 19:29:34 paulf Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2007/01/22 19:29:34  paulf
#     makefile.sol had comment in the wrong place
#
#     Revision 1.2  2005/04/20 19:52:22  davidk
#     Removed pinno object per Alex Bittenbinder.
#
#     Revision 1.1  2004/03/17 21:19:21  lombard
#     Initial revision
#
#     Revision 1.2  2002/11/03 18:34:51  lombard
#     Added GLOBALFLAGS to CFLAGS definition.
#
#
#

IDATAP = ../idatap
CFLAGS = -I$(IDATAP)/include -mt -g ${GLOBALFLAGS}

B =  $(EW_HOME)/$(EW_VERSION)/bin
L =  $(EW_HOME)/$(EW_VERSION)/lib

O = main.o client.o forward.o hbeat.o init.o params.o \
    reformat.o signals.o \
    $L/transport.o $L/getutil.o $L/kom.o $L/sleep_ew.o $L/logit_mt.o \
    $L/time_ew.o $L/sema_ew.o $L/threads_ew.o

#   pinno.o \      Pin # functionality removed 2005/04/20

import_ida: $O
	cc -o $B/import_ida $O -lm -lposix4 -L$(IDATAP) -lidatap -lsocket -lnsl -mt
