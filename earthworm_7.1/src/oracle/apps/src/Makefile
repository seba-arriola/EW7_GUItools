#
#       Top level makefile 
#




#
# List all open-platform directories 
#
OPEN_DIRS = \
	aeic2arc \
	alarms \
	archive \
	arc2trigII \
	coinclist \
	db_cleanup \
	db_cleanup2 \
	delete_event \
	eqparam2html \
	ellipse2arc \
	evt_discr \
	gencatalog\
	get_transfunc \
	getimage \
	getlist \
	hypo2ora \
	mag2ora \
#	manual_trigger \
	ora2sactarfile \
	ora2snippet_gif \
	ora_trace_fetch \
	ora_trace_req \
	ora_trace_save \
	orareport \
	reaper \
	review \
	sac_pz2ora \
	scn2pz \
	shakemapfeed \
	sm_ew2ora \
	smdbtest \
	stainfo \
  station_maintenance \
	stalist_hinv2ora \
	stalist_shkmap2ora \
	stalist_usnsn2ora \
	triglist2ora \
	unlocker \
	wvdbtest

SOLARIS_ONLY_DIRS = 

NT_ONLY_DIRS =  \
	configure_ewdb_install

NT_DIRS =  $(OPEN_DIRS) $(NT_ONLY_DIRS)

SOLARIS_DIRS =  $(OPEN_DIRS) $(SOLARIS_ONLY_DIRS)



#
# List all directories 
#
ALL_DIRS = $(OPEN_DIRS) $(NT_ONLY_DIRS) $(SOLARIS_ONLY_DIRS)


#
#  Top level directives
#

default: solaris

solaris: sol_libs sol_modules
nt: nt_libs nt_modules


#
#  Solaris directives
#
sol_libs:: FRC
	echo "Making OS-independent libraries";
	(cd libsrc; make -f makefile.sol);


sol_modules:: FRC
	@for x in $(SOLARIS_DIRS); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.sol); \
	done

clean_solaris:
	@(cd ../lib; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	@(cd libsrc; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	@for x in $(ALL_DIRS); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done

clean_bin_solaris:
	@for x in $(ALL_DIRS); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean_bin); \
	done


#
#  NT directives
#
nt_libs:: FRC
	@echo "Making OS-independent Oracle libraries"
	nt_comp libsrc

nt_modules:: FRC
	@echo "Making Oracle Modules and Programs"
	@nt_make_subdirs $(NT_DIRS) 

clean_nt:: FRC
	cd ..\lib
	@echo Cleaning in ..\lib
	-del *.o *.obj core *% *~
	cd ..\src
	@nt_clean libsrc
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clbin %x

FRC:


