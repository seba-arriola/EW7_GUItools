# makefile for globalproc

CFLAGS = -D_REENTRANT -llibc ${GLOBALFLAGS} 

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
OL = $(APPS_DIR)/lib
OLSRC = $(APPS_DIR)/src/libsrc

OBJ = globalproc.o \
      $(L)/geo_to_km.o \
      $(L)/transport.o \
      $(L)/swap.o \
      $(L)/dirops_ew.o \
      $(L)/site.o \
      $(L)/tlay.o \
      $(L)/brent.o \
      $(L)/mnbrak.o \
      $(L)/global_amp_rw.o \
      $(L)/global_pick_rw.o \
      $(L)/global_loc_rw.o \
      $(L)/global_msg.o \
      $(L)/watchdog_client.o \
      $(OL)/init_ewevent.o \
      $(L)/kom.o \
      $(L)/logit_mt.o \
      $(L)/getutil.o \
      $(L)/chron3.o \
      $(L)/threads_ew.o \
      $(L)/sema_ew.o \
      $(L)/time_ew.o \
      $(L)/sleep_ew.o


globalproc: $(OBJ); \
        cc -o $(B)/globalproc $(OBJ) -lposix4 -lthread -lm

$(OL)/init_ewevent.o:
	$(CC) -c -g $(CFLAGS) -o $(OL)/init_ewevent.o $(OLSRC)/init_ewevent.c

lint:
	lint globalproc.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~  $(OL)/init_ewevent.o 

clean_bin:
	rm -f $B/globalproc*
