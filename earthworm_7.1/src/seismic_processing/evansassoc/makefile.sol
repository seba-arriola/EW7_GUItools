
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2002/11/03 19:14:16 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2002/11/03 19:14:16  lombard
#     Added CFLAGS definition
#
#     Revision 1.3  2000/08/08 18:36:52  lucky
#     fixed lint directive
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:15:33  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = evansassoc.o doit.o list.o time_dtos.o $L/kom.o $L/transport.o \
    $L/getutil.o $L/logit.o $L/sleep_ew.o $L/time_ew.o 

evansassoc: $O
	cc -o $B/evansassoc $O -lm -lposix4


lint:
	lint evansassoc.c doit.c list.c time_dtos.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/evansassoc*
