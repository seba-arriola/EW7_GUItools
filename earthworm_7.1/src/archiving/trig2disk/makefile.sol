
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.7 2007/02/07 05:46:36 stefan Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.7  2007/02/07 05:46:36  stefan
#     miniseed writer thanks to BGS, Richard Luckett
#
#     Revision 1.6  2002/02/21 16:17:50  cjbryan
#     added gseputaway, seiputaway, and seiutils; needed for new output formats
#     ,
#
#     Revision 1.5  2001/07/02 20:39:10  lucky
#     Added geo_to_km which is needed by sacputaway
#
#     Revision 1.4  2001/02/01 01:39:28  dietz
#     *** empty log message ***
#
#     Revision 1.3  2000/08/08 17:19:17  lucky
#     Added lint directive
#
#     Revision 1.2  2000/03/30 15:47:45  davidk
#     removed -XCC compiler flag that allows C++ comments.  Couldn't find
#     any C++ comments and this flag makes gcc choke.
#
#     Revision 1.1  2000/02/14 19:48:44  lucky
#     Initial revision
#
#
#


CFLAGS = -D_REENTRANT ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJ = trig2disk.o \
      CatPsuedoTrig.o \
      $L/putaway.o \
      $L/ahputaway.o \
      $L/sacputaway.o \
      $L/geo_to_km.o \
      $L/sudsputaway.o \
      $L/tankputaway.o \
      $L/gseputaway.o \
      $L/seiputaway.o \
      $L/seiutils.o \
      $L/logit_mt.o \
      $L/chron3.o \
      $L/dirops_ew.o \
      $L/getutil.o \
      $L/kom.o \
      $L/sleep_ew.o \
      $L/threads_ew.o \
      $L/time_ew.o \
      $L/transport.o \
      $L/socket_ew.o \
      $L/socket_ew_common.o \
      $L/mem_circ_queue.o \
      $L/sema_ew.o \
      $L/parse_trig.o \
      $L/ws_clientII.o \
      $L/swap.o \
      $L/mseedputaway.o \
      $L/libmseed.a


trig2disk: $(OBJ); \
        cc -o $(B)/trig2disk $(OBJ) -lsocket -lnsl -lposix4 -lthread -lm

lint:
	lint trig2disk.c CatPsuedoTrig.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/trig2disk*
