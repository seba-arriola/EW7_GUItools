# Makefile for new_summary (GTK Viewer + Earthworm 7.1 + ATWC)

CC = gcc
LIBDIRASD = ..

# We use pkg-config to automatically get the GTK3 flags and disable severe memory checks
CFLAGS = -Wall -O2 -g $(shell pkg-config --cflags gtk+-3.0) -I$(LIBDIRASD)/libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE

# We use pkg-config to get the libraries to link against
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lm -lpthread

# Base Earthworm libraries
EWLIBS = $(EW_HOME)/$(EW_VERSION)/lib/logit.o \
         $(EW_HOME)/$(EW_VERSION)/lib/time_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/transport.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sleep_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/getutil.o \
         $(EW_HOME)/$(EW_VERSION)/lib/kom.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sema_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/threads_ew.o

# ATWC source files needed for dummy.c to work
ATWCLIBS = $(LIBDIRASD)/libsrc/dummy.c \
           $(LIBDIRASD)/libsrc/get_hypo.c \
           $(LIBDIRASD)/libsrc/geotools.c \
           $(LIBDIRASD)/libsrc/mjstime.c \
           $(LIBDIRASD)/libsrc/complex.c \
           $(LIBDIRASD)/libsrc/taulib.c

all: new_summary

new_summary: new_summary.c
	$(CC) $(CFLAGS) -o new_summary new_summary.c $(ATWCLIBS) $(EWLIBS) $(LDFLAGS)

clean:
	rm -f new_summary *.o
