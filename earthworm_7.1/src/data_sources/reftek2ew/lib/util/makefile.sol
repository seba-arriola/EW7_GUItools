
CC=cc 
CFLAGS=-O -I../.. -DSOLARIS
LDIR = ..


OBJS = bcd2long.o dump.o find.o getline.o mstime.o parse.o string.o \
       swap.o timefunc.o timer.o


all: libutil.a

libutil.a: $(OBJS)
	ar -r ../libutil.a $(OBJS)

clean:
	rm -f *.o *.obj core a.out *~ *% ../libutil.a
	(cd $(LDIR); rm -f $(OBJS))
