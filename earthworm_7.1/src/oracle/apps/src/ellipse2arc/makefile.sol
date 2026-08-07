
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2004/07/01 19:08:16 labcvs Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2004/07/01 19:08:16  labcvs
#     Moved from src/data_sources to src/oracle/apps/src JMP
#
#     Revision 1.3  2002/11/03 18:48:59  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.2  2002/05/01 16:49:38  lucky
#     *** empty log message ***
#
#     Revision 1.1  2002/03/22 20:14:07  lucky
#     Initial revision
#
#     Revision 1.2  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 18:30:07  lucky
#     Initial revision
#
#
#
CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
A = $(APPS_DIR)/lib


ellipse2arc: ellipse2arc.o hypo71_2_ewevent.o
	cc -o $B/ellipse2arc ellipse2arc.o hypo71_2_ewevent.o \
	$L/getutil.o  $L/kom.o $L/transport.o $L/sleep_ew.o \
	$L/dirops_ew.o $L/logit.o $L/time_ew.o $L/threads_ew.o \
	$L/sema_ew.o $L/mem_circ_queue.o $L/chron3.o $L/read_arc.o \
	$A/arc_2_ewevent.o $A/init_ewevent.o \
	-mt -lm -lsocket -lnsl -lposix4 -lc


lint:
	lint ellipse2arc.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ellipse2arc*
