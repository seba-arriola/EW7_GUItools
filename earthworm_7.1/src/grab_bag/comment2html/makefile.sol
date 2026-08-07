#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile.sol,v 1.1 2003/01/30 23:12:14 lucky Exp $
#
#    Revision history:
#     $Log: makefile.sol,v $
#     Revision 1.1  2003/01/30 23:12:14  lucky
#     Initial revision
#
#     Revision 1.3  2000/08/08 18:00:30  lucky
#     Added lint directive
#
#     Revision 1.2  2000/02/14 21:26:54  lucky
#     *** empty log message ***
#
#     Revision 1.1  2000/02/14 16:04:49  lucky
#     Initial revision
#
#
#

CFLAGS = -D_REENTRANT $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin 
L = $(EW_HOME)/$(EW_VERSION)/lib

support_files = api_doc_util.o parse_files_for_keywords.o \
                write_html_comments.o write_html_file.o write_comment_file.o

ew_files =      $L/dirops_ew.o $L/logit.o $L/time_ew.o

all:	$(support_files) comment2html protoparser

comment2html_files = comment2html.o 
comment2html: $(comment2html_files) 
	cc -o $(B)/comment2html $(comment2html_files) $(support_files) $(ew_files) -lm -lposix4

protoparser_files= protoparser.o 
protoparser: $(protoparser_files) 
	cc -o $(B)/protoparser $(protoparser_files) write_comment_file.o $(ew_files) -lm -lposix4

.c.o:
	cc -c ${CFLAGS} $<

lint:
	lint comment2html.o $(support_files) $(GLOBALFLAGS)

depend:
	makedepend -fmakefile.sol -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/comment2html*
