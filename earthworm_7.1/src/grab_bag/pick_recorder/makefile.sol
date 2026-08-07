
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2005/08/29 20:19:53 friberg Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2005/08/29 20:19:53  friberg
#     added CFLAGS
#
#     Revision 1.3  2005/07/27 20:49:57  friberg
#     added CFLAGS=-D_SPARC -D_SOLARIS -I/home/isti/development_2.8/v7.0_working/include -I/home/isti/development_2.8/v7.0_working/src/oracle/schema-working/src/include -I/home/isti/development_2.8/v7.0_working/src/oracle/schema-working/src/include/internal -I/home/isti/development_2.8/v7.0_working/src/oracle/apps/src/include -I/opt/oracle/rdbms/demo to get it to work on Solaris
#
#     Revision 1.2  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.1  2000/07/24 21:02:15  lucky
#     Initial revision
#
#     Revision 1.1  2000/02/14 17:18:52  lucky
#     Initial revision
#
#
#

CFLAGS=$(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


SRCS = pick_recorder.c
OBJS = pick_recorder.o

LIBS = -lm -lposix4

PICK_REC = $(OBJS) $L/logit.o $L/kom.o $L/getutil.o $L/sleep_ew.o \
           $L/time_ew.o $L/transport.o $L/swap.o $L/mem_circ_queue.o \
		   $L/threads_ew.o $L/sema_ew.o 

pick_recorder: $(PICK_REC)
	cc $(CFLAGS) -o $B/pick_recorder $(PICK_REC) $(LIBS) -lthread


lint:
	lint pick_recorder.c $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/pick_recorder*
