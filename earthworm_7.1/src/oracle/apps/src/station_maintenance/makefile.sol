
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#


#
#  This top level makefile creates all station_maintenance subdirectories
#

APP_SUB_DIR=station_maintenance

default:
	-mkdir -p $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	-cp -f programmer_notes.*   $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	-cp -f *.html               $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	-cp -f bugs.txt             $(WEB_DOC_DIR)/EWDB_APPLICATIONS/$(APP_SUB_DIR)
	make 

clean_solaris:
	make clean_solaris

clean:
	make clean_solaris

clean_bin_solaris:
	make clean_bin_solaris

