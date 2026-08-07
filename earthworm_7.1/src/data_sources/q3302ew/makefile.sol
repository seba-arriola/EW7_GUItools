CC = gcc
CC = cc

LIB330_DIR = $(EW_HOME)/$(EW_VERSION)/src/libsrc/lib330

BINDIR = $(EW_HOME)/$(EW_VERSION)/bin
LIBDIR = $(EW_HOME)/$(EW_VERSION)/lib

CFLAGS = $(GLOBALFLAGS) -I$(LIB330_DIR) -I. -g
LDFLAGS = -L$(LIB330_DIR) -l330 -lpthread -lc -lm -lnsl -lsocket -lrt

SRCS = q3302ew.c logging.c config.c options.c lib330Interface.c heart.c

OBJS = $(SRCS:%.c=%.o)

EW_LIBS = $(LIBDIR)/logit_mt.o $(LIBDIR)/kom.o $(LIBDIR)/threads_ew.o \
          $(LIBDIR)/time_ew.o $(LIBDIR)/transport.o $(LIBDIR)/sleep_ew.o \
          $(LIBDIR)/getutil.o $(LIBDIR)/sema_ew.o 
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

all: q3302ew

q3302ew: $(OBJS)
	$(CC) $(GLOBALFLAGS) -o q3302ew $(OBJS) $(EW_LIBS) $(LDFLAGS)
	cp q3302ew $(BINDIR)

clean:
	rm *.o
	rm q3302ew
