
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2002/03/22 20:16:14 lucky Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2002/03/22 20:16:14  lucky
#     Initial revision
#
#     Revision 1.4  2001/02/01 01:36:15  dietz
#     *** empty log message ***
#
#     Revision 1.3  2000/08/08 17:38:18  lucky
#     Added lint directive
#
#     Revision 1.2  2000/02/14 21:31:13  lucky
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 18:42:44  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


BINARIES = import_gen_pasv.o $L/kom.o $L/getutil.o $L/logit_mt.o $L/sema_ew.o \
           $L/transport.o $L/sleep_ew.o $L/socket_ew.o $L/time_ew.o \
           $L/threads_ew.o $L/socket_ew_common.o
          

import_gen_pasv: $(BINARIES)
	cc -o $B/import_gen_pasv $(BINARIES) -lnsl -lsocket -mt -lposix4 -lthread


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/import_gen_pasv*
