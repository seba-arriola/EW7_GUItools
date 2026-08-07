#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2000/02/14 16:00:43 lucky Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2000/02/14 16:00:43  lucky
#     Initial revision
#
#
#

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/adsend*
