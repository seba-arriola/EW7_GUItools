
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.3 2002/11/03 19:13:38 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.3  2002/11/03 19:13:38  lombard
#     Added CFLAGS definition
#
#     Revision 1.2  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 17:13:57  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


all:
	make -f makefile.sol eqverify
	make -f makefile.sol arcfeeder

eqverify: eqverify.o $L/chron3.o $L/getutil.o $L/kom.o $L/logit.o \
	$L/time_ew.o $L/pipe.o $L/sleep_ew.o
	cc -o $B/eqverify eqverify.o $L/chron3.o $L/getutil.o \
	$L/kom.o $L/logit.o $L/time_ew.o $L/pipe.o $L/sleep_ew.o -lm -lposix4 

arcfeeder: arcfeeder.o $L/getutil.o $L/kom.o $L/pipe.o $L/sleep_ew.o
	cc -o $B/arcfeeder arcfeeder.o $L/getutil.o $L/kom.o $L/pipe.o \
	$L/sleep_ew.o -lposix4	


lint:
	lint eqverify.c arcfeeder.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/eqverify* $B/arcfeeder*
