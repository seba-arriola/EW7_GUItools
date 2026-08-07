#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: make_lib_defs.sol,v 1.3 2004/04/14 23:13:54 davidk Exp $
#
#    Revision history:
#     $Log: make_lib_defs.sol,v $
#     Revision 1.3  2004/04/14 23:13:54  davidk
#     Re-streamlined makefiles, to build the EWDB and hydra trees on Solaris.
#
#     Revision 1.2  2002/11/03 21:28:04  lombard
#     Added CFLAGS definition
#
#     Revision 1.1  2001/05/23 22:51:11  davidk
#     Initial revision
#
#
##################################################################


######################################
# Define the macro definitions that
# we need for all of the schema
# makefiles.
######################################
COPY = cp
DELETE_FORCE = rm -f
EMAKE = make -f makefile.sol
EMAKE_FORCE = make -f makefile.sol
EWDB_LIB= $(SCHEMA_DIR)/lib/
CFLAGS  = ${GLOBALFLAGS}
EW_LIB  = $(EW_HOME)/$(EW_VERSION)/lib/



###############################
#OBJECT FILES EXTENSION
###############################
OBJ   = o
EXE     =

