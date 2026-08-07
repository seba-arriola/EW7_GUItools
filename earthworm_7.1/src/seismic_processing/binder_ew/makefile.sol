
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2004/05/14 23:35:37 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2004/05/14 23:35:37  dietz
#     modified to work with TYPE_PICK_SCNL messages only
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 16:08:53  lucky
#     Initial revision
#
#     Revision 1.1  2000/02/14 16:07:49  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

BINDER = binder_ew.o bind.o grid.o hyp.o l1.o nrutil.o assess.o sample.o \
     ingelada.o $L/tlay.o $L/mnbrak.o $L/brent.o $L/logit_mt.o $L/getutil.o \
     $L/sema_ew.o $L/site.o $L/sleep_ew.o $L/kom.o $L/chron3.o $L/time_ew.o \
     $L/transport.o $L/rdpickcoda.o

binder_ew: $(BINDER)
	cc -o $B/binder_ew $(BINDER) -lm -mt -lposix4 -lthread

lint:
	lint binder_ew.c bind.c grid.c hyp.c l1.c nrutil.c \
			assess.c sample.c ingelada.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/binder_ew*

