
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2004/04/30 18:36:40 kohler Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2004/04/30 18:36:40  kohler
#     gaplist now accepts both TYPE_TRACEBUF and TYPE_TRACEBUF2 messages.
#     WMK 4/30/04
#
#     Revision 1.3  2002/11/03 18:54:23  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.2  2000/08/08 17:54:53  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:39:05  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJ = gaplist.o config.o $L/chron3.o $L/getutil.o $L/kom.o $L/logit.o \
	 $L/time_ew.o $L/transport.o $L/sleep_ew.o $L/swap.o $L/trheadconv.o


gaplist: $(OBJ)
	cc -o $B/gaplist $(OBJ) -lm -lposix4

lint:
	lint gaplist.c config.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/gaplist*
