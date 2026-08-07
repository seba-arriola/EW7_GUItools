
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2006/07/11 23:34:15 kohler Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2006/07/11 23:34:15  kohler
#     Added new optional configuration parameter, DefDir.  At startup, diskmgr
#     changes the default directory to DefDir.  Then, it determines the free
#     space in DefDir.  This allows the user to specify the partition that
#     diskmgr works with.  WMK 7/11/2006
#
#     Revision 1.3  2004/03/13 18:58:53  lombard
#     added CFLAGS setting
#
#     Revision 1.2  2000/08/08 18:02:10  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:00:08  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJ = diskmgr.o $L/getavail.o $L/getutil.o $L/kom.o $L/logit.o \
         $L/sleep_ew.o $L/time_ew.o $L/transport.o  $L/dirops_ew.o

diskmgr: $(OBJ)
	cc -o $B/diskmgr $(OBJ) -lm -lposix4

lint:
	lint diskmgr.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/diskmgr*
