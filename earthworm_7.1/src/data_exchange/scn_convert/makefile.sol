#
#   This file is managed using Concurrent Versions System (CVS).
#
#    $Id: makefile.sol,v 1.6 2006/12/28 23:27:53 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.6  2006/12/28 23:27:53  lombard
#     Added version number, printed on startup.
#     Revised scnl2scn to provide complete mapping from SCNL back to SCN, using
#     configuration command similar to scn2scnl.
#
#     Revision 1.5  2004/10/19 21:59:41  lombard
#     *** empty log message ***
#
#     Revision 1.4  2004/10/19 21:54:04  lombard
#     Changes to support rules for renaming specific and wild-carded SCNs to
#     configured SCNLs.
#
#     Revision 1.3  2004/05/26 15:55:10  kohler
#     *** empty log message ***
#
#     Revision 1.2  2004/05/21 19:10:09  dietz
#     original version of makefile.sol for scn_convert directory
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

BIN = $L/kom.o $L/getutil.o $L/time_ew.o $L/chron3.o $L/logit.o \
      $L/transport.o $L/sleep_ew.o $L/chron3.o $L/rdpickcoda.o

all: scn2scnl scnl2scn

scn2scnl: scn2scnl.o scn_config.o to_pick_scnl.o to_coda_scnl.o \
	  scn_convert.o to_trace_scnl.o $(BIN) 
	cc -o $B/scn2scnl scn2scnl.o scn_config.o to_pick_scnl.o \
	  to_coda_scnl.o to_trace_scnl.o scn_convert.o $(BIN) -lm -lposix4
 
scnl2scn: scnl2scn.o scnl_config.o to_pick2k.o to_coda2k.o \
	  scnl_convert.o to_trace_scn.o $(BIN)
	cc -o $B/scnl2scn scnl2scn.o scnl_config.o to_pick2k.o \
      to_coda2k.o to_trace_scn.o scnl_convert.o $(BIN) -lm -lposix4
 
lint:
	lint scn2scnl.c scnl2scn.c scn_config.c scnl_config.c scn_convert.o \
      $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/scn2scnl* $B/scnl2scn*

