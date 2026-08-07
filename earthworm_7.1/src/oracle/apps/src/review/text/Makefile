#
#       Top level makefile 
#




#
# List all directories 
#
ALL_DIRS = \
	db2file \
	tr_delete_event \
	file2db \
	tr_insert_event 

SOLARIS_DIRS =  \
	db2file \
	tr_delete_event \
	file2db \
	tr_insert_event 

NT_DIRS =  \
	db2file \
	tr_delete_event \
	file2db \
	tr_insert_event 



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

