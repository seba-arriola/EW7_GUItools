#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.7 2003/05/29 13:49:37 friberg Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.7  2003/05/29 13:49:37  friberg
#     added in k2info.o
#
#     Revision 1.6  2001/08/08 16:11:48  lucky
#     version 2.23
#
#     Revision 1.5  2000/08/08 17:47:16  lucky
#     Added lint directive
#
#     Revision 1.4  2000/07/28 22:36:10  lombard
#     Moved heartbeats to separate thread; added DontQuick command; removed
#     redo_com() since it doesn't do any good; other minor bug fixes
#
#     Revision 1.3  2000/06/09 23:17:27  lombard
#      bug fix
#
#     Revision 1.2  2000/05/12 19:08:43  lombard
#     Added restart mechanism
#
#     Revision 1.1  2000/05/04 23:48:35  lombard
#     Initial revision
#
#
#

CFLAGS = -g ${GLOBALFLAGS}

B = ${EW_HOME}/${EW_VERSION}/bin
# for testing just put it locally!
#B=.
L = ${EW_HOME}/${EW_VERSION}/lib

all: k2ew_tcp k2ew_tty

LIBS = -lm -lthread -lposix4 -lsocket
TCP_LIBS = -lnsl

OBJS = k2ewmain.o \
       outptthrd.o \
       terminat.o \
       getconfig.o \
       k2ewerrs.o \
       k2crctbl.o \
       k2cirbuf.o \
       k2pktman.o \
       k2misc.o \
       k2pktio.o \
       k2ewrstrt.o \
       heartbt.o \
       k2info.o \
       error_ew_un.o

TCP_OBJS = k2c_tcp.o

TTY_OBJS = k2c_ser_un.o

EWLIBS = $L/logit_mt.o \
       $L/kom.o \
       $L/getutil.o \
       $L/sleep_ew.o \
       $L/time_ew.o \
       $L/transport.o \
       $L/sema_ew.o \
       $L/threads_ew.o

EW_SOCK_LIBS = $L/socket_ew_common.o \
       $L/socket_ew.o 


k2ew_tcp: $(OBJS) $(TCP_OBJS)
	cc -mt $(CFLAGS) -o $B/k2ew_tcp $(OBJS) $(TCP_OBJS) $(EWLIBS) \
     $(EW_SOCK_LIBS) $(LIBS) $(TCP_LIBS)


k2ew_tty: $(OBJS) $(TTY_OBJS)
	cc -mt $(CFLAGS) -o $B/k2ew_tty $(OBJS) $(TTY_OBJS) $(EWLIBS) $(LIBS)


lint:
	lint k2ewmain.c outptthrd.c terminat.c getconfig.c \
			k2ewerrs.c  k2crctbl.c k2cirbuf.c k2pktman.c \
			k2misc.c k2pktio.c k2ewrstrt.c heartbt.c error_ew_un.c \
			k2c_tcp.c k2c_ser_un.c  $(GLOBALFLAGS)


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/k2ew*


.c.o:
	cc -c ${CFLAGS} $<

