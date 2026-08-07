
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:16:25 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:16:25  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:18:52  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


SRCS = ew2rsam.c
OBJS = ew2rsam.o

LIBS = -lm -lposix4

EW2RSAM = $(OBJS) $L/logit.o $L/kom.o $L/getutil.o $L/sleep_ew.o \
           $L/time_ew.o $L/transport.o $L/swap.o $L/mem_circ_queue.o \
		   $L/threads_ew.o $L/sema_ew.o 

ew2rsam: $(EW2RSAM)
	cc -o $B/ew2rsam $(EW2RSAM) $(LIBS) -lthread


lint:
	lint ew2rsam.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ew2rsam*
