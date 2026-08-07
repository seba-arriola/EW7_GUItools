
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2004/05/11 18:14:18 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2004/05/11 18:14:18  dietz
#     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 16:56:25  lucky
#     Initial revision
#
#
#

CFLAGS = -O ${GLOBALFLAGS}
FFLAGS = 

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

LIBS = -lm -lposix4 -lthread

OBJS = decimate.o \
       configure.o \
       decthrd.o \
       do1stg.o \
       filtdecim.o \
       hqr.o \
       initpars.o \
       matchscn.o \
       readcnfg.o \
       readewh.o \
       remeznp.o \
       resetsta.o \
       setdecstg.o \
       setstafilt.o \
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
       $L/trheadconv.o \
       $L/sema_ew.o 


decimate: $(OBJS)
	cc $(CFLAGS) -o $B/decimate $(OBJS) $(LIBS)

lint:
	lint decimate.c configure.c decthrd.c do1stg.c filtdecim.c \
			hqr.c initpars.c matchscn.c readcnfg.c readewh.c remeznp.c \
			resetsta.c setdecstg.c setstafilt.c statrpt.c zeroes.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/decimate*


.c.o:
	cc -c ${CFLAGS} $<

