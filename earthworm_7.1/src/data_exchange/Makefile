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
	coaxtoring \
	condenselogo \
	ew2file \
	export \
	file2ew \
	getfileII \
	getfile_ew \
	import_generic \
	import_ida \
	import_pasv \
	liss \
	liss2ew_scnl \
	makehbfile \
	pickwasher \
	rcv \
	ringdup \
	ringtocoax \
	ringtocoaxII \
	scn_convert \
	sendfileII \
	slink2ew \
	vdl 


#
# List all modules to be compiled into the Solaris distribution
#
SOLARIS_MODULES = \
	coaxtoring \
	condenselogo \
	ew2file \
	export \
	file2ew \
	getfileII \
	getfile_ew \
	import_generic \
	import_ida \
	import_pasv \
	liss \
	makehbfile \
	pickwasher \
	rcv \
	ringdup \
	ringtocoax \
	scn_convert \
	sendfileII \
	slink2ew \
	vdl 

#
# List all modules to be compiled into the NT distribution
#
NT_MODULES = \
	coaxtoring \
	condenselogo \
	ew2file \
	export \
	file2ew \
	getfileII \
	getfile_ew \
	import_generic \
	import_pasv \
	liss \
	liss2ew_scnl \
	makehbfile \
	pickwasher \
	ringdup \
	ringtocoax \
	ringtocoaxII \
	scn_convert \
	slink2ew \
	sendfileII

UNIX_MODULES = \
	coaxtoring \
	condenselogo \
	ew2file \
	export \
	file2ew \
	getfileII \
	getfile_ew \
	import_generic \
	import_pasv \
	liss \
	makehbfile \
	ringdup \
	ringtocoax \
	scn_convert \
	sendfileII \
	slink2ew 

#
# List all stand-alone non-Earthworm programs
#
STANDALONE_MODULES = \
	getfileII \
	makehbfile \
	sendfileII

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
	@for x in $(STANDALONE_MODULES); \
	do \
		(echo ------; cd $$x; echo Copying executable for $$x; \
		cp $$x $(EW_HOME)/$(EW_VERSION)/bin/.; cd ..); \
	done


clean_solaris:
	@for x in $(ALL_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done

clean_bin_solaris:
	@for x in $(ALL_MODULES); \
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
	@for x in $(ALL_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
			make -f makefile.ux clean); \
	done

clean_bin_unix:
	@for x in $(ALL_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
			make -f makefile.ux clean_bin); \
	done




#
#  NT directives
#
nt_modules:: FRC
	@echo "Making Earthworm data_exchange modules"
	@nt_make_subdirs $(NT_MODULES)
	@for %x in ($(STANDALONE_MODULES)) \
	do \
		@nt_copy_exe %x

clean_nt:: FRC
	@for %x in ($(ALL_MODULES)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(ALL_MODULES)) \
	do \
		@nt_clbin %x

FRC:
