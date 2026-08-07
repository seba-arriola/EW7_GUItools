
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2005/06/17 14:53:18 davidk Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2005/06/17 14:53:18  davidk
#     Added logit object that appeared to be required by program.
#     (copying what I did to the NT makefile).
#
#     Revision 1.3  2005/04/04 20:31:11  dietz
#     added -lrt to the link link (needed for clock_gettime in time_ew.c)
#
#     Revision 1.2  2004/08/09 16:48:02  davidk
#     Added time_ew.o to required object list.  It is needed by chron3.o
#
#     Revision 1.1  2001/08/30 07:53:38  dietz
#     Initial revision
#
#     Revision 1.2  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 18:31:49  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

dumpwave: dumpwave.o
	cc -o $B/dumpwave ${CFLAGS} dumpwave.o $L/swap.o $L/chron3.o $L/time_ew.o  $L/logit.o -lrt

.KEEP_STATE:


lint:
	lint dumpwave.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/dumpwave* 
