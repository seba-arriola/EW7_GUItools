#
#       Top level makefile 
#


#
# List all directories 
#
ALL_DIRS = \
	alarms_manager \
	alarms_insert_prog \
	alarms_insert_format \
	alarms_insert_recipient \
	alarms_process_prog \
	alarms_process_format \
	alarms_process_new_recipient \
	alarms_process_recipient 

SOLARIS_DIRS =  \
	alarms_manager \
	alarms_insert_prog \
	alarms_insert_format \
	alarms_insert_recipient \
	alarms_process_prog \
	alarms_process_format \
	alarms_process_new_recipient \
	alarms_process_recipient 


NT_DIRS =  \
	alarms_manager \
	alarms_insert_prog \
	alarms_insert_format \
	alarms_insert_recipient \
	alarms_process_prog \
	alarms_process_format \
	alarms_process_new_recipient \
	alarms_process_recipient 




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
	@for %x in ($(NT_DIRS)) \
	do \
		@nt_comp %x

clean_nt:: FRC
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(ALL_DIRS)) \
	do \
		@nt_clbin %x

FRC:

