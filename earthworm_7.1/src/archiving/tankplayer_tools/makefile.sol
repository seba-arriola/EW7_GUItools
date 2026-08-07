#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.6 2007/01/02 15:26:45 stefan Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.6  2007/01/02 15:26:45  stefan
#     clean_bin tweak for ms2tb
#
#     Revision 1.5  2006/11/17 17:21:23  ilya
#     Added path to QLIB2 in makefile.sol
#
#     Revision 1.4  2006/08/08 20:53:15  paulf
#     added ms2tb to makefile.sol
#
#     Revision 1.3  2005/10/14 23:06:18  dietz
#     *** empty log message ***
#
#     Revision 1.2  2002/11/03 05:14:38  lombard
#     Cleaned up makefile
#
#
#
#IGD 2006/11/17  Added QLIB2 location required for ms2tb
QLIB_DIR = ../../libsrc/qlib2/

CFLAGS = $(GLOBALFLAGS) -I$(QLIB_DIR)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

LIBS = $L/swap.o $L/time_ew.o $L/logit.o

all: remux_tbuf sac2tb ms2tb
	make -f makefile.sol remux_tbuf
	make -f makefile.sol sac2tb
	
remux_tbuf: remux_tbuf.o $(LIBS)
	cc -g -o $B/remux_tbuf remux_tbuf.o $(LIBS) -lrt
	
sac2tb: sac2tb.o $(LIBS)
	cc -g -o $B/sac2tb sac2tb.o $(LIBS) -lrt

ms2tb: ms2tb.o $(LIBS) read_mseed_data.o
	cc -g -o $B/ms2tb ms2tb.o read_mseed_data.o -L$(L) -L$(QLIB_DIR) -lqlib2nl -lm

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/remux_tbuf* $B/sac2tb* $B/ms2tb*
