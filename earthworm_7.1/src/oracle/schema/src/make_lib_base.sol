#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: make_lib_base.sol,v 1.3 2004/04/14 23:19:33 davidk Exp $
#
#    Revision history:
#     $Log: make_lib_base.sol,v $
#     Revision 1.3  2004/04/14 23:19:33  davidk
#     For solaris build tree, changed C compiler name to $(CC) macro and
#     C++ compiler name to $(CCC) macro.
#
#     Revision 1.2  2004/04/14 23:13:54  davidk
#     Re-streamlined makefiles, to build the EWDB and hydra trees on Solaris.
#
#     Revision 1.1  2001/05/23 22:51:11  davidk
#     Initial revision
#
#
##################################################################



######################################
# Define common behaviors needed for
# all schema makefiles.
######################################
.c.$(OBJ):
	$(CC) -c $(CFLAGS) $(INCLUDE_FLAGS) $<


clean:
	$(DELETE_FORCE) *.$(OBJ)

sql_scripts: sql_scripts/*.sql
	@(cd sql_scripts; echo Making in:; pwd; make -f makefile.sol)

