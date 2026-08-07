#
#       makefile for Earthworm
#             Lucky Vidmar Mon Jan 11 21:31:51 MST 1999
#
#
# This makefile contains all the directives necessary to compile
# earthworm modules in a directory under Solaris and NT, as well as 
# clean up the Earthworm source and binary file tree. 
#
# 1. To build (compile) the Earthworm modules on a Sun:
#        make solaris
#
# 2. To build (compile) the Earthworm modules on an NT PC:
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
# Whenever a new module is added to Earthworm, it should be added to
# this makefile to the list of ALL_MODULES, and to the appropriate (or
# both) lists of SOLARIS_MODULES and NT_MODULES. The analogous
# procedure should be followed when a module is removed from the
# Earthworm distribution.
#

#
# List all modules (for cleaning purposes)
#
ALL_MODULES = \
	adsend \
	k2ew \
	naqs2ew \
	psnadsend \
	q2ew \
	q3302ew \
	reboot_mss \
	reboot_mss_ew \
	reftek2ew \
	scream2ew  \
	srparxchewsend \
	windsr2ew


UNIX_MODULES = \
	q3302ew \
	scream2ew  \
	srparxchewsend \
	q2ew

#
# List all modules to be compiled into the NT distribution

#
# List all modules to be compiled into the Solaris distribution
#
SOLARIS_MODULES = \
	k2ew \
	naqs2ew \
	q2ew \
	q3302ew \
	reboot_mss \
	reboot_mss_ew \
	reftek2ew  \
	scream2ew  

#
# List all modules to be compiled into the NT distribution
#

# turned of adsend because most folks don't have the NI includes.

NT_MODULES = \
	k2ew \
	naqs2ew \
	psnadsend \
	reboot_mss \
	q3302ew \
	reboot_mss_ew \
	reftek2ew \
	scream2ew  \
	srparxchewsend \
	windsr2ew 

#       if you've got the NI stuff, uncomment adsend and put it back in 
#       the list above
#
#	adsend \
#
#
# DK 082402  Note:  The NT tree compilation will fail at adsend, unless you 
# comment out the adsend line above, or you have the National Instruments
#	header files to compile adsend.

#
#  Top level directives
#
solaris: sol_modules
nt: nt_modules
unix: unix_modules


#
#  Solaris directives
#
sol_modules:: FRC
	@for x in $(SOLARIS_MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.sol); \
	done

clean_solaris:
	@for x in $(SOLARIS_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done

clean_bin_solaris:
	@for x in $(SOLARIS_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.sol clean_bin); \
	done

#
#  Unix directives
#
unix_modules:: FRC
	@for x in $(UNIX_MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.ux); \
	done

clean_unix:
	@for x in $(UNIX_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done

clean_bin_unix:
	@for x in $(UNIX_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.sol clean_bin); \
	done




#
#  NT directives
#
nt_modules:: FRC
	@echo "Making Earthworm data_sources modules"
	@nt_make_subdirs $(NT_MODULES)

clean_nt:: FRC
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_clbin %x

FRC:
