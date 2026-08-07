#
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2004/06/25 18:27:27 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2004/06/25 18:27:27  dietz
#     modified to work with TYPE_TRACEBUF2 and location code
#
#     Revision 1.3  2002/11/03 19:41:18  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2001/05/01 23:42:39  bogaert
#     Added Clean functions
#
#     Revision 1.1  2001/04/26 17:43:54  kohler
#     Initial revision
#
#
#               Make file for reboot_mss_ew
#                      Solaris Version
#
#  The posix4 library is required for nanaosleep.
#
O = reboot_mss_ew.o rb_sol.o config.o
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

CFLAGS = ${GLOBALFLAGS}

reboot_mss_ew: $O
	cc -o $B/reboot_mss_ew $O $L/sleep_ew.o $L/kom.o $L/getutil.o $L/logit.o \
              $L/time_ew.o $L/trheadconv.o $L/transport.o $L/swap.o -lm -lsocket -lnsl -lposix4

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/reboot_mss_ew*
