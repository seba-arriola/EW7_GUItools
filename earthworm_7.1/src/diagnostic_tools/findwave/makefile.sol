#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2002/11/03 18:53:29 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2002/11/03 18:53:29  lombard
#     Added RCS header and CFLAGS definition to makefile
#
#
#


CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = findwave.o $L/getutil.o $L/kom.o $L/transport.o $L/sleep_ew.o \
    $L/logit.o $L/time_ew.o $L/swap.o

findwave: $O
	cc -o $B/findwave $O -lm -lposix4

lint:
	lint findwave.c $(GLOBALFLAGS)



# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/findwave*
