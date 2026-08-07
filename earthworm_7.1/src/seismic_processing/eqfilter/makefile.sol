
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.5 2001/02/01 01:42:52 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.5  2001/02/01 01:42:52  dietz
#     *** empty log message ***
#
#     Revision 1.4  2000/11/30 17:11:56  lombard
#     Removed several unnecessary objects and system libraries from link
#     command line, shrinking executeable by 25%. The unneeded objects
#     were ws_clientII, socket stuff, and swap; rather silly since eqfilter
#     doesn't talk to wave_servers.
#
#     Revision 1.3  2000/08/08 18:11:30  lucky
#     Added lint directive
#
#     Revision 1.2  2000/02/14 22:13:10  lucky
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 17:08:44  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT ${GLOBALFLAGS} 

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJ = eqfilter.o \
      area.o \
      $(L)/logit_mt.o \
      $(L)/chron3.o \
      $(L)/getutil.o \
      $(L)/kom.o \
      $(L)/sleep_ew.o \
      $(L)/threads_ew.o \
      $(L)/time_ew.o \
      $(L)/transport.o \
      $(L)/mem_circ_queue.o \
      $(L)/sema_ew.o \
      $(L)/parse_trig.o \
      $(L)/read_arc.o

eqfilter: $(OBJ); \
        cc -o $(B)/eqfilter $(OBJ) -lposix4 -lthread -lm


lint:
	lint eqfilter.c area.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/eqfilter*
