#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#                     Make file for makehbfile
#                         Solaris Version
#
CFLAGS = -g

O = makehbfile.o config.o chdir_sol.o mkdir_sol.o rename_sol.o

all: makehbfile

makehbfile: $O
	cc -o makehbfile $O -lm -lsocket -lnsl -lposix4 -lc

clean:
	/bin/rm -f makehbfile *.o

clean_bin:
	/bin/rm -f makehbfile
