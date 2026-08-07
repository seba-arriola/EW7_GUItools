#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#

SQL_DIR = $(SCHEMA_DIR)/sql_scripts


all:	procs
	-cp -f *.sql $(SQL_DIR)
	-cp -f README* $(SQL_DIR)

procs: procs/*.sql
	@(cd procs; echo Entering:; pwd; \
	make -f makefile.sol; \
	);

