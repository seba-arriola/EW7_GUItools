# Obtiene el nombre de la carpeta (ej. loc_wcatwc)
MODULE = $(notdir $(CURDIR))

CC = gcc
# Banderas de compilacion (sin GTK)
CFLAGS = -Wall -O2 -I../../libsrc -I$(EW_HOME)/$(EW_VERSION)/include -fcommon -D_LINUX -D_REENTRANT -D_FORTIFY_SOURCE=0 -fno-stack-protector -U_FORTIFY_SOURCE  

EW_LIB = $(EW_HOME)/$(EW_VERSION)/lib
# Librerias base de Earthworm necesarias para ATWC
# Librerias base de Earthworm necesarias para ATWC
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

# Busca todos los .c en libsrc y en la carpeta actual
ATWC_SRCS = $(wildcard ../../libsrc/*.c)
MOD_SRCS = $(wildcard *.c)

all: $(MODULE)

$(MODULE): $(MOD_SRCS) $(ATWC_SRCS)
	$(CC) $(CFLAGS) -o $(MODULE) $(MOD_SRCS) $(ATWC_SRCS) $(LDFLAGS)
	cp $(MODULE) $(EW_HOME)/$(EW_VERSION)/bin/
	@echo "======================================================"
	@echo " EXITO: $(MODULE) compilado y copiado a EW bin!"
	@echo "======================================================"

clean:
	rm -f $(MODULE)
