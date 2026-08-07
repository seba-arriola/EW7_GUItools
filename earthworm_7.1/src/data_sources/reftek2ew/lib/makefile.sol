
# Make all libraries

all: reftek rtp util

reftek: FORCE
	cd $@; make -f makefile.sol

rtp: FORCE
	cd $@; make -f makefile.sol

util: FORCE
	cd $@; make -f makefile.sol

clean: FORCE
	cd reftek;  make -f makefile.sol $@
	cd rtp;     make -f makefile.sol $@
	cd util;    make -f makefile.sol $@

FORCE:
