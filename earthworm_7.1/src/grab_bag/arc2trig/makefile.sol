#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2005/08/29 20:05:30 friberg Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2005/08/29 20:05:30  friberg
#     removed errant space after B variable for bin
#
#     Revision 1.3  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.2  2000/02/14 21:26:54  lucky
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 16:04:49  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)
SRCS = arc2trig.c writetrig.c

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

ARC = arc2trig.o writetrig.o $L/chron3.o $L/logit.o $L/getutil.o \
     $L/read_arc.o $L/transport.o $L/kom.o $L/sleep_ew.o $L/time_ew.o

arc2trig: $(ARC)
	cc -o $(B)/arc2trig $(ARC) -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint arc2trig.c writetrig.c $(GLOBALFLAGS)

depend:
	makedepend -fmakefile.sol -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/arc2trig*
