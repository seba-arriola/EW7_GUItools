
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:34:46 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:34:46  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2000/08/08 18:12:53  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:40:54  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = status.o  $L/time_ew.o $L/transport.o $L/sleep_ew.o \
    $L/getutil.o $L/dirops_ew.o $L/kom.o $L/logit.o

status: ${O}
	cc -g -o $B/status ${O} -lc -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<


lint:
	lint status.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/status*
