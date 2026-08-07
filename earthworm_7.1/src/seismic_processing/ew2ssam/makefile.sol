
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:16:43 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:16:43  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2001/08/07 17:01:54  bogaert
#     *** empty log message ***
#
#     Revision 1.1  2001/05/03 23:53:03  bogaert
#     Initial revision
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


SRCS = ew2ssam.c
OBJS = ew2ssam.o

LIBS = -lm -lposix4

EW2RSAM = $(OBJS) $L/logit.o $L/kom.o $L/getutil.o $L/sleep_ew.o \
           $L/time_ew.o $L/transport.o $L/swap.o $L/mem_circ_queue.o \
		   $L/threads_ew.o $L/sema_ew.o 

ew2ssam: $(EW2RSAM)
	cc -o $B/ew2ssam $(EW2RSAM) $(LIBS) -lthread


lint:
	lint ew2ssam.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ew2ssam*
