
CFLAGS = $(GLOBALFLAGS) -Ilibslink

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

LS = libslink

LSLIBS = $(LS)/libslink.a

LSLD = -L$(LS) -lslink
SYSLIBS = -lnsl -lsocket -lposix4 -lm -lthread

EWLIBS = $L/kom.o $L/getutil.o $L/logit_mt.o $L/socket_ew_common.o\
           $L/transport.o $L/sleep_ew.o $L/socket_ew.o $L/time_ew.o \
           $L/threads_ew.o $L/sema_ew.o $L/swap.o $L/mem_circ_queue.o

PROGS = slink2ew

all: $(PROGS)
	cp $(PROGS) $B/

slink2ew: slink2ew.o $(EWLIBS) $(LSLIBS)
	cc -o slink2ew slink2ew.o $(EWLIBS) $(LSLD) $(SYSLIBS)

$(LS)/libslink.a:
	cd libslink; make -f Makefile static

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $(OUTPUT_OPTION) $<

lint:
	lint slink2ew.c $(CFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~ 
	cd libslink; make -f Makefile clean

clean_bin:
	rm -f $(PROGS)
	cd $B; rm -f $(PROGS)
