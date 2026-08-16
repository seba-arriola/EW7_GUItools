# Obtiene el nombre de la carpeta (ej. loc_wcatwc)
MODULE = $(notdir $(CURDIR))

CC = gcc

# Banderas mágicas: Tolerancia absoluta a 64 bits y supresión de advertencias
CFLAGS = -Wall -O2 -g -I../../libsrc -I$(EW_HOME)/$(EW_VERSION)/include -D_LINUX -D_REENTRANT -fno-stack-protector -U_FORTIFY_SOURCE -Wno-format


EW_LIB = $(EW_HOME)/$(EW_VERSION)/lib

# Librerias base de Earthworm
EW_OBJS = $(EW_LIB)/logit.o \
          $(EW_LIB)/time_ew.o \
          $(EW_LIB)/transport.o \
          $(EW_LIB)/sleep_ew.o \
          $(EW_LIB)/getutil.o \
          $(EW_LIB)/kom.o \
          $(EW_LIB)/sema_ew.o \
          $(EW_LIB)/threads_ew.o

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
