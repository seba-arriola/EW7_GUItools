
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.4 2002/11/03 05:08:33 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.4  2002/11/03 05:08:33  lombard
#     Added missing CFLAGS definition,
#     Cleaned up compile target,
#
#     Revision 1.3  2001/05/04 16:43:17  bogaert
#     removed references to 'arcfile2ring', which is replaced by file2ring.
#
#     Revision 1.2  2000/08/08 17:19:17  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 18:56:41  lucky
#     Initial revision
#
#
#

CFLAGS = $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


MREPORT = menlo_report.o $L/logit.o $L/time_ew.o $L/kom.o  \
          $L/getutil.o $L/sleep_ew.o $L/transport.o $L/copyfile.o $L/dirops_ew.o

all: menlo_report

menlo_report: $(MREPORT)
	cc -o $B/menlo_report $(MREPORT)  -lm -lposix4


.c.o:
	cc -c $(CFLAGS)  $<


lint:
	lint menlo_report.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f  $B/menlo_report*
