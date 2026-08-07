# makefile for eqfilterII

CFLAGS = -D_REENTRANT -llibc ${GLOBALFLAGS} 

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

OBJ = eqfilterII.o \
      area.o \
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
      $(L)/global_msg.o \
      $(L)/global_loc_rw.o \
      $(L)/read_arc.o \
      $L/rayloc_message_rw.o

eqfilterII: $(OBJ); \
        cc -o $(B)/eqfilterII $(OBJ) -lposix4 -lthread -lm


lint:
	lint eqfilterII.c area.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/eqfilterII*
