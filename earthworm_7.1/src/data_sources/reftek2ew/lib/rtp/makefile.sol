
CC=cc 
CFLAGS=-O -I../.. -DSOLARIS
LDIR = ..

OBJS = accept.o attr.o close.o cmdpkt.o log.o misc.o open.o pid.o recv.o \
       send.o server.o soh.o stat.o version.o

all: librtp.a

librtp.a: $(OBJS)
	ar -r ../librtp.a $(OBJS)

clean:
	rm -f *.o *.obj core a.out *~ *%  ../librtp.a
	(cd $(LDIR); rm -f $(OBJS))
