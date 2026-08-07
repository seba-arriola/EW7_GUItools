# makefile for geqproc
#

CFLAGS =  -D_REENTRANT ${GLOBALFLAGS} 

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
OL = $(APPS_DIR)/lib
OLSRC = $(APPS_DIR)/src/libsrc
INC = $(EW_HOME)/$(EW_VERSION)/include


OBJ = geqproc.o \
      $(L)/logit_mt.o \
      $(L)/chron3.o \
      $(L)/getutil.o \
      $(L)/kom.o \
      $(L)/sleep_ew.o \
      $(L)/threads_ew.o \
      $(L)/time_ew.o \
      $(L)/transport.o \
      $(L)/mem_circ_queue.o \
      $(L)/sema_ew.o \
      $(L)/pipe.o \
      $(L)/glevt_2_ewevent.o \
      $(OL)/arc_2_ewevent.o \
      $(OL)/init_ewevent.o \
      $(L)/global_msg.o \
      $(L)/global_loc_rw.o \
      $(L)/read_arc.o \

geqproc: $(OBJ); \
	cc $(CFLAGS) -o $(B)/geqproc $(OBJ) -lposix4 -lthread -lm

$(OL)/arc_2_ewevent.o:
	$(CC) -c -g $(CFLAGS) -o $(OL)/arc_2_ewevent.o $(OLSRC)/arc_2_ewevent.c
$(OL)/init_ewevent.o:
	$(CC) -c -g $(CFLAGS) -o $(OL)/init_ewevent.o $(OLSRC)/init_ewevent.c


lint:
	lint geqproc.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~ $(OL)/init_ewevent.o $(OL)/arc_2_ewevent.o

clean_bin:
	rm -f $B/geqproc*
