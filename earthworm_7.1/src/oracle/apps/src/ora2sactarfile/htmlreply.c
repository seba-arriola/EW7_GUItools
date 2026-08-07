
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: htmlreply.c,v 1.3 2001/09/10 20:45:08 lucky Exp $
 *    Revision history:
 *
 *    $Log: htmlreply.c,v $
 *    Revision 1.3  2001/09/10 20:45:08  lucky
 *    Added support for download in ZIP format.
 *
 *    Revision 1.2  2001/07/01 21:55:25  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.1  2001/06/07 17:16:25  lucky
 *    Initial revision
 *
 *    Revision 1.4  2000/09/18 17:22:22  bogaert
 *    Final version before v5.1
 *
 *    Revision 1.3  2000/08/07 19:40:21  lucky
 *    Using html_trailer from libsrc
 *
 *    Revision 1.2  2000/03/24 18:26:41  lucky
 *    Changed the way that ora2sac and ora2sactafile are compiled. Now there
 *    is a common function ora2sac_common where the meat of the processing is done.
 *     No more ifdefs, and the compilation process is now automatic.
 *
 *    Revision 1.1  2000/02/15 19:42:46  lucky
 *    Initial revision
 *
 *    Revision 1.6  1999/09/22 16:04:41  lucky
 *    The ftp site and directory are now configurable in the config file
 *
 *    Revision 1.5  1999/07/20 15:37:42  lucky
 *    Added ability to send email to Webmaster
 *
 *    Revision 1.4  1999/07/07 17:59:04  lucky
 *    Added more descriptive explanation of how SAC files are created,
 *    per Alex' request.
 *

 *    Revision 1.2  1999/06/24 00:24:52  lucky
 *    updated to produce anonftp links to tar.Z SAC files
 *
 *    Revision 1.1  1999/05/05 18:32:31  lucky
 *    Initial revision
 *
 *
 */
  
/*  Functions to write valid html to stdout
 *
 */

#include <stdio.h>
#include <string.h>
#include "ora2sactarfile.h"



/* Write html header to stdout */
void html_header( void )
{
   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>ora2sactarfile: Download SAC files</TITLE></HEAD>\n"
          "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Earthworm DBMS<br></FONT></H1>\n" );
   printf("<H2><FONT COLOR=red>Download Event Seismograms in SAC Format</FONT></H2>\n");
   printf("<br><br><pre>\n");

   printf ("SAC Seismograms for the requested events are ready for download.\n"
           "Seismograms are available in <I>TAR</I> or <I>ZIP</I> formats.\n"
           "Clicking on the appropriate link below will begin the download process. \n"
           "You will be prompted for a local directory on your computer where\n"
           "the Seismograms should be saved.\n\n"
           "Close this window when download is finished.\n\n");
		

   printf("</pre></center>\n");
   return;

}


void html_tablerow( int eventid, char *ftphost, char *ftpdir, 
						char *yrmondir, char *eventdir, int nfiles )
{
   printf("<tr>\n"
          "<td align=center>%d</td> \n"
          "<td align=center><A HREF=\"ftp://%s%s/%s/%s.tar.Z\">%s.tar.Z</A></td> \n"
          "<td align=center><A HREF=\"ftp://%s%s/%s/%s.zip\">%s.zip</A></td> \n"
          "<td align=center>%d</td>\n"
          "</tr>\n",
			eventid, 
			ftphost, ftpdir, yrmondir, eventdir, eventdir, 
			ftphost, ftpdir, yrmondir, eventdir, eventdir, 
			nfiles );

   return;
}

void html_tableheader( void )
{
   printf("<center>\n"
          "<Table border=1>\n"
          "<tr bgcolor=\"#0000AA\"><strong>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Event ID</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Download in TAR format</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Download in ZIP format</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Number of Channels</td>\n");
   printf("</tr></strong>\n");
   return;
}

