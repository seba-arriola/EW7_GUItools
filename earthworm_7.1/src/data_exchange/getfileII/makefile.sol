#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#                    Make file for getfileII
#
CFLAGS = -g

O = getfileII.o config.o socket_sol.o chdir_sol.o rename_sol.o \
    log.o tzset_sol.o

all: getfileII

getfileII: $O
	cc -o getfileII $O -lm -lsocket -lnsl -lposix4 -lc

clean:
	-/bin/rm -f getfileII *.o

clean_bin:
	-/bin/rm -f getfileII 
