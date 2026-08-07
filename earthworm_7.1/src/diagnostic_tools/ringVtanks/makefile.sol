
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = ringvtanks.o $L/socket_ew.o $L/time_ew.o $L/sleep_ew.o $L/chron3.o \
     $L/logit.o $L/ws_clientII.o $L/socket_ew.o $L/socket_ew_common.o \
     $L/getutil.o $L/transport.o $L/swap.o $L/kom.o $L/threads_ew.o

ringvtanks: $O
	cc -o $B/ringvtanks $O -lm -lsocket -lnsl -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint ringvtanks.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ringvtanks*
