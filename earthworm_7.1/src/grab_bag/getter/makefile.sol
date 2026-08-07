
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:06:02 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:06:02  lombard
#     Added CFLAGS definition
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


getter: getter.o $L/getutil.o $L/kom.o $L/transport.o $L/sleep_ew.o
	cc -o $B/getter getter.o $L/getutil.o $L/kom.o $L/transport.o $L/sleep_ew.o \
	-lm -lposix4


lint:
	lint getter.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/getter*
