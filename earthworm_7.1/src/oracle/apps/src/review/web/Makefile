#
#       Top level makefile 
#




#
# List all directories 
#
ALL_DIRS = \
	retrieve_pick \
	review_alarm_msg \
	review_confirm \
	review_event \
	review_finish \
	review_get_alarms \
	review_next_action \
	review_other_solutions \
	review_process_alarms \
	review_process_event \
	review_show_alarm_msg \
	review_update_alarms_msg 

SOLARIS_DIRS =  \
	retrieve_pick \
	review_alarm_msg \
	review_confirm \
	review_event \
	review_finish \
	review_get_alarms \
	review_next_action \
	review_other_solutions \
	review_process_alarms \
	review_process_event \
	review_show_alarm_msg \
	review_update_alarms_msg 

NT_DIRS =  \
	retrieve_pick \
	review_alarm_msg \
	review_confirm \
	review_event \
	review_finish \
	review_get_alarms \
	review_next_action \
	review_other_solutions \
	review_process_alarms \
	review_process_event \
	review_show_alarm_msg \
	review_update_alarms_msg 



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

