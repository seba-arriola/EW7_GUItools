
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.10 2005/04/22 17:15:23 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.10  2005/04/22 17:15:23  dietz
#     added msglen arg to exportfilter_logmsg() to correct logging of msgs.
#     modified to use rdpickcoda.c functions to interpret all picks/codas.
#
#     Revision 1.9  2004/04/20 22:52:52  dietz
#     moved export_actv.c and its sample configs from subdir export_actv into subdir export to eliminated duplication of export filter code
#
#     Revision 1.8  2002/07/19 22:59:22  dietz
#     references to scnprifilter.c changed to scnfilter.c (files merged)
#
#     Revision 1.7  2002/06/07 18:37:09  dietz
#     Removed built for export_scn_remap (merged with export_scn)
#
#     Revision 1.6  2002/03/21 17:39:26  dhanych
#     Added build for export_scn_pri
#
#     Revision 1.5  2001/01/19 01:18:33  dietz
#     *** empty log message ***
#
#     Revision 1.4  2001/01/19 01:13:38  dietz
#     added compilation of export_scn_remap
#
#     Revision 1.3  2001/01/02 16:56:01  lucky
#     Added swap.o
#
#     Revision 1.2  2000/08/08 17:38:18  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:23:11  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


BINARIES = export.o $L/mem_circ_queue.o $L/chron3.o $L/kom.o \
	$L/getsysname_ew.o $L/getutil.o $L/logit_mt.o $L/transport.o \
	$L/sleep_ew.o $L/socket_ew.o $L/socket_ew_common.o \
	$L/time_ew.o $L/threads_ew.o $L/sema_ew.o $L/swap.o

BINARIESACK = export_ack.o $L/mem_circ_queue.o $L/chron3.o $L/kom.o \
	$L/getsysname_ew.o $L/getutil.o $L/logit_mt.o $L/transport.o \
	$L/sleep_ew.o $L/socket_ew.o $L/socket_ew_common.o \
	$L/time_ew.o $L/threads_ew.o $L/sema_ew.o $L/swap.o

BINARIESPRI = export_scn_pri.o $L/priority_queue.o $L/chron3.o $L/kom.o \
	$L/getsysname_ew.o $L/getutil.o $L/logit_mt.o $L/transport.o \
	$L/sleep_ew.o $L/socket_ew.o $L/socket_ew_common.o \
	$L/time_ew.o $L/threads_ew.o $L/sema_ew.o $L/swap.o

BINARIESACTV = export_actv.o $L/mem_circ_queue.o $L/chron3.o $L/kom.o \
	$L/getsysname_ew.o $L/getutil.o $L/logit_mt.o $L/transport.o \
	$L/sleep_ew.o $L/socket_ew.o $L/socket_ew_common.o \
	$L/time_ew.o $L/threads_ew.o $L/sema_ew.o $L/swap.o


all:
	make -f makefile.sol export_generic
	make -f makefile.sol export_scnl
	make -f makefile.sol export_scnl_pri
	make -f makefile.sol export_gen_actv
	make -f makefile.sol export_scnl_actv
	make -f makefile.sol export_ack
	make -f makefile.sol export_scnl_ack


export_generic: $(BINARIES) genericfilter.o
	cc -o $B/export_generic $(BINARIES) genericfilter.o -lnsl -lsocket -mt -lposix4 -lthread -lc

export_scnl: $(BINARIES) scnfilter.o $L/rdpickcoda.o
	cc -o $B/export_scnl $(BINARIES) scnfilter.o $L/rdpickcoda.o -lnsl -lsocket -mt -lposix4 -lthread -lc

export_scnl_pri: $(BINARIESPRI) scnfilter.o $L/rdpickcoda.o
	cc -o $B/export_scnl_pri $(BINARIESPRI) scnfilter.o $L/rdpickcoda.o -lnsl -lsocket -mt -lposix4 -lthread -lc

export_gen_actv: $(BINARIESACTV) genericfilter.o
	cc -o $B/export_gen_actv $(BINARIESACTV) genericfilter.o -lnsl -lsocket -mt -lposix4 -lthread -lc

export_scnl_actv: $(BINARIESACTV) scnfilter.o $L/rdpickcoda.o
	cc -o $B/export_scnl_actv $(BINARIESACTV) scnfilter.o $L/rdpickcoda.o -lnsl -lsocket -mt -lposix4 -lthread -lc

export_ack: $(BINARIESACK) genericfilter.o
	cc -o $B/export_ack $(BINARIESACK) genericfilter.o -lnsl -lsocket -mt -lposix4 -lthread -lc

export_scnl_ack: $(BINARIESACK) scnfilter.o $L/rdpickcoda.o
	cc -o $B/export_scnl_ack $(BINARIESACK) scnfilter.o $L/rdpickcoda.o -lnsl -lsocket -mt -lposix4 -lthread -lc



lint:
	lint export.c export_ack.c export_actv.c export_scn_pri.c genericfilter.c scnfilter.c  $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/export_generic* $B/export_scnl* $B/export_*actv* $B/export_*ack*
