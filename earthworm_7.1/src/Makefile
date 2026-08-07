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
# 1a. To compile the Earthworm distrib. on a Linux or other POSIX box:
#        make unix
#
# 2. To build (compile) the entire Earthworm distribution on an NT PC:
#        nmake nt
#
# 3. To clean up the source tree, i.e. remove all *.o, *.obj, *~, *%,
#    and core files from all modules, on a Sun:
#        make clean_solaris
#
# 3a. To clean up the source tree on a POSIX computer:
#	make clean_unix
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
	archiving \
	data_exchange \
	diagnostic_tools \
	display \
	grab_bag \
	reporting \
	seismic_processing \
	system_control \
	data_sources 


#
#  Top level directives
#
solaris: sol_libs sol_modules
unix: unix_libs unix_modules
nt: nt_libs nt_modules


#
#  Solaris directives
#
sol_libs:: FRC
	echo "Making libraries";
	(cd libsrc; make -f makefile.sol);

sol_modules:: FRC
	@for x in $(SUBFOLDERS); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make solaris); \
	done

clean_solaris:
	(cd ../lib; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	(cd libsrc/util; echo Cleaning in:; pwd; \
		make -f makefile.sol clean);
	(cd libsrc/solaris; echo Cleaning in:; pwd; \
		make -f makefile.sol clean);
	(cd libsrc/util/gd_lib; echo Cleaning in:; pwd; \
		make -f makefile.sol clean);
	(cd libsrc/winnt; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make clean_solaris); \
	done

clean_bin_solaris:
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make clean_bin_solaris); \
	done

#
#  Unix directives
#
unix_libs:: FRC
	echo "Making OS-independent libraries";
	(cd libsrc/util; make -f makefile.ux);
	echo "Making unix specific libraries";
	(cd libsrc/unix; make -f makefile.ux)
	echo "Making gd libraries";
	(cd libsrc/util/gd_lib; make -f makefile.ux);

unix_modules:: FRC
	@for x in $(SUBFOLDERS); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make unix); \
	done

clean_unix:
	(cd ../lib; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	(cd libsrc/util; echo Cleaning in:; pwd; \
		make -f makefile.ux clean);
	(cd libsrc/unix; echo Cleaning in:; pwd; \
		make -f makefile.ux clean);
	(cd libsrc/util/gd_lib; echo Cleaning in:; pwd; \
		make -f makefile.ux clean);
	(cd libsrc/winnt; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make clean_unix); \
	done

clean_bin_unix:
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make clean_bin_); \
	done


#
#  NT directives
#
nt_libs:: FRC
	@echo "Making  C libraries"
	nt_comp libsrc
#	@echo "Making  C++ libraries"
#	nt_comp libsrc_cpp


nt_modules:: FRC
	@echo "Making Earthworm modules"
	nt_make_tree $(SUBFOLDERS)

clean_nt:: FRC
	cd ..\lib
	@echo Cleaning ..\lib
	-del *.o *.obj core *% *~
	cd ..\src\libsrc\util
	@echo Cleaning libsrc\util
	nmake -f makefile.nt clean
	cd gd_lib
	@echo Cleaning libsrc\util\gd_lib
	nmake -f makefile.nt clean
	cd ..\..\winnt
	@echo Cleaning libsrc\winnt
	nmake -f makefile.nt clean
	cd ..\solaris
	@echo Cleaning libsrc\solaris
	-del *.o *.obj core *% *~
	cd ..\..
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder %x clean_nt

clean_bin_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder %x clean_bin_nt

FRC:
