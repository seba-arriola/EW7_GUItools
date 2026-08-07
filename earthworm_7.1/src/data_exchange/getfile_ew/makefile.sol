
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2002/11/03 19:04:18 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2002/11/03 19:04:18  lombard
#     Added CFLAGS definition
#
#     Revision 1.1  2002/03/22 19:59:04  lucky
#     Initial revision
#
#     Revision 1.1  2002/03/22 19:58:02  lucky
#     Initial revision
#
#     Revision 1.2  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 18:30:07  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


getfile_ew: getfile_ew.o getfile_socket.o 
	cc -o $B/getfile_ew getfile_ew.o getfile_socket.o \
	$L/getutil.o $L/kom.o $L/dirops_ew.o $L/transport.o \
	$L/sleep_ew.o $L/logit.o  $L/time_ew.o $L/threads_ew.o \
	$L/socket_ew.o $L/socket_ew_common.o \
	-mt -lm -lsocket -lnsl -lposix4 -lc


lint:
	lint getfile_ew.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/getfile_ew*
