#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: make_ora_api.sol,v 1.4 2002/08/26 16:20:27 davidk Exp $
#
#    Revision history:
#     $Log: make_ora_api.sol,v $
#     Revision 1.4  2002/08/26 16:20:27  davidk
#     Updated compilation tree for better compilation on NT.
#
#     Revision 1.3  2001/05/23 22:55:35  davidk
#     made oracle tree compile under NT.  Made some globabl changes that apply to solaris.
#
#
##################################################################

###############################
#EW_ORACLE_VERSION not currently set
###############################
EW_ORACLE_VERSION = 


include $(APPS_DIR)/src/make_defs.sol
include $(APPS_DIR)/src/make_ora_api.ind
include $(ORACLE_HOME)/rdbms/lib/env_rdbms.mk

ALL_ORA_API_LIBS = -L$(SCHEMA_DIR)/lib -lewdb 
RDBMSLIB=$(ORACLE_HOME)/rdbms/lib/

LDFLAGS=-L$(LIBHOME) -L$(ORACLE_HOME)/rdbms/lib -lposix4 -lthread
LLIBPSO=`cat $(ORACLE_HOME)/rdbms/lib/psoliblist`


# directory that contain oratypes.h and other oci demo program header files
INCLUDE= -I$(ORACLE_HOME)/rdbms/demo -I$(ORACLE_HOME)/rdbms/public -I$(ORACLE_HOME)/plsql/public -I$(ORACLE_HOME)/network/public

#
CONFIG = $(ORACLE_HOME)/rdbms/lib/config.o

# module to be used for linking with non-deferred option

# flag for linking with non-deferred option (default is deferred mode)
NONDEFER=false

# libraries for linking oci programs
OCISHAREDLIBS=$(TTLIBS) $(LLIBTHREAD)
OCISTATICLIBS=$(STATICTTLIBS) $(LLIBTHREAD)

PSOLIBLIST=$(ORACLE_HOME)/rdbms/lib/psoliblist
CLEANPSO=rm -f $(PSOLIBLIST); $(GENPSOLIB)

DOLIB=$(ORACLE_HOME)/lib/liborcaccel.a
DUMSDOLIB=$(ORACLE_HOME)/lib/liborcaccel_stub.a
REALSDOLIB=/usr/lpp/orcaccel/liborcaccel.a


.SUFFIXES: .o 

.c.o:
	$(CC) -c $(GLOBALFLAGS) $<
#	$(CC) -c $(GLOBALFLAGS) $(CFLAGS) $(INCLUDE_FLAGS) $<

#build: $(LIBCLNTSH) $(OBJS)
$(APP): $(ALL_APP_OBJECTS)
	$(CC) $(LDFLAGS) -o $(FULL_APP_NAME) $(ALL_APP_OBJECTS) $(ALL_LIB_OBJECTS) $(OCISHAREDLIBS) $(APP_OTHER)

