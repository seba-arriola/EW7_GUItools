
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:08:37 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:08:37  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2000/08/08 18:02:10  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:39:55  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJ = statmgr.o files.o pageout.o $L/kom.o $L/logit.o $L/getutil.o \
    $L/sendmail.o $L/sleep_ew.o $L/time_ew.o $L/transport.o $L/errexit.o

statmgr: $(OBJ)
	cc -g -o $B/statmgr $(OBJ) -lc -lm -lposix4


statmgr.o: statmgr.c statmgr.h


lint:
	lint statmgr.c files.c pageout.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/statmgr*
