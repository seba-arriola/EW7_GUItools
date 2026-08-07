# @(#)makefile.sol	1.3 07/01/98
# USGS style makefile for reftek2ew
#
# This makefile requires that the Reftek RTP libraries already exist
# on your system.  If you don't have it, get it from
#
#        ftp://ftp.reftek.com:/pub/rtp
#
# and follow the directions therein to build the library.  Specify the
# directory containing the libraries and include files as shown:

RTPLDIR = ./lib
RTPIDIR = ./

#CFLAGS = ${GLOBALFLAGS} -I$(RTPIDIR) -mt
CFLAGS = ${GLOBALFLAGS} -I$(RTPIDIR) 

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


O = main.o hbeat.o init.o notify.o params.o scn.o send.o terminate.o \
    samprate.o $L/transport.o $L/getutil.o $L/kom.o $L/logit.o $L/sema_ew.o \
    $L/sleep_ew.o $L/time_ew.o $L/threads_ew.o

all: libs reftek2ew

libs: FORCE
	cd lib; make -f makefile.sol


reftek2ew: $O
#	cc -o $B/reftek2ew $O -L$(RTPLDIR) -lrtp -lreftek -lutil -lposix4 -lsocket -lnsl -mt -lm 
	cc -o $B/reftek2ew $O -L$(RTPLDIR) -lrtp -lreftek -lutil -lposix4 -lsocket -lnsl -mt -lm -lthread

clean: FORCE
	rm -f a.out core *.o *.obj *% *~
	cd lib; make -f makefile.sol clean

clean_bin: FORCE
	rm -f $B/reftek2ew*

FORCE:
