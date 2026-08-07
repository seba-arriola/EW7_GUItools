
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.8 2005/04/04 20:04:51 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.8  2005/04/04 20:04:51  dietz
#     added swap.o
#
#     Revision 1.7  2002/11/03 19:32:51  lombard
#     Added CFLAGS definition
#
#     Revision 1.6  2002/06/05 16:19:44  lucky
#     I don't remember
#
#     Revision 1.5  2001/03/27 01:13:54  dietz
#     *** empty log message ***
#
#     Revision 1.4  2001/02/09 22:00:31  dietz
#     Added file2ew to compilation list
#
#     Revision 1.3  2001/02/08 16:36:02  dietz
#     changed to produce an executable specific to each incoming
#     format.
#
#     Revision 1.2  2000/09/26 22:59:05  dietz
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 19:19:05  lucky
#     Initial revision
#
#
#

CFLAGS = ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

BINARIES = file2ew.o $L/chron3.o $L/logit.o $L/kom.o $L/getutil.o \
	   $L/sleep_ew.o $L/swap.o $L/time_ew.o $L/transport.o $L/dirops_ew.o $L/k2evt2ew.o 

SMBIN = $(BINARIES) $L/rw_strongmotionII.o 


all:
	make -f makefile.sol file2ew
	make -f makefile.sol sm_csmip2ew 
	make -f makefile.sol sm_nsmp2ew 
	make -f makefile.sol sm_redi2ew 
	make -f makefile.sol sm_terra2ew 
	make -f makefile.sol sm_tremor2ew 

file2ew: $(BINARIES) raw2ew.o
	cc -o $B/file2ew $(BINARIES) raw2ew.o  -lm -lposix4

sm_csmip2ew: $(SMBIN) csmip2ew.o
	cc -o $B/sm_csmip2ew $(SMBIN) csmip2ew.o  -lm -lposix4

sm_nsmp2ew: $(SMBIN) nsmp2ew.o
	cc -o $B/sm_nsmp2ew $(SMBIN) nsmp2ew.o  -lm -lposix4

sm_redi2ew: $(SMBIN) redi2ew.o
	cc -o $B/sm_redi2ew $(SMBIN) redi2ew.o  -lm -lposix4

sm_terra2ew: $(SMBIN) terra2ew.o
	cc -o $B/sm_terra2ew $(SMBIN) terra2ew.o  -lm -lposix4

sm_tremor2ew: $(SMBIN) tremor2ew.o
	cc -o $B/sm_tremor2ew $(SMBIN) tremor2ew.o  -lm -lposix4




# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/sm_*2ew* $B/file2ew*
