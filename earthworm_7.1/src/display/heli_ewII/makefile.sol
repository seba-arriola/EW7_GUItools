
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2001/08/07 16:59:57 lucky Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2001/08/07 16:59:57  lucky
#     Initial revision
#
#     Revision 1.3  2000/08/08 17:56:20  lucky
#     Added lint directive
#
#     Revision 1.2  2000/02/14 18:36:47  lucky
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 18:36:15  lucky
#     Initial revision
#
#
#	Solaris Makefile: heli_ewII

CFLAGS = -g -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

BINARIES = heli_ewII.o  \
		$L/logit.o $L/getutil.o  $L/transport.o $L/kom.o \
		$L/sleep_ew.o $L/time_ew.o $L/parse_trig.o \
		$L/getavail.o $L/getsysname_ew.o $L/chron3.o\
		$L/socket_ew.o $L/socket_ew_common.o $L/ws_clientII.o\
		$L/gd.o $L/gdfontt.o $L/gdfonts.o $L/gdfontmb.o \
		$L/gdfontl.o $L/gdfontg.o

heli1: $(BINARIES)
	cc -o $B/heli_ewII $(BINARIES) -lsocket -lnsl -lm -lposix4 -lc

.c.o:
	$(CC) $(CFLAGS) -g $(CPPFLAGS) -c  $(OUTPUT_OPTION) $<

lint:
	lint heli_ewII.c $(GLOBALFLAGS)
	
# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/heli_ewII*
