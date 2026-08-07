B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


harley2hinv: harley2hinv.o
	cc -o $B/harley2hinv harley2hinv.o -lm -lposix4

lint:
	lint harley2hinv.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/harley2hinv*
