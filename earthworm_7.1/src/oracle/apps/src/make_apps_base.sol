#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: make_apps_base.sol,v 1.6 2004/04/14 23:19:33 davidk Exp $
#
#    Revision history:
#     $Log: make_apps_base.sol,v $
#     Revision 1.6  2004/04/14 23:19:33  davidk
#     For solaris build tree, changed C compiler name to $(CC) macro and
#     C++ compiler name to $(CCC) macro.
#
#     Revision 1.5  2004/04/10 16:59:18  davidk
#     Adding support for $(APPS_LIBCPP), a separate library directory for cpp objects.
#
#     Revision 1.4  2004/03/17 20:44:22  davidk
#     old change:  removed inclusion of APPS/src/make_ora_api.nt file.
#
#     Revision 1.3  2001/07/20 17:18:32  davidk
#     Added default make command to copy comments as well as build the app.
#
#     Revision 1.2  2001/05/23 22:50:56  davidk
#     Modified makefiles so that the latest (post Magnitude addition)
#     version of the EWDB_API and apps now build on NT.
#
#
##################################################################


APPS_AND_DOC: $(APP)
	-mkdir -p $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	-cp -f $(APP).d $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)/$(APP).d.txt
	-cp -f programmer_notes.*   $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	-cp -f details.html         $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	-cp -f bugs.txt             $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)


###############################
#This is the include statement
# that links this file to the
# hideous set of makefiles that
# contain all of the DB linking
# logic
###############################
include $(APPS_DIR)/src/make_ora_api.sol

#lint doesn't work right now
lint:
	lint $(GLOBALFLAGS)

# cleanup directives
clean:
	/bin/rm -f a.out core *.o *.obj

clean_bin:
	/bin/rm -f $(FULL_APP_NAME)

install:
	

.c.$(OBJ):
   $(CC) $(cflags) $<

.cpp.$(OBJ):
   $(CCC) $(cppflags) $<


