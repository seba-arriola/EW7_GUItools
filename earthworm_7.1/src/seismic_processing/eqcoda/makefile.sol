
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.5 2002/11/03 19:11:17 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.5  2002/11/03 19:11:17  lombard
#     Added CFLAGS definition
#
#     Revision 1.4  2001/12/12 19:18:19  dietz
#     Added stub program, feedeqcoda.
#
#     Revision 1.3  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.2  2000/07/21 23:09:24  dietz
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 17:07:37  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


eqcoda: eqcoda.o eqm2_calls.o stalist.o \
	$L/chron3.o $L/getutil.o $L/kom.o $L/logit.o \
	$L/time_ew.o $L/pipe.o 
	cc -o $B/eqcoda eqcoda.o eqm2_calls.o stalist.o \
	$L/chron3.o $L/getutil.o \
	$L/kom.o $L/logit.o $L/time_ew.o $L/pipe.o -lm -lposix4

feedeqcoda: feedeqcoda.o $L/getutil.o $L/kom.o $L/pipe.o $L/sleep_ew.o
	cc -o $B/feedeqcoda feedeqcoda.o $L/getutil.o $L/kom.o $L/pipe.o \
	$L/sleep_ew.o -lposix4	

lint:
	lint eqcoda.c eqm2_calls.c stalist.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/eqcoda*
