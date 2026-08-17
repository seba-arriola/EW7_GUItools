# Makefile for new_hypo_display (GTK Viewer + Earthworm 7.1 + ATWC)

CC = gcc
LIBDIRASD = ..

# We use pkg-config to automatically get the GTK3 flags and disable strict memory checks
CFLAGS = -Wall -O2 -g $(shell pkg-config --cflags gtk+-3.0) -I$(LIBDIRASD)/libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE

# We use pkg-config to get the GTK3 libraries to link against
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

# ATWCLIBS: All the dependencies WCATWC needs for Mwp, Dummy, ReportPick, Waves, etc.
ATWCLIBS = $(LIBDIRASD)/libsrc/get_pick.c \
           $(LIBDIRASD)/libsrc/report.c \
           $(LIBDIRASD)/libsrc/mags.c \
           $(LIBDIRASD)/libsrc/mags_mwp.c \
           $(LIBDIRASD)/libsrc/dataprocessing.c \
           $(LIBDIRASD)/libsrc/mjstime.c \
           $(LIBDIRASD)/libsrc/geotools.c \
           $(LIBDIRASD)/libsrc/complex.c \
           $(LIBDIRASD)/libsrc/taulib.c \
           $(LIBDIRASD)/libsrc/dummy.c \
           $(LIBDIRASD)/libsrc/filters.c \
           $(LIBDIRASD)/libsrc/get_hypo.c \
           $(LIBDIRASD)/libsrc/diskrw.c \
           $(LIBDIRASD)/libsrc/ReadStationData.c \
           $(LIBDIRASD)/libsrc/locquake.c \
           $(LIBDIRASD)/libsrc/fereg.c \
           $(LIBDIRASD)/libsrc/littoral.c

all: new_hypo_display

new_hypo_display: new_hypo_display.c
	$(CC) $(CFLAGS) -o new_hypo_display new_hypo_display.c $(ATWCLIBS) $(EWLIBS) $(LDFLAGS)

clean:
	rm -f new_hypo_display *.o
