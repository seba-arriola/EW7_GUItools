CC = gcc
LIBDIRASD = ..

CFLAGS = -Wall -O2 -g $(shell pkg-config --cflags gtk+-3.0) -I$(LIBDIRASD)/libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE

LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lm -lpthread

EWLIBS = $(EW_HOME)/$(EW_VERSION)/lib/logit.o \
         $(EW_HOME)/$(EW_VERSION)/lib/time_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/transport.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sleep_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/getutil.o \
         $(EW_HOME)/$(EW_VERSION)/lib/kom.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sema_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/threads_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/swap.o

all: ew_controller

ew_controller: ew_controller.c
	$(CC) $(CFLAGS) -o ew_controller ew_controller.c $(EWLIBS) $(LDFLAGS)

clean:
	rm -f ew_controller *.o