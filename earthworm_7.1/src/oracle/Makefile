#
#       Top level makefile for Earthworm
#             Lucky Vidmar Mon Feb 14 21:48:28 GMT 2000
#
#
# This makefile contains all the directives necessary to compile
# earthworm under Solaris and NT, as well as clean up the Earthworm
# source and binary file tree. 
#
# 1. To build (compile) the entire Earthworm distribution on a Sun:
#        make solaris
#
# 2. To build (compile) the entire Earthworm distribution on an NT PC:
#        nmake nt
#
# 3. To clean up the source tree, i.e. remove all *.o, *.obj, *~, *%,
#    and core files from all modules, on a Sun:
#        make clean_solaris
#
# 4. To clean up the source tree, i.e. remove all *.o, *.obj, *~, *%,
#    and core files from all modules, on a PC running NT:
#        nmake clean_nt
#
# 5. To clean up the bin directory, i.e. remove all Earthworm programs,
#    on a Sun:
#        make clean_bin_solaris
#
# 6. To clean up the bin directory, i.e. remove all Earthworm programs,
#    on a PC running NT:
#        nmake clean_bin_nt
#
#

#
# Starting with Earthworm version 5, the source directory is 
# divided into subfolders. Whenever a new module is added to Earthworm, 
# it should be added to the Makefile in the appropriate subfolder. 
#

#
# If a new subfolder is created, it should be added to the 
#  list of SUBFOLDERs
#
SUBFOLDERS = \
	schema-working \
	apps 


#
#  Top level directives
#
solaris: sol_modules
nt: nt_modules

#
#  Solaris directives
#
sol_modules:: FRC
	@for x in $(SUBFOLDERS); \
	do \
		(echo ------; cd $$x/src; echo Making $@ in:; pwd; \
		make solaris); \
	done

clean_solaris:
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x/src; echo Cleaning in:; pwd; \
		make clean_solaris); \
	done

clean_bin_solaris:
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x/src; echo Cleaning binaries in:; pwd; \
		make clean_bin_solaris); \
	done



#
#  NT directives
#
nt_modules:: FRC
	@echo "Making Earthworm modules"
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder2 %x\src nt

clean_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder2 %x\src clean_nt

clean_bin_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder2 %x\src clean_bin_nt

FRC:
