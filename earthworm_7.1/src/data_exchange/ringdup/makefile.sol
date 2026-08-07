
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.5 2006/06/06 19:19:33 paulf Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.5  2006/06/06 19:19:33  paulf
#     upgraded for Hydra Block_scnl command
#
#     Revision 1.4  2005/07/27 20:47:27  friberg
#     fixed ringdup_scn target to have proper libs when using scnfilter
#
#     Revision 1.3  2004/05/27 16:42:09  dietz
#     Modified to link with genericfilter and scnfilter objects from the
#     ../export source directory to avoid massive code duplication. This change
#     instantly gave SCNL support to ringdup* executable.
#
#     Revision 1.2  2000/08/08 17:38:18  lucky
#     Added lint directive
#
#     Revision 1.1  2000/05/24 17:53:07  lucky
#     Initial revision
#
#     Revision 1.1  2000/03/29 16:16:00  whitmore
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
E = ../export

BINARIES = ringdup.o $L/mem_circ_queue.o $L/kom.o \
	$L/getsysname_ew.o $L/getutil.o $L/logit_mt.o $L/transport.o \
	$L/sleep_ew.o $L/time_ew.o $L/threads_ew.o $L/sema_ew.o $L/swap.o         

all:
	make -f makefile.sol ringdup_generic
	make -f makefile.sol ringdup_scn

ringdup_generic: $(BINARIES) $E/genericfilter.o
	cc -o $B/ringdup_generic $(BINARIES) $E/genericfilter.o -lnsl -mt -lposix4 -lthread -lc

ringdup_scn: $(BINARIES) ./scnfilter_exclude.o
	cc -o $B/ringdup_scn $(BINARIES) ./scnfilter_exclude.o $L/rdpickcoda.o $L/chron3.o -lnsl -mt -lposix4 -lthread -lc


lint:
	lint ringdup.c genericfilter.c scnfilter.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ringdup_generic* $B/ringdup_scn*
