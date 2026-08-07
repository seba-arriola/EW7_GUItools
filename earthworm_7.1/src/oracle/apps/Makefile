#
#       Makefile for Earthworm oracle/apps directory


#
# If a new subfolder is created, it should be added to the 
#  list of SUBFOLDERs
#
SUBFOLDERS = 	src


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
	@echo "Making Earthworm oracle modules"
	@nt_make_tree $(SUBFOLDERS)


clean_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder2 %x\src clean_nt

clean_bin_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder2 %x\src clean_bin_nt

FRC:
