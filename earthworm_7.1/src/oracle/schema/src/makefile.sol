#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.5 2004/04/14 23:13:54 davidk Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.5  2004/04/14 23:13:54  davidk
#     Re-streamlined makefiles, to build the EWDB and hydra trees on Solaris.
#
#     Revision 1.4  2002/09/03 22:49:55  davidk
#     Changed the "ar" linking options to replace objects in the
#     archive file instead of just appending them to the end.
#     Whoever set it up for APPEND must have had a brain fart.
#
#     Revision 1.3  2002/08/26 16:20:28  davidk
#     Updated compilation tree for better compilation on NT.
#
#
##################################################################/

include $(SCHEMA_DIR)/src/make_api_lib.ind
include $(SCHEMA_DIR)/src/make_lib_defs.sol

OBJ=o

$(SCHEMA_DIR)/lib/libewdb.a: $(ALL_ORA_API_LIBS)
	ar cr $(SCHEMA_DIR)/lib/libewdb.a $(ALL_ORA_API_LIBS)



# this file used to be used to build oracle executables under
# solaris, but was replaced by apps/src/make_ora_api.sol
# it now functions to build a static library out of the oracle
# api objects and support libraries
# DK 082402
