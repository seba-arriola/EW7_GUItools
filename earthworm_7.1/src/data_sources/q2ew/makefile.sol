#
# q2ew - quanterra to earthworm interface
#

CC     = cc
CFLAGS = -D_REENTRANT ${GLOBALFLAGS}

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

#IGD 2006/11/16 Moved qlib2 to libsrc
QLIB_DIR = ../../libsrc/qlib2/
COMSERV_INC = ./comserv/include
COMSERV_LIB = ./comserv/util

CFLAGS +=  -g -I$(QLIB_DIR) -I$(COMSERV_INC)
#CFLAGS += -xCC -I$(QLIB_DIR) -I$(COMSERV_INC)

all: qlib2 comserv q2ew

qlib2:  FORCE
	@echo "Making qlib2"
	cd $(QLIB_DIR); make

comserv:  FORCE
	@echo "Making comserv"
	cd comserv; make -f makefile.sol

SRCS = convert.c getconfig.c main.c scn_map.c cs_status.c \
	heart.c misc_seed_utils.c die.c logo.c options.c

OBJS = convert.o getconfig.o main.o scn_map.o cs_status.o \
	heart.o misc_seed_utils.o die.o logo.o options.o
 
EW_LIBS = \
	$L/getutil.o \
	$L/kom.o \
	$L/logit_mt.o \
	$L/sema_ew.o \
	$L/threads_ew.o \
	$L/time_ew.o \
	$L/sleep_ew.o \
	$L/transport.o

# IGD 2006/11/16 Note that we use qlib2nl: no-leap-seconds version of Qlib2
q2ew: $(OBJS); \
        $(CC) -o $(B)/q2ew $(OBJS) $(EW_LIBS) -L$(QLIB_DIR) -L$(COMSERV_LIB) -lqlib2nl -lutil -lsocket -lnsl -lposix4 -lthread -lm

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~
	(cd comserv; make -f makefile.sol clean)
	(cd $(QLIB_DIR); make clean)

clean_bin:
	rm -f $B/q2ew*

FORCE:
