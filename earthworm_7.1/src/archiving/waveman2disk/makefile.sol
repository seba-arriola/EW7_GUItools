
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.8 2007/02/07 05:45:33 stefan Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.8  2007/02/07 05:45:33  stefan
#     miniseed writer thanks to BGS, Richard Luckett
#
#     Revision 1.7  2002/02/20 23:57:24  cjbryan
#     added gseputaway, seiputaway, and seiutils; needed for new output formats
#
#     Revision 1.6  2001/07/02 20:41:31  lucky
#     added geo_to_km; needed by sacputaway
#
#     Revision 1.5  2001/04/12 04:26:55  lombard
#     *** empty log message ***
#
#     Revision 1.4  2001/02/01 01:40:05  dietz
#     *** empty log message ***
#
#     Revision 1.3  2000/08/08 17:19:17  lucky
#     Added lint directive
#
#     Revision 1.2  2000/03/30 15:49:58  davidk
#     commented out -xCC flag that allows C++ comments.  Couldn't find any
#     C++ comments and this flag makes gcc choke.
#
#     Revision 1.1  2000/02/14 20:02:23  lucky
#     Initial revision
#
#
#



CFLAGS = -D_REENTRANT ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


OBJ = waveman2disk.o \
      CatPsuedoTrig.o

EWLIBS =  $L/putaway.o \
      $L/ahputaway.o \
      $L/sacputaway.o \
      $L/geo_to_km.o \
      $L/sudsputaway.o \
      $L/tankputaway.o \
      $L/gseputaway.o \
      $L/seiputaway.o \
      $L/seiutils.o \
      $L/dirops_ew.o \
      $L/logit.o \
      $L/kom.o \
      $L/sleep_ew.o \
      $L/time_ew.o \
      $L/socket_ew.o \
      $L/socket_ew_common.o \
      $L/parse_trig.o \
      $L/ws_clientII.o \
      $L/swap.o \
      $L/chron3.o \
      $L/mseedputaway.o \
      $L/libmseed.a 


waveman2disk: $(OBJ); \
        cc -o $(B)/waveman2disk $(OBJ) $(EWLIBS) -lsocket -lnsl -lposix4 -lm

lint:
	lint waveman2disk.c CatPsuedoTrig.c $(GLOBALFLAGS)
	

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/waveman2disk
