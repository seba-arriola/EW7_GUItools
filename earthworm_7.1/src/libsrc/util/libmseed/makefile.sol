# Standard compiler parameters
CFLAGS = -c -D_REENTRANT $(GLOBALFLAGS)

LIBMSEED = $(EW_HOME)/$(EW_VERSION)/src/libsrc/util/libmseed
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

LDFLAGS =
LDLIBS =

LIB_OBJS = fileutils.o genutils.o gswap.o lmplatform.o lookup.o \
           msrutils.o pack.o packdata.o traceutils.o unpack.o \
           unpackdata.o

libmseed.a: $(LIB_OBJS)
	rm -f $@
	ar -csq $@ $(LIB_OBJS)
	cp $(LIBMSEED)/$@ $L

.c.o:
	cc -c $(CFLAGS) $<

clean:
	rm -f *.o *.obj core *.a $(L)/libmseed.a

