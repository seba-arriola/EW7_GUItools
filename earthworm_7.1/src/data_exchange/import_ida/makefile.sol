
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.2 2006/11/30 19:38:53 stefan Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.2  2006/11/30 19:38:53  stefan
#     added ./ to make.script
#
#     Revision 1.1  2003/01/30 23:11:34  lucky
#     Initial revision
#
#     Revision 1.1  2002/11/03 18:26:34  lombard
#     Initial revision
#
#     Revision 1.1  2000/02/14 18:43:47  lucky
#     Initial revision
#
#
#

#
#  Uses a script to make all components of import_ida
#

all:
	./make.script

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


# Clean-up rules
clean:
	(cd idatap; rm -f libidatap.a *.o src/idatap/*.o src/idatap/*.obj)
	(cd import_ida; rm -f a.out core *.o *.obj *% *~)

clean_bin:
	rm -f $B/import_ida*
