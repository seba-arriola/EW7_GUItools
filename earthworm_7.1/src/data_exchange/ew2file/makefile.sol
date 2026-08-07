#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2002/12/20 02:36:33 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2002/12/20 02:36:33  lombard
#     Initial revision
#
#
#

CFLAGS = -g -D_REENTRANT $(GLOBALFLAGS)

APP = ew2file
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

BINARIES = ew2file.o $L/mem_circ_queue.o $L/kom.o \
           $L/getsysname_ew.o $L/getutil.o $L/logit_mt.o $L/transport.o \
	       $L/sleep_ew.o $L/time_ew.o $L/threads_ew.o $L/sema_ew.o  $L/dirops_ew.o

all: $B/$(APP)

$B/$(APP): $(BINARIES)
	cc -o $B/ew2file $(CFLAGS) $(BINARIES) -mt -lposix4 -lthread


lint:
	lint ew2file.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o

clean_bin:
	rm -f $B/$(APP)
