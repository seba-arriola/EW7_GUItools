
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:15:53 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:15:53  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:17:36  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJS = evanstrig.o mteltrg.o mutils.o compare.o interpolate.o
LIBS = -lm -lposix4

EVANSTRIG = $(OBJS)\
    $L/kom.o $L/getutil.o $L/time_ew.o $L/chron3.o $L/logit.o \
    $L/transport.o $L/sleep_ew.o $L/swap.o

evanstrig: $(EVANSTRIG)
	cc -o $B/evanstrig $(EVANSTRIG) $(LIBS)

lint:
	lint evanstrig.c $(GLOBALFLAGS)



# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/evanstrig*
