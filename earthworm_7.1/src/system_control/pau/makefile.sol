
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2002/11/03 19:34:01 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2002/11/03 19:34:01  lombard
#     Added CFLAGS definition
#
#     Revision 1.3  2001/05/11 20:47:13  dietz
#     *** empty log message ***
#
#     Revision 1.2  2000/08/08 18:12:53  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 19:04:51  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJS = setflags.o $L/dirops_ew.o $L/kom.o $L/getutil.o \
       $L/sleep_ew.o $L/transport.o $L/logit.o $L/time_ew.o    

all:
	make -f makefile.sol pau
	make -f makefile.sol pidpau

pau: pau.o $(OBJS)
	cc -o $B/pau pau.o $(OBJS)  -lposix4

pidpau: pidpau.o $(OBJS)
	cc -o $B/pidpau pidpau.o $(OBJS)  -lposix4

lint:
	lint pau.c $(GLOBALFLAGS)
	lint pidpau.c $(GLOBALFLAGS)
	lint setflags.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/*pau*
