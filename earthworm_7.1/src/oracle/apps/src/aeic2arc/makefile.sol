#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2004/07/01 18:58:51 labcvs Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2004/07/01 18:58:51  labcvs
#     Moved aeic2arc from src/data_sources to /src/oracle/apps/src JMP
#
#     Revision 1.3  2002/11/03 18:47:40  lombard
#     Added RCS header and CFLAGS definition to makefile
#
#
#


#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
A = $(APPS_DIR)/lib


aeic2arc: aeic2arc.o aeic_2_ewevent.o
	cc -o $B/aeic2arc aeic2arc.o aeic_2_ewevent.o \
	$L/getutil.o  $L/kom.o $L/transport.o $L/sleep_ew.o \
	$L/dirops_ew.o $L/logit.o $L/time_ew.o $L/threads_ew.o \
	$L/sema_ew.o $L/mem_circ_queue.o $L/chron3.o $L/read_arc.o \
	$A/arc_2_ewevent.o $A/init_ewevent.o \
	-mt -lm -lsocket -lnsl -lposix4 -lc


lint:
	lint aeic2arc.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/aeic2arc*
