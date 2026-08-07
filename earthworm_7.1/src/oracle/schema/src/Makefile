#
#       Top level makefile for earthworm schema
#


#
# List all directories 
#
ALL_DIRS = \
	alarms \
	external \
	infra \
  merge \
	misc \
	ora_api \
	parametric \
	strong_motion \
	waveform 



#
#  Top level directives
#

default: solaris

#
#  Solaris directives
#
solaris:: sol_make_subdirs sol_make_master_lib

sol_make_subdirs:
	@for x in $(ALL_DIRS); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.sol); \
	done
	
sol_make_master_lib:
	make -f makefile.sol

clean_solaris:: FRC
	(cd ../lib; echo Cleaning in:; pwd; \
		rm -f *.o *.obj core *% *~);
	@for x in $(ALL_DIRS); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done

clean_bin_solaris:: FRC

#
#  NT directives
#
nt: nt_make_subdirs nt_make_master_lib
	

nt_make_subdirs:
	@nt_make_subdirs $(ALL_DIRS)
	
nt_make_master_lib:
	nmake -f makefile.nt master_lib

clean_nt:: FRC
	cd ..\lib 
	@echo Cleaning in ..\lib
	-del *.o *.obj core *% *~
	cd ..\src
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC

FRC:
