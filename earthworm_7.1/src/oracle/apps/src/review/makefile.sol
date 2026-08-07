
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#


#
#  This top level makefile creates both web and text versions
#


default: do_web 
clean: web_clean text_clean
clean_bin: web_clean_bin text_clean_bin

do_web:
	cd web; make solaris

do_text:
	cd text; make solaris


web_clean:
	cd web; make clean_solaris

text_clean:
	cd text; make clean_solaris

web_clean_bin:
	cd web; make clean_bin_solaris

text_clean_bin:
	cd text; make clean_bin_solaris

