#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2006/01/19 19:04:55 friberg Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2006/01/19 19:04:55  friberg
#     rayloc upgraded to use a library version of rayloc_message_rw.c
#
#     Revision 1.1  2004/08/05 04:15:11  friberg
#     First commit of rayloc_ew in EW-CENTRAL CVS
#
#     Revision 1.8  2004/08/03 18:29:37  ilya
#     *** empty log message ***
#
#     Revision 1.7  2004/08/03 18:26:05  ilya
#     Now we use stock EW functions from v6.2
#
#     Revision 1.6  2004/08/03 17:51:47  ilya
#     Finalizing the project: using EW globals
#
#     Revision 1.5  2004/07/29 21:32:03  ilya
#     New logging; tests; fixes
#
#     Revision 1.4  2004/07/29 17:28:54  ilya
#     Fixed makefile.sol; added makefile.sol_gcc; tested cc compilation
#
#     Revision 1.3  2004/06/25 14:22:15  ilya
#     Working version: no output
#
#     Revision 1.2  2004/06/24 16:32:06  ilya
#     global_msg.h
#
#     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
#     initial import into CVS
#



CFLAGS = ${GLOBALFLAGS} -g -I ${I} -DUSE_LOGIT
FFLAGS = -g
B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
I = $(EW_HOME)/$(EW_VERSION)/include/

TARGET1=rayloc_test1
TARGET2=rayloc_test2
TARGET3=rayloc_test3

F_MAIN_SRC = raylocator.f
F_STUB_SRC = librayloc1.f
C_MAIN1_SRC    = rayloc_test1.c
C_MAIN2_SRC    = rayloc_test2.c

C_SRCS=  rayloc_stations.c  rayloc1.c \
   rayloc_unused_phases.c 

F_SRCS = distaz.f  input.f  libsun.f   \
   ellip.f   io_util.f     libtau.f  robust_util.f \
   hypo.f    output.f

RAYLOC_EW_OBJ = rayloc_ew_main.o rayloc_ew_config.o\
    $L/kom.o $L/getutil.o $L/time_ew.o $L/chron3.o $L/logit.o \
    $L/transport.o $L/sleep_ew.o $L/swap.o $L/global_msg.o $L/global_loc_rw.o \
    $L/rayloc_message_rw.o

C_OBJS    = $(C_SRCS:%.c=%.o)
F_OBJS    = $(F_SRCS:%.f=%.o)
F_STUB    = $(F_STUB_SRC:%.f=%.o)
C_MAIN1   = $(C_MAIN1_SRC:%.c=%.o)
C_MAIN2   = $(C_MAIN2_SRC:%.c=%.o)
F_MAIN    = $(F_MAIN_SRC:%.c=%.o)

PROGS = rayloc_ew #$(TARGET2) $(TARGET3) $(TARGET1) 

########################################################################
all: $(PROGS)
	cp rayloc_ew $B/

rayloc_ew: $(RAYLOC_EW_OBJ) $(F_OBJS) $(C_OBJS)  $(F_STUB)
	${F77}  -o rayloc_ew $(RAYLOC_EW_OBJ) $(F_OBJS) $(C_OBJS)  $(F_STUB) -lm -lposix4

$(TARGET1):         $(F_OBJS) $(F_MAIN)
	$(F77) -o $(TARGET1) $(F_OBJS) $(F_MAIN)

$(TARGET2):         $(F_OBJS) $(C_OBJS) $(C_MAIN1) $(F_STUB)
	$(F77) -o $(TARGET2) $(F_OBJS) $(C_OBJS) $(C_MAIN1) $(F_STUB) $L/kom.o

$(TARGET3):         $(F_OBJS) $(C_OBJS) $(C_MAIN2) $(F_STUB)
	$(F77) -o $(TARGET3) $(F_OBJS) $(C_OBJS) $(C_MAIN2) $(F_STUB) $L/kom.o

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/rayloc_ew*
