# Makefile para new_summary (Visor GTK + Earthworm 7.1 + ATWC)

CC = gcc
LIBDIRASD = ..

# Usamos pkg-config para obtener automáticamente los flags de GTK3 y desactivamos chequeos severos de memoria
CFLAGS = -Wall -O2 -g $(shell pkg-config --cflags gtk+-3.0) -I$(LIBDIRASD)/libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE

# Usamos pkg-config para obtener las librerías a enlazar
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lm -lpthread

# Librerías base de Earthworm
EWLIBS = $(EW_HOME)/$(EW_VERSION)/lib/logit.o \
         $(EW_HOME)/$(EW_VERSION)/lib/time_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/transport.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sleep_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/getutil.o \
         $(EW_HOME)/$(EW_VERSION)/lib/kom.o \
         $(EW_HOME)/$(EW_VERSION)/lib/sema_ew.o \
         $(EW_HOME)/$(EW_VERSION)/lib/threads_ew.o

# Archivos fuente del ATWC necesarios para que dummy.c funcione
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
