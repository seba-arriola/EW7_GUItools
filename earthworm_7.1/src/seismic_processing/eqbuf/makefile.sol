
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2003/05/06 20:53:39 davidk Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2003/05/06 20:53:39  davidk
#     *** empty log message ***
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:02:31  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


eqbuf: eqbuf.o $L/kom.o $L/getutil.o $L/pipe.o $L/mem_circ_queue.o \
	$L/logit.o $L/sema_ew.o $L/sleep_ew.o $L/time_ew.o $L/threads_ew.o
	cc -o $B/eqbuf eqbuf.o $L/kom.o $L/getutil.o \
	$L/logit.o $L/pipe.o $L/sema_ew.o $L/sleep_ew.o $L/time_ew.o \
        $L/mem_circ_queue.o $L/threads_ew.o -mt -lposix4 -lthread -lc

lint:
	lint eqbuf.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/eqbuf*
