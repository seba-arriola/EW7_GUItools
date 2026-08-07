
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.5 2007/02/20 22:20:53 stefan Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.5  2007/02/20 22:20:53  stefan
#     lock libs
#
#     Revision 1.4  2006/04/04 18:16:05  stefan
#     startstop with reconfigure and libraries
#
#     Revision 1.3  2000/08/08 18:40:11  lucky
#     fixed lint directive
#
#     Revision 1.2  2000/08/08 18:12:53  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:38:25  lucky
#     Initial revision
#
#
#


B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


CFLAGS=-D_REENTRANT ${GLOBALFLAGS}


BINARIES = startstop.o $L/kom.o $L/logit_mt.o $L/getutil.o \
	$L/time_ew.o $L/transport.o $L/sleep_ew.o $L/sema_ew.o \
	$L/startstop_unix_generic.o $L/lockfile.o $L/lockfile_ew.o \
	$L/startstop_lib.o 

startstop: $(BINARIES)
	cc -o $B/startstop $(BINARIES) -lm -mt -lposix4 -lthread

.c.o:
	cc -c $(CFLAGS) $<


lint:
	lint startstop.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/startstop*
