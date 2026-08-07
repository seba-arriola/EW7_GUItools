
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2000/08/08 17:38:18 lucky Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2000/08/08 17:38:18  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:11:50  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

RINGTOCOAX = ringtocoax.o sender.o $L/logit.o $L/kom.o $L/getutil.o \
           $L/sleep_ew.o $L/socket_ew.o $L/time_ew.o $L/transport.o

ringtocoax: $(RINGTOCOAX)
	cc -o $B/ringtocoax $(RINGTOCOAX) -lm -lsocket -lnsl -lposix4


lint:
	lint ringtocoax.c sender.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ringtocoax*
