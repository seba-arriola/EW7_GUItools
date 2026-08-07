##################################################################


CFLAGS = $(GLOBALFLAGS)
EWLIB=$(EW_HOME)/$(EW_VERSION)/lib
B=$(EW_HOME)/$(EW_VERSION)/bin
LIBS=-lm -lrt
DIRSEP=/

OBJ=o
cc = cc
link = cc

include makefile.ind

$(APP): $(APP_OBJECTS)
	echo "making the main app"
	echo "have $(APP_OBJECTS)"
	$(link) $(APP_OBJECTS) $(ALL_CLIENT_LIBS) $(LIBS) -o $(APP)
	cp $(APP) $(B)

.c.$(OBJ):
	$(cc) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/raypicker*
