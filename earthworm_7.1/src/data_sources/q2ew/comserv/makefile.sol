######################################################################
# Ilya Dricker (IGD) 2006/11/16
# I clean comserv directory leaving only util subdir: this is what q2ew
# needs. Correspondingly, I changed Makefiles
########################################################################
# BINDIR defined with respect to the subdirectories.
BINDIR	= ../bin
CC      =    cc
ALL	= util 

all: 	
	for dir in $(ALL) ; do \
		echo build for $$dir ... ; \
		(cd $$dir; make -f Makefile.solaris_cc all); \
	done

clean:
	for dir in $(ALL) ; do \
		(cd $$dir; make -f Makefile.solaris_cc clean); \
	done

veryclean:
	for dir in $(ALL) ; do \
		(cd $$dir; make -f Makefile.solaris_cc veryclean); \
	done

