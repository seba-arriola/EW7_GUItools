#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: make_lib_base.sol,v 1.7 2004/04/14 23:19:33 davidk Exp $
#
#    Revision history:
#     $Log: make_lib_base.sol,v $
#     Revision 1.7  2004/04/14 23:19:33  davidk
#     For solaris build tree, changed C compiler name to $(CC) macro and
#     C++ compiler name to $(CCC) macro.
#
#     Revision 1.6  2004/04/14 23:13:49  davidk
#     Re-streamlined makefiles, to build the EWDB and hydra trees on Solaris.
#
#     Revision 1.5  2004/04/10 16:59:18  davidk
#     Adding support for $(APPS_LIBCPP), a separate library directory for cpp objects.
#
#     Revision 1.4  2004/04/09 22:22:49  davidk
#     Added .cpp suffix definition to compile .cpp (C++) files.
#     Included a $(CLEAN_OTHER) variable that can be used
#     by makefiles to clean additional directories/stuff, when
#     a "make clean" is executed.
#
#     Revision 1.3  2001/05/23 23:40:35  davidk
#     Fixed the install command.  A <tab> was missing.  Stupid makefiles.
#
#     Revision 1.2  2001/05/23 22:50:57  davidk
#     Modified makefiles so that the latest (post Magnitude addition)
#     version of the EWDB_API and apps now build on NT.
#
#
##################################################################

.c.o: 
	$(CC) -c $(CFLAGS) $<
	cp $@ $(APPS_LIB)

.cpp.$(OBJ):
	$(CCC) -c $(cppflags) $<
	cp $@ $(APPS_LIBCPP)

clean: $(CLEAN_OTHER)
	/bin/rm -f $(FULL_APP_NAME) core *.o *.obj *% *~

install:
	cp *.$(OBJ) $(APPS_LIB)


