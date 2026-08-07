
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: htmlreply_ora2sactarfile.c,v 1.4 2000/09/18 17:22:22 bogaert Exp $
 *    Revision history:
 *
 *    $Log: htmlreply_ora2sactarfile.c,v $
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

void html_header( void );                  /* htmlreply.c */
void html_tableheader( void );             /* htmlreply.c */
void html_shorttableheader( void );        /* htmlreply.c */
void html_shorttablerow( int, char * );    /* htmlreply.c */
void html_shorttrailer( void );            /* htmlreply.c */
void html_tablerow( int, char *, char *, char *, char *, int );    /* htmlreply.c */



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

   printf ("A directory containing the seismograms in the SAC format\n"
           "has been created for each requested event.\n"
           "This directory has been archived using the <I>tar</I> command\n"
           "and the resulting tar file has been compressed using the \n"
           "<I>compress</I> command on a Sun Solaris 2.6 system.\n\n");

   printf("</pre></center>\n");
   return;

}


void html_tableheader( void )
{
   printf("<center>\n"
          "<Table border=1>\n"
          "<tr bgcolor=\"#0000AA\"><strong>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">EventID</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Download SAC<br>Seismograms File</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Number of<br>Seismograms</td>\n");
   printf("</tr>\n");
   return;
}

void html_tablerow( int eventid, char *ftphost, char *ftpdir, 
						char *yrmondir, char *eventdir, int nfiles )
{
   printf("<tr>\n"
          "<td align=center>%d</td> \n"
          "<td align=right><A HREF=\"ftp://%s%s/%s/%s.tar.Z\">%s</A></td> \n"
          "<td align=center>%d</td>\n"
          "</tr>\n",
          eventid, ftphost, ftpdir, yrmondir, eventdir, eventdir, nfiles );

   return;
}

void html_shorttableheader( void )
{
   printf("<center>\n"
          "<Table border=1>\n"
          "<tr bgcolor=\"#0000AA\"><strong>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">EventID</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Output Directory</td>\n");
   printf("</tr></strong>\n");
   return;
}

void html_shorttablerow( int eventid, char *dir )
{
   printf("<tr>\n"
          "<td align=center>%d</td> <td align=right>%s</td>\n"
          "</tr>\n",
          eventid, dir );
   return;
}

/* Write end-html to stdout */
void html_shorttrailer( void )
{
   printf("</table>\n");
   printf("<p>Seismogram retreival may take a few minutes\n" );
   printf("<p><font color=RED>Be Patient</font><br>\n" );
   printf("</html>\n<br>\n");
   return;
}
