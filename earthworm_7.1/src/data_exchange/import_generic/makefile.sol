
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.5 2005/04/25 22:26:07 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.5  2005/04/25 22:26:07  dietz
#     Added new module import_ack, partner module to export*ack.
#     Import_ack sends an acknowledgment packet for every packet received.
#     The socket writing thread has been merged with the socket reading
#     thread. Most useful for low frequency, high importance packets.
#
#     Revision 1.4  2001/02/01 01:36:15  dietz
#     *** empty log message ***
#
#     Revision 1.3  2000/08/08 17:38:18  lucky
#     Added lint directive
#
#     Revision 1.2  2000/02/14 21:31:13  lucky
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 18:42:44  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


EWBIN = $L/kom.o $L/getutil.o $L/logit_mt.o $L/sema_ew.o \
        $L/transport.o $L/sleep_ew.o $L/socket_ew.o $L/time_ew.o \
        $L/threads_ew.o $L/socket_ew_common.o
          
all:
	make -f makefile.sol import_generic
	make -f makefile.sol import_ack

import_generic: import_generic.o $(EWBIN)
	cc -o $B/import_generic import_generic.o $(EWBIN) -lnsl -lsocket -mt -lposix4 -lthread

import_ack: import_ack.o $(EWBIN)
	cc -o $B/import_ack import_ack.o $(EWBIN) -lnsl -lsocket -mt -lposix4 -lthread


lint:
	lint import_generic.c import_ack.c  $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/import_generic* $B/import_ack*
