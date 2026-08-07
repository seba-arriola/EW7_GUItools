
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 18:55:42 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 18:55:42  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.2  2000/08/08 17:54:53  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:31:49  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


sniffring: sniffring.o $L/getutil.o $L/kom.o $L/sleep_ew.o $L/transport.o $L/errexit.o
	cc -o $B/sniffring sniffring.o $L/getutil.o $L/kom.o $L/transport.o \
          $L/sleep_ew.o $L/errexit.o -lposix4

lint:
	lint sniffring.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/sniffring*
