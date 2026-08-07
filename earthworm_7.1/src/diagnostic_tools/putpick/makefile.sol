#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2004/06/24 18:58:14 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2004/06/24 18:58:14  dietz
#     New tool for writing pick/coda msgs to ring
#
#

CFLAGS = ${GLOBALFLAGS}
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

EWBINARIES = $L/chron3.o $L/getutil.o $L/kom.o $L/logit.o \
             $L/rdpickcoda.o $L/time_ew.o $L/transport.o $L/sleep_ew.o 

putpick: putpick.o $(EWBINARIES)
	cc -o $B/putpick putpick.o $(EWBINARIES) -lm -lposix4

lint:
	lint putpick.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/putpick*
