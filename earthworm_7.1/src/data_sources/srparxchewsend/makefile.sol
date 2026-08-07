# FILE: makefile.sol                 Copyright (c), Symmetric Research, 2004
#
#                Nmake file for srparxchewsend  - Solaris version
#
#           SOLARIS VERSION NOT SUPPORTED (NO SOLARIS PARxCH DRIVERS)
#

CFLAGS=${GLOBALFLAGS} -g

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

SRPARXCHEWSEND = srparxchewsend.o parxch.o pargps.o srhelper.o \
           $L/logit.o $L/kom.o $L/getutil.o $L/sleep_ew.o $L/time_ew.o $L/transport.o    

srparxchewsend: $(SRPARXCHEWSEND)
	echo "srparxchewsend not built under Solaris since no PARxCH hardware drivers available"


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/srparxchewsend*
