#                    Nmake File For ws2matlab - Sun Solaris version
#

CFLAGS = -D_REENTRANT ${GLOBALFLAGS}

APP1 = mGetMenu
APP2 = mGetAscii
O1 = $(APP1).o
O2 = $(APP2).o

BIN = $(EW_HOME)/$(EW_VERSION)/bin
LIB = $(EW_HOME)/$(EW_VERSION)/lib
INC = $(EW_HOME)/$(EW_VERSION)/include

MATLAB = /usr/local/matlab
MATLABLIB = $(MATLAB)/extern/lib/sol2
#MATLINK = -leng -lmx
MATLINK = 
MATINC = $(MATLAB)/extern/include

LIBSEW = $(LIB)/ws_clientII.o \
	$(LIB)/logit_mt.o \
	$(LIB)/putaway.o \
	$(LIB)/ahputaway.o \
	$(LIB)/sacputaway.o \
	$(LIB)/geo_to_km.o \
	$(LIB)/sudsputaway.o \
	$(LIB)/tankputaway.o \
	$(LIB)/gseputaway.o \
	$(LIB)/seiputaway.o \
	$(LIB)/seiutils.o \
	$(LIB)/chron3.o \
	$(LIB)/dirops_ew.o \
	$(LIB)/kom.o \
	$(LIB)/sleep_ew.o \
	$(LIB)/time_ew.o \
	$(LIB)/socket_ew.o \
	$(LIB)/socket_ew_common.o \
	$(LIB)/sema_ew.o \
	$(LIB)/swap.o

ALL: $(APP1).mexsol $(APP2).mexsol

#$(APP1).mexsol: $(O1)
#	ld -G -L$(MATLABLIB) $(MATLINK) -o $(APP1).mexsol $(LIBSEW) $(O1)

$(APP1).mexsol: $(O1)
	mex -I$(MATINC) -I$(INC) -L$(MATLABLIB) $(MATLINK) $(APP1).c $(LIBSEW)

$(O1): $(APP1).c
	cc -c $(APP1).c -o $(O1) -I$(MATINC) -I$(INC)

#$(APP2).mexsol: $(O2)
#	ld -G -L$(MATLABLIB) $(MATLINK) -o $(APP2).mexsol $(LIBSEW) $(02)

$(APP2).mexsol: $(O2)
	mex -I$(MATINC) -I$(INC) -L$(MATLABLIB) $(MATLINK) $(APP2).c $(LIBSEW)

$(O2): $(APP2).c
	cc -c $(APP2).c -o $(O2) -I$(MATINC) -I$(INC)

lint:
	lint mGetMenu.c mGetAscii.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.mexsol *.o *.obj *% *~

clean_bin:
	rm -f $(BIN)\mGetMenu* $(BIN)\mGetAscii*

