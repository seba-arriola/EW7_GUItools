
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2005/01/28 20:48:33 luetgert Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2005/01/28 20:48:33  luetgert
#     Now location code compatible.
#     Remote-copy eliminated.
#     Uses binary wsclient routines.
#     .
#
#     Revision 1.3  2001/04/12 17:14:41  davidk
#     replaced reference to queue_max_size.o with mem_circ_queue.o
#
#     Revision 1.2  2000/08/08 17:56:20  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:17:10  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


BINARIES = sgram.o  \
		$L/swap.o $L/logit.o $L/getutil.o  $L/transport.o $L/kom.o \
		$L/sleep_ew.o $L/time_ew.o $L/threads_ew.o $L/sema_ew.o\
		$L/mem_circ_queue.o  \
		$L/getavail.o $L/getsysname_ew.o $L/chron3.o\
		$L/parse_trig.o $L/pipe.o \
		$L/socket_ew.o $L/socket_ew_common.o $L/ws_clientII.o\
		$L/gd.o $L/gdfontt.o $L/gdfonts.o $L/gdfontmb.o \
		$L/gdfontl.o $L/gdfontg.o

sgram: $(BINARIES)
	cc -o $(B)/sgram $(BINARIES) -lsocket -lnsl -lm -mt -lposix4 -lthread -lc

.c.o:
	$(CC) $(CFLAGS) -g $(CPPFLAGS) -c  $(OUTPUT_OPTION) $<

lint:
	lint sgram.c $(GLOBALFLAGS)
# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/sgram*

