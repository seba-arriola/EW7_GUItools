
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#


#
#  This top level makefile creates all alarms subdirectories
#


default: do_web do_crit do_del do_town
clean: web_clean crit_clean del_clean town_clean
clean_bin: web_clean_bin crit_clean_bin del_clean_bin town_clean_bin

do_web:
	(cd web; make solaris)

do_crit:
	(cd crit_programs; make solaris)

do_del:
	(cd delivery_modules; make solaris)

do_town:
	(cd nearest_town; make solaris)


web_clean:
	(cd web; make clean_solaris)

crit_clean:
	(cd crit_programs; make clean_solaris)

del_clean:
	(cd delivery_modules; make clean_solaris)

town_clean:
	(cd nearest_town; make clean_solaris)


web_clean_bin:
	(cd web; make clean_bin_solaris)

crit_clean_bin:
	(cd crit_programs; make clean_bin_solaris)

del_clean_bin:
	(cd delivery_modules; make clean_bin_solaris)

town_clean_bin:
	(cd nearest_town; make clean_bin_solaris)

