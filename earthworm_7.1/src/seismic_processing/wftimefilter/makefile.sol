#
#   THIS FILE IS UNDER CVS - 
#   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
#
#    $Id: makefile.sol,v 1.2 2005/07/27 20:52:41 friberg Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2005/07/27 20:52:41  friberg
#     cleaned up errant space at the end of the B variable, gmake doesn't like it
#
#     Revision 1.1  2005/05/10 22:54:34  dietz
#     New module to filter out time overlaps and bogus future timestamps
#     in waveform data.
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

SRCS = wftimefilter.c

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJ = wftimefilter.o $L/trheadconv.o $L/logit.o $L/getutil.o $L/time_ew.o \
      $L/chron3.o $L/transport.o $L/kom.o $L/sleep_ew.o $L/swap.o

wftimefilter: $(OBJ)
	cc -o $B/wftimefilter  $(OBJ) -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint wftimefilter.c $(GLOBALFLAGS)

depend:
	makedepend -fmakefile.sol -- $(CFLAGS) -- $(SRCS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/wftimefilter*
