
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.9 2007/02/20 13:08:31 paulf Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.9  2007/02/20 13:08:31  paulf
#     lockfile added for singleton programs to use
#
#     Revision 1.8  2004/07/16 20:34:29  lombard
#     Replaced service_ew.obj with service_ew.o
#
#     Revision 1.7  2004/07/13 16:51:49  mark
#     Added service_ew
#
#     Revision 1.6  2002/11/03 05:04:41  lombard
#     Removed useless dependencies.
#
#     Revision 1.5  2002/11/03 05:03:28  lombard
#     Removed redundant flags from CFLAGS line
#
#     Revision 1.4  2001/04/06 21:28:03  davidk
#     removed references to libgen(lgen), as these are note relevant to compiling
#     source files into objects.
#
#     Revision 1.3  2001/04/05 18:28:13  cjbryan
#     added libgen; needed for mkdirp of RecursiveCreateDir
#
#     Revision 1.2  2000/08/09 16:00:01  lucky
#     Added lint directive
#
#     Revision 1.1  2000/02/14 18:46:07  lucky
#     Initial revision
#
#
#


CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

SRCS = copyfile.c dirops_ew.c getavail.c getsysname_ew.c pipe.c sema_ew.c\
   sendmail.c sendpage.c sleep_ew.c socket_ew.c threads_ew.c time_ew.c\
   transport.c wait_timer.c errexit.c remote_copy.c service_ew.c lockfile_ew.c
OBJS = copyfile.o dirops_ew.o getavail.o getsysname_ew.o pipe.o sema_ew.o\
   sendmail.o sendpage.o sleep_ew.o socket_ew.o threads_ew.o time_ew.o\
   transport.o wait_timer.o errexit.o remote_copy.o service_ew.o lockfile_ew.o

all: $(OBJS)

.c.o:
	cc -c $(CFLAGS)  $<
	cp $@ $L

lint:
	lint $(SRCS) $(GLOBALFLAGS)

clean:
	/bin/rm -f $(OBJS)

realclean: clean
	sh -c 'for o in $(OBJS) ;\
	do /bin/rm -f $L/$$o;\
	done'
