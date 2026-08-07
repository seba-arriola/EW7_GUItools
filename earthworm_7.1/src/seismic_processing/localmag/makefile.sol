#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.11 2007/03/29 20:09:50 paulf Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.11  2007/03/29 20:09:50  paulf
#     added eventXML option from INGV. This option allows writing the Shakemap style event information out as XML in the SAC out dir
#
#     Revision 1.10  2006/03/10 13:03:28  paulf
#     upgraded to SCNL version 2.1.0, tested at Utah
#
#     Revision 1.9  2003/08/05 19:55:07  lucky
#     *** empty log message ***
#
#     Revision 1.8  2002/09/27 17:32:27  dhanych
#     *** empty log message ***
#
#     Revision 1.7  2002/05/13 16:49:19  dhanych
#     20020513 dbh -- added glevt_2_ewevent, init_ew_event for global messages
#
#     Revision 1.6  2001/05/02 00:12:57  bogaert
#     fixed clean_bin
#
#     Revision 1.5  2001/04/11 21:07:08  lombard
#     "site.?" renamed to "lm_site.?" for clarity.
#
#     Revision 1.4  2001/03/01 05:25:44  lombard
#     changed FFT package to fft99; fixed bugs in handling of SCNPars;
#     changed output to Magnitude message using rw_mag.c
#
#     Revision 1.3  2001/01/15 03:55:55  lombard
#     bug fixes, change of main loop, addition of stacker thread;
#     moved fft_prep, transfer and sing to libsrc/util.
#
#     Revision 1.2  2000/12/31 17:27:25  lombard
#      More bug fixes and cleanup.
#
#     Revision 1.1  2000/12/19 18:31:25  lombard
#     Initial revision
#
#
#
#
# Makefile for localmag  -- Solaris version
LDC = cc

# To turn on UW-specific features, uncomment the following four lines.
#UW_FLAGS = -Xc -DUW -I$(SNAPSHOME)/Src/Utils
#UW_OBJ = lm_uw.o uwresp.o
#UW_LIBS = -L$(SNAPSHOME)/Src/Utils -lutils
#LDC = f77

FFLAGS = -g
CFLAGS = ${GLOBALFLAGS} ${UW_FLAGS}
C_OPTS = -g

#B = ../../../bin
#L = ../../../lib
B = ${EW_HOME}/${EW_VERSION}/bin
L = ${EW_HOME}/${EW_VERSION}/lib

all: localmag

LIBS = -lm -lnsl -lsocket -lposix4 -lthread

OBJS = lm_main.o lm_util.o lm_config.o lm_ws.o lm_misc.o lm_sac.o \
   lm_site.o lm_xml_event.o $(UW_OBJ)

EWLIBS = $L/swap.o $L/logit_mt.o $L/read_arc.o $L/time_ew.o $L/chron3.o \
   $L/ws_clientII.o $L/socket_ew_common.o $L/socket_ew.o $L/kom.o \
   $L/sleep_ew.o $L/tlay.o $L/mnbrak.o $L/brent.o $L/dirops_ew.o \
   $L/transport.o $L/getutil.o $L/mem_circ_queue.o $L/sema_ew.o \
   $L/threads_ew.o $L/fft99.o $L/fft_prep.o $L/transfer.o \
   $L/rw_mag.o 

localmag: $(OBJS)
	$(LDC) $(FFLAGS) -o $B/localmag $(OBJS) $(EWLIBS) $(UW_LIBS) $(LIBS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/localmag

uwresp.o: uwresp.f
	f77 -c $(FFLAGS) uwresp.f

.c.o:
	cc -c ${CFLAGS} $(C_OPTS) $<

