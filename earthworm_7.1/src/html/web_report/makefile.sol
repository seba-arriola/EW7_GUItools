
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


MREPORT = web_report.o $L/logit.o $L/time_ew.o $L/kom.o  \
          $L/getutil.o $L/sleep_ew.o $L/transport.o $L/copyfile.o $L/dirops_ew.o

all:
	make -f makefile.sol web_report

web_report: $(MREPORT)
	cc -o $B/web_report $(MREPORT)  -lm -lposix4



lint:
	lint web_report.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f  $B/web_report*
