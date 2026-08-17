# Gets the folder name (e.g. loc_wcatwc)
MODULE = $(notdir $(CURDIR))

CC = gcc

# Magic flags: Absolute 64-bit tolerance and warning suppression
CFLAGS = -Wall -O2 -g -I../../libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE -Wno-format


EW_LIB = $(EW_HOME)/$(EW_VERSION)/lib

# Base Earthworm libraries
EW_OBJS = $(EW_LIB)/logit.o \
          $(EW_LIB)/time_ew.o \
          $(EW_LIB)/transport.o \
          $(EW_LIB)/sleep_ew.o \
          $(EW_LIB)/getutil.o \
          $(EW_LIB)/kom.o \
          $(EW_LIB)/sema_ew.o \
          $(EW_LIB)/threads_ew.o

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
