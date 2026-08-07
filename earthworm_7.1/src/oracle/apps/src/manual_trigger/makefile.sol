
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2004/07/01 19:13:35 labcvs Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2004/07/01 19:13:35  labcvs
#     Moved from src/data_sources to src/oracle/apps/src JMP
#
#     Revision 1.3  2002/11/03 18:50:14  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.2  2002/05/01 16:49:38  lucky
#     *** empty log message ***
#
#     Revision 1.1  2001/11/12 18:16:01  lucky
#     Initial revision
#
#     Revision 1.2  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:08:08  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
A = $(APPS_DIR)/lib


manual_trigger: manual_trigger.o $L/getutil.o $L/parse_trig.o $L/transport.o $L/sleep_ew.o
	cc -o $B/manual_trigger manual_trigger.o $L/getutil.o $L/kom.o \
			$L/transport.o $L/sleep_ew.o $L/parse_trig.o $L/chron3.o \
			$L/logit.o $L/time_ew.o  $L/read_arc.o \
			$A/arc_2_ewevent.o $A/init_ewevent.o -lm -lposix4


lint:
	lint manual_trigger.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/manual_trigger*
