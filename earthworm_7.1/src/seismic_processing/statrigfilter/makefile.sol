#
#   THIS FILE IS UNDER CVS - 
#   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
#
#    $Id: makefile.sol,v 1.1 2005/11/23 18:56:31 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2005/11/23 18:56:31  dietz
#     New module for filtering individual station triggers. Heavily based
#     on the code from pkfilter.
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

SRCS = statrigfilter.c

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJ = statrigfilter.o $L/logit.o $L/getutil.o $L/time_ew.o \
      $L/chron3.o $L/transport.o $L/kom.o $L/sleep_ew.o 

statrigfilter: $(OBJ)
	cc -o $B/statrigfilter  $(OBJ) -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint statrigfilter.c $(GLOBALFLAGS)

depend:
	makedepend -fmakefile.sol -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/statrigfilter*
