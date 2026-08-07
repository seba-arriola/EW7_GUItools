CC = gcc
LIBDIRASD = ..
# Usamos pkg-config para obtener automáticamente los flags de GTK3 y desactivamos los controles estrictos de memoria
CFLAGS = -Wall -O2 -g $(shell pkg-config --cflags gtk+-3.0) -I$(LIBDIRASD)/libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE

# Usamos pkg-config para obtener las librerías a enlazar de GTK3
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lm -lpthread



# Librerías base de Earthworm
EWLIBS = $(EW_HOME)/$(EW_VERSION)/lib/logit.o \
         $(EW_HOME)/$(EW_VERSION)/lib/time_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/transport.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sleep_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/getutil.o \
         $(EW_HOME)/$(EW_VERSION)/lib/kom.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sema_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/threads_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/swap.o

# ATWCLIBS: Todas las dependencias que WCATWC necesita para Mwp, Dummy, ReportPick, etc.
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

all: new_develo

new_develo: new_develo.c
	$(CC) $(CFLAGS) -o new_develo new_develo.c $(ATWCLIBS) $(EWLIBS) $(LDFLAGS)

clean:
	rm -f new_develo *.o
