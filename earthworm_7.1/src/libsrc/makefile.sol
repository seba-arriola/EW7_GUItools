
ALL:  util solaris qlib2 lib330
	@echo Successfully built libsrc

util:: FRC
	(cd util; make -f makefile.sol)

solaris:: FRC
	(cd solaris; make -f makefile.sol)

qlib2:: FRC
	(cd qlib2; make)

lib330:: FRC
	(cd lib330; make)

clean: FRC
	(cd ../../lib; echo Cleaning in:; pwd; \
	rm -f *.o *.obj  *% *~);
	(cd util; make -f makefile.sol clean)
	(cd solaris; make -f makefile.sol clean)
	(cd qlib2; make clean)
 
FRC: 
	

