#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2005/07/27 20:51:53 friberg Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2005/07/27 20:51:53  friberg
#     cleaned up errant space at the end of the B variable, gmake doesn't like it
#
#     Revision 1.2  2004/05/14 18:00:52  dietz
#     rdpickcoda moved to EW library
#
#     Revision 1.1  2004/04/22 18:01:56  dietz
#     Moved pkfilter source from Contrib/Menlo to the earthworm orthodoxy
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

SRCS = pkfilter.c

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJ = pkfilter.o $L/rdpickcoda.o $L/logit.o $L/getutil.o $L/time_ew.o \
      $L/chron3.o $L/transport.o $L/kom.o $L/sleep_ew.o 

pkfilter: $(OBJ)
	cc -o $B/pkfilter  $(OBJ) -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint pkfilter.c $(GLOBALFLAGS)

depend:
	makedepend -fmakefile.sol -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/pkfilter*
