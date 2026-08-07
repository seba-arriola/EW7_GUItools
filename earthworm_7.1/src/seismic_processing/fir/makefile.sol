
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2004/07/28 22:43:04 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2004/07/28 22:43:04  lombard
#     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:27:23  lucky
#     Initial revision
#
#
#

CFLAGS = -O ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


SRCS = fir.c
OBJS = fir.o

LIBS = -lm -lposix4 -lthread

OBJS = fir.o \
       bandcom.o \
       configure.o \
       filt1scn.o \
       firfilt.o \
       firthrd.o \
       hqr.o \
       initpars.o \
       initsta.o \
       matchscn.o \
       readcnfg.o \
       readewh.o \
       remeznp.o \
       resetsta.o \
       setfilt.o \
       statrpt.o \
       zeroes.o \
       $L/logit_mt.o \
       $L/kom.o \
       $L/getutil.o \
       $L/sleep_ew.o \
       $L/time_ew.o \
       $L/transport.o \
       $L/swap.o \
       $L/mem_circ_queue.o \
       $L/threads_ew.o \
       $L/sema_ew.o 

fir: $(OBJS)
	cc ${CFLAGS} -o $B/fir $(OBJS) $(LIBS)

lint:
	lint fir.c bandcom.c configure.c filt1scn.c firfilt.c \
			firthrd.c hqr.c initpars.c initsta.c matchscn.c \
			readcnfg.c readewh.c remeznp.c resetsta.c setfilt.c \
			statrpt.c zeroes.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *% *~

clean_bin:
	rm -f $B/fir


.c.o:
	cc -c ${CFLAGS} $<

