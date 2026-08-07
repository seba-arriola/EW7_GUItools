
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 2.10 2004/06/04 01:16:05 davidk Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 2.10  2004/06/04 01:16:05  davidk
#     Updated wave_viewer to include support for SCNL based Wave Servers.
#     New version number is 2.10.
#
#     Revision 1.1  2000/06/01 22:24:53  davidk
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
	rm -f $B/wave_viewer*

