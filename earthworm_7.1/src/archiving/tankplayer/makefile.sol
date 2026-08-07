
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 05:10:37 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 05:10:37  lombard
#     Removed KEEP_STATE line
#
#     Revision 1.2  2000/08/08 17:19:17  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:41:59  lucky
#     Initial revision
#
#
#



CFLAGS = $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


BINARIES = tankplayer.o $L/getutil.o $L/kom.o $L/logit.o \
           $L/sleep_ew.o $L/swap.o $L/time_ew.o $L/transport.o

tankplayer: $(BINARIES)
	cc -o $B/tankplayer ${CFLAGS} $(BINARIES)  -lposix4

lint:
	lint tankplayer.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/tankplayer*
