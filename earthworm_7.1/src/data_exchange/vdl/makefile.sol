#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2004/04/03 22:13:15 lombard Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2004/04/03 22:13:15  lombard
#     Let rm not be stupid about missing files
#
#     Revision 1.1  2000/08/08 17:38:18  lucky
#     Initial revision
#
#
#


# 
#  Earthworm makefile.sol for VDL

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


#  This target passes off all the work to Dave Ketchum's
#  VDL master makefile which also resides in this directory

vdl: 
	(rm -f makedate; make -f makefile.sol makedate);
	make vdlew -f makefile

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/vdl*
