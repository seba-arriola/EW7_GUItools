#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 18:55:00 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 18:55:00  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.2  2000/08/08 17:54:53  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:41:21  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = getmenu.o $L/socket_ew.o $L/time_ew.o $L/sleep_ew.o $L/chron3.o \
     $L/logit.o $L/ws_clientII.o $L/socket_ew_common.o

getmenu: $O
	cc -o $B/getmenu $O -lm -lsocket -lnsl -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint getmenu.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/getmenu*
