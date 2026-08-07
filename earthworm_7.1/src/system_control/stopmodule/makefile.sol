
#
#	make file for stopmodule
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = stopmodule.o $L/dirops_ew.o $L/time_ew.o $L/transport.o $L/sleep_ew.o \
    $L/getutil.o $L/kom.o $L/logit.o 

stopmodule: ${O}
	cc -g -o $B/stopmodule ${O} -lc -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint stopmodule.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/stopmodule*
