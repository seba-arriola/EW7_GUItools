#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2002/11/03 18:52:27 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2002/11/03 18:52:27  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.1  2001/02/09 21:29:44  dietz
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


file2ring: file2ring.o $L/getutil.o $L/transport.o $L/sleep_ew.o
	cc -o $B/file2ring file2ring.o $L/getutil.o $L/transport.o \
	$L/sleep_ew.o $L/kom.o -lposix4	


lint:
	lint file2ring.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/file2ring*
