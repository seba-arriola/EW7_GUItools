#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2000/08/08 17:38:18 lucky Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2000/08/08 17:38:18  lucky
#     Added lint directive
#
#     Revision 1.2  2000/03/06 18:57:59  lombard
#     Fixed targets clean and clean_bin.
#
#     Revision 1.1  2000/03/05 21:46:05  lombard
#     Initial revision
#
#
#


CFLAGS = $(GLOBALFLAGS) -IDCC_SEED/include -IDCC_SEED/include/seed

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

SL = DCC_SEED/lib
SEEDLIBS = $(SL)/libdcc_seed.a $(SL)/libsteim123.a $(SL)/libdcc_time.a \
    $(SL)/libdcc_misc.a

SEEDLD = -L$(SL) -ldcc_seed -lsteim123 -ldcc_time -ldcc_misc
SYSLIBS = -lnsl -lsocket -lposix4 -lm -lthread

EWLIBS = $L/kom.o $L/getutil.o $L/logit_mt.o $L/socket_ew_common.o\
           $L/transport.o $L/sleep_ew.o $L/socket_ew.o $L/time_ew.o \
           $L/threads_ew.o $L/sema_ew.o $L/swap.o $L/mem_circ_queue.o

E2LOBJS = ew2liss.o e2l_procthrd.o e2l_server.o

PROGS = liss2ew ew2liss dumpseed

all: $(PROGS)
	cp $(PROGS) $B/

liss2ew: liss2ew.o $(EWLIBS) $(SEEDLIBS)
	cc -o liss2ew liss2ew.o $(EWLIBS) $(SEEDLD) $(SYSLIBS)

ew2liss: $(E2LOBJS) $(EWLIBS) $(SEEDLIBS)
	cc -mt -o ew2liss $(E2LOBJS) $(EWLIBS) $(SEEDLD) $(SYSLIBS)

dumpseed: dumpseed.o readseedport.o $(SEEDLIBS)
	cc -o dumpseed dumpseed.o readseedport.o $(SEEDLD) $(SYSLIBS)

$(SL)/libdcc_seed.a:
	cd DCC_SEED; make -f makefile.sol dcc_seed

$(SL)/libsteim123.a:
	cd DCC_SEED; make -f makefile.sol steim123

$(SL)/libdcc_time.a:
	cd DCC_SEED; make -f makefile.sol dcc_time

$(SL)/libdcc_misc.a:
	cd DCC_SEED; make -f makefile.sol dcc_misc

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $(OUTPUT_OPTION) $<

lint:
	lint liss2ew.c ew2liss.c dumpseed.c readseedport.c $(CFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~ 
	cd DCC_SEED; make -f makefile.sol clean

clean_bin:
	rm -f $(PROGS)
	cd $B; rm -f $(PROGS)
