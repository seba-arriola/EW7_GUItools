#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2006/12/12 20:01:50 paulf Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2006/12/12 20:01:50  paulf
#     added makefile.ux
#
#     Revision 1.1  2006/11/22 23:06:39  stefan
#     menlo contrib
#
#     Revision 1.1.1.1  2004/04/28 23:15:44  dietz
#     pre-location code Contrib/Menlo
#
#
#
CFLAGS = $(GLOBALFLAGS)

SRCS = condenselogo.c

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJ = condenselogo.o $L/logit.o $L/getutil.o $L/time_ew.o \
      $L/transport.o $L/kom.o $L/sleep_ew.o

condenselogo: $(OBJ)
	cc -o $B/condenselogo $(OBJ) -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint condenselogo.c $(GLOBALFLAGS)

depend:
	makedepend -fmakefile.sol -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/condenselogo*
