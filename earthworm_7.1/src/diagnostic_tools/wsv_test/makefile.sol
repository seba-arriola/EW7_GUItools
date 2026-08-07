
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

wsv_test: wsv_test.o $L/getutil.o $L/kom.o $L/ws_clientII.o $L/logit.o \
         $L/sleep_ew.o $L/time_ew.o $L/transport.o $L/socket_ew.o $L/socket_ew_common.o 
	cc -o $B/wsv_test wsv_test.o $L/getutil.o $L/kom.o $L/ws_clientII.o \
	      $L/logit.o $L/sleep_ew.o $L/time_ew.o $L/transport.o $L/socket_ew.o \
              $L/socket_ew_common.o -lm -lsocket -lnsl -lposix4

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/wsv_test*
