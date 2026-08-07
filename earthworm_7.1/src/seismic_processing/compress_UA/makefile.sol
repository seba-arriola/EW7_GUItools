
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:09:30 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:09:30  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 16:22:40  lucky
#     Initial revision
#
#     Revision 1.1  2000/02/14 16:22:08  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

SRCS = compress_UA.c
OBJS = compress_UA.o

LIBS = -lm -lposix4

COMPRESS = $(OBJS) $L/logit.o $L/kom.o $L/getutil.o $L/sleep_ew.o \
           $L/time_ew.o $L/transport.o $L/swap.o

compress_UA: $(COMPRESS)
	cc -o $B/compress_UA $(COMPRESS) $(LIBS)

lint:
	lint compress_UA.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~ 

clean_bin:
	rm -f $B/compress_UA*
