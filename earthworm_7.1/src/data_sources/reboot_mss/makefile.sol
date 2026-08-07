#
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2002/11/03 18:51:08 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2002/11/03 18:51:08  lombard
#     Added CFLAGS definition to makefile
#
#     Revision 1.3  2001/05/01 23:43:34  bogaert
#     *** empty log message ***
#
#     Revision 1.2  2001/05/01 23:40:42  bogaert
#     added Clean functions.
#
#     Revision 1.1  2001/04/26 17:30:25  kohler
#     Initial revision
#
#
#                 Make file for reboot_mss
#                      Solaris Version
#
#  The posix4 library is required for nanaosleep.
#
O = reboot_mss.o rb_mss.o rb_sol.o
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
CFLAGS = ${GLOBALFLAGS}

reboot_mss: $O
	cc -o $B/reboot_mss $O $L/sleep_ew.o -lm -lsocket -lnsl -lposix4

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/reboot_mss*
