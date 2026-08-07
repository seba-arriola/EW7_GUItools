#
#  Simply uses a script to make all components of rcv_ew
#


B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


all:
	./make.script



# Clean-up rules
clean:
	(cd STEIM123; rm -f a.out core *.o *.obj *% *~)
	(cd sunrcv; rm -f a.out core *.o *.obj *% *~)

clean_bin:
	rm -f $B/rcv* $B/station*
