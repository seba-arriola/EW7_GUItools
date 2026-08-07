
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
#     Revision 1.1  2000/02/14 16:16:56  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJS = coaxtoring.o receiver_sol.o $L/threads_ew.o $L/getutil.o $L/kom.o \
        $L/logit_mt.o $L/transport.o $L/time_ew.o $L/sema_ew.o $L/socket_ew.o \
        $L/errexit.o $L/sleep_ew.o

coaxtoring: $(OBJS)
	cc -o $B/coaxtoring $(OBJS) \
           -mt -lm -lsocket -lnsl -lposix4 -lthread -lc

lint:
	lint coaxtoring.c receiver_sol.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/coaxtoring*
