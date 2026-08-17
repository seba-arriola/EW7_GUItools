# Gets the folder name (e.g. loc_wcatwc)
MODULE = $(notdir $(CURDIR))

CC = gcc
# Compilation flags (no GTK)
CFLAGS = -Wall -O2 -I../../libsrc -I$(EW_HOME)/$(EW_VERSION)/include -fcommon -D_LINUX -D_REENTRANT -D_FORTIFY_SOURCE=0 -fno-stack-protector -U_FORTIFY_SOURCE  

EW_LIB = $(EW_HOME)/$(EW_VERSION)/lib
# Base Earthworm libraries required by ATWC
# Base Earthworm libraries required by ATWC
EW_OBJS = $(EW_LIB)/transport.o \
          $(EW_LIB)/getutil.o \
          $(EW_LIB)/kom.o \
          $(EW_LIB)/sleep_ew.o \
          $(EW_LIB)/time_ew.o \
          $(EW_LIB)/logit_mt.o \
          $(EW_LIB)/sema_ew.o \
          $(EW_LIB)/threads_ew.o \
          $(EW_LIB)/swap.o \
          $(EW_LIB)/mem_circ_queue.o

LDFLAGS = $(EW_OBJS) -lm -lpthread

# Finds all .c files in libsrc and in the current folder
ATWC_SRCS = $(wildcard ../../libsrc/*.c)
MOD_SRCS = $(wildcard *.c)

all: $(MODULE)

$(MODULE): $(MOD_SRCS) $(ATWC_SRCS)
	$(CC) $(CFLAGS) -o $(MODULE) $(MOD_SRCS) $(ATWC_SRCS) $(LDFLAGS)
	cp $(MODULE) $(EW_HOME)/$(EW_VERSION)/bin/
	@echo "======================================================"
	@echo " SUCCESS: $(MODULE) compiled and copied to EW bin!"
	@echo "======================================================"

clean:
	rm -f $(MODULE)
