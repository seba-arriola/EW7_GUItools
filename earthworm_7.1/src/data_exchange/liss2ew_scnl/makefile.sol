#
#   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT. 
#
#    $Id: makefile.sol,v 1.1 2007/01/05 18:49:33 dietz Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2007/01/05 18:49:33  dietz
#     Added for cleanup rules only
#
#

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/liss2ew_scnl*
