
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

CFLAGS = ${GLOBALFLAGS}



O = getstation.o $L/socket_ew.o $L/time_ew.o $L/sleep_ew.o $L/chron3.o \
     $L/logit.o $L/ws_clientII.o $L/socket_ew_common.o $L/sleep_ew.o $L/kom.o

getstation: $O
	cc -o $B/getstation $O -lm -lsocket -lnsl -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint getstation.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/getstation*
