
CC=cc 
CFLAGS=-O -I../.. -DSOLARIS
LDIR = ..

OBJS = ad.o cd.o com.o dcomp.o ds.o dt.o eh.o et.o om.o sc.o sh.o  \
       steim.o reftek_string.o testlib.o type.o

all: libreftek.a

libreftek.a: $(OBJS)
	ar -r ../libreftek.a $(OBJS)


clean:
	rm -f *.o *.obj core a.out *~ *% ../libreftek.a
	(cd $(LDIR); rm -f $(OBJS))
