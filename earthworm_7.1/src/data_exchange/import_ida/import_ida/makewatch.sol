# Solaris makefile to build the debug utility 'watch'

IDATAP = ../idatap
CFLAGS += -I$(IDATAP)/include -mt -g

B =  $(EW_HOME)/$(EW_VERSION)/bin
L =  $(EW_HOME)/$(EW_VERSION)/lib

O = watch.o \
	$L/transport.o $L/getutil.o $L/kom.o $L/sleep_ew.o $L/logit_mt.o \
	$L/time_ew.o $L/sema_ew.o

watch: $O
	cc -o ./watch $O -lm -lposix4 -L$(IDATAP) -lidatap
