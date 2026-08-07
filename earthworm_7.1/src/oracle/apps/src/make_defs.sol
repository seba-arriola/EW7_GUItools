#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: make_defs.sol,v 1.5 2004/04/10 16:59:18 davidk Exp $
#
#    Revision history:
#     $Log: make_defs.sol,v $
#     Revision 1.5  2004/04/10 16:59:18  davidk
#     Adding support for $(APPS_LIBCPP), a separate library directory for cpp objects.
#
#     Revision 1.4  2004/03/31 17:29:45  davidk
#     Added .SUFFIXES definition.
#
#     Revision 1.3  2001/07/20 17:18:32  davidk
#     Added definition of APP_SUB_DIR.
#
#     Revision 1.2  2001/05/23 22:50:56  davidk
#     Modified makefiles so that the latest (post Magnitude addition)
#     version of the EWDB_API and apps now build on NT.
#
#
##################################################################

CFLAGS 	= -g -c -D_REENTRANT $(GLOBALFLAGS) -I$(APPS_DIR)/include


###############################
# EWDB and Earthworm LIB 
# and BIN dirs
###############################

WEB_BIN = $(WEB_DIR)/bin/
EW_LIB  = $(EW_HOME)/$(EW_VERSION)/lib/
EWDB_LIB= $(SCHEMA_DIR)/lib/
APPS_LIB  = $(APPS_DIR)/lib/
APPS_LIBCPP  = $(APPS_DIR)/lib/
ALARMS_LIB= $(APPS_DIR)/src/alarms/lib/
REVIEW_LIB= $(APPS_DIR)/src/review/web/lib/
EW_BIN  = $(EW_HOME)/$(EW_VERSION)/bin/
APP_SUB_DIR = $(APP)


###############################
#OBJECT FILES EXTENSION
###############################
OBJ 	= o

#The current make program expect C++ source
#files to be .cc.  Add .cpp SUFFIX definition
.SUFFIXES: .cpp

