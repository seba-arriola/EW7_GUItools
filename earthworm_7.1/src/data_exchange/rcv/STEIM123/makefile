
# for SPARC SUN-OS 4.1.3 ANSI C:
CC= cc
CFLAGS= -g -dalign 
INCLUDE= .
LFLAGS= -g 
LDLIBS= 

all:		steim123 srdseed swrseed 

clean:		
		rm -f *.o steim123 srdseed swrseed 


steim123:	steim123.o steimlib.o sprofile.o
		$(CC) $(LFLAGS) -o steim123 \
		steim123.o steimlib.o sprofile.o $(LDLIBS)


srdseed:	srdseed.o steimlib.o 
		$(CC) $(LFLAGS) -o srdseed \
		srdseed.o steimlib.o $(LDLIBS)


swrseed:	swrseed.o steimlib.o 
		$(CC) $(LFLAGS) -o swrseed \
		swrseed.o steimlib.o $(LDLIBS)


steim123.o:	$(INCLUDE)/steim.h $(INCLUDE)/sprofile.h steim123.c
		$(CC) -c $(CFLAGS) -I$(INCLUDE) steim123.c

steimlib.o:	$(INCLUDE)/steim.h steimlib.c
		$(CC) -c $(CFLAGS) -I$(INCLUDE) steimlib.c

sprofile.o:	$(INCLUDE)/steim.h $(INCLUDE)/sprofile.h sprofile.c
		$(CC) -c $(CFLAGS) -I$(INCLUDE) sprofile.c


srdseed.o:	$(INCLUDE)/steim.h srdseed.c
		$(CC) -c $(CFLAGS) -I$(INCLUDE) srdseed.c

swrseed.o:	$(INCLUDE)/steim.h swrseed.c
		$(CC) -c $(CFLAGS) -I$(INCLUDE) swrseed.c


