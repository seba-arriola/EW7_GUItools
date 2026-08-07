#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2003/02/14 19:43:39 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2003/02/14 19:43:39  dietz
#     Added naqsserTG (serial Tide Gauge data)
#
#     Revision 1.3  2002/11/04 18:41:45  dietz
#     Fixed so clean_bin doesn't remove source files *nmx*.
#
#     Revision 1.2  2002/03/15 23:10:09  dietz
#     *** empty log message ***
#
#     Revision 1.1  2001/06/20 22:34:52  dietz
#     Initial revision
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

BINARIES = nmxsrv_socket.o nmx_api.o nmxp_packet.o crc32.o\
           $L/kom.o $L/getutil.o $L/logit.o \
           $L/transport.o $L/sleep_ew.o $L/socket_ew.o $L/swap.o $L/time_ew.o \
           $L/threads_ew.o $L/socket_ew_common.o
          
all:
	make -f makefile.sol naqs2ew
	make -f makefile.sol naqssoh
	make -f makefile.sol naqsserTG
	make -f makefile.sol getmenu-nmx

naqs2ew: naqs2ew.o channels.o naqschassis.o $(BINARIES)
	cc -o $B/naqs2ew naqs2ew.o channels.o naqschassis.o $(BINARIES) -lm -lnsl -lsocket -lposix4 

naqssoh: naqssoh.o sohchannels.o naqschassis.o $(BINARIES)
	cc -o $B/naqssoh naqssoh.o sohchannels.o naqschassis.o $(BINARIES) -lm -lnsl -lsocket -lposix4 

naqsserTG: naqsserTG.o serchannels.o naqschassis.o $(BINARIES)
	cc -o $B/naqsserTG naqsserTG.o serchannels.o naqschassis.o $(BINARIES) -lm -lnsl -lsocket -lposix4


getmenu-nmx: getmenu-nmx.o $(BINARIES)
	cc -o $B/getmenu-nmx getmenu-nmx.o $(BINARIES) -lm -lnsl -lsocket -lposix4

dsclient: dschassis.o $(BINARIES)
	cc -o $B/dsclient dschassis.o $(BINARIES) -lm -lnsl -lsocket -lposix4 


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/naqs* $B/*nmx*
