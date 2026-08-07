#
#       Station Maintenance Top level makefile 
#
#
# List all directories 
#
OPEN_DIRS = \
	sta_maint \
	sta_export \
	sta_import \
	sta_display
#	sta_gui \

SOLARIS_ONLY_DIRS = 

NT_ONLY_DIRS =  

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


#
#  Solaris directives
#

solaris:: FRC
	@for x in $(SOLARIS_DIRS); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.sol); \
	done

clean_solaris:
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


nt:: FRC
	@echo "Making Oracle Station MaintenanceModules and Programs"
	@nt_make_subdirs $(NT_DIRS) 

clean_nt:: FRC
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clbin %x

FRC:

