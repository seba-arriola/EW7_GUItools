
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: htmlreply.c,v 1.6 2002/02/20 20:16:07 lucky Exp $
 *    Revision history:
 *
 *    $Log: htmlreply.c,v $
 *    Revision 1.6  2002/02/20 20:16:07  lucky
 *    added web appearance parameters
 *
 *    Revision 1.5  2001/07/02 19:38:05  lucky
 *    Removed prototype for DrawMap -- get it from header files.
 *
 *    Revision 1.4  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.3  2000/08/10 23:41:55  bogaert
 *    Change PG&E reference to "Earthworm DBMS"
 *
 *    Revision 1.2  2000/08/07 19:41:15  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2000/02/15 19:45:44  lucky
 *    Initial revision
 *
 *    Revision 1.4  1999/09/22 16:16:40  lucky
 *    FTP connection stuff is now configurable from the config file
 *
 *    Revision 1.3  1999/07/20 15:32:23  lucky
 *    added ability to send email to webmaster
 *
 *    Revision 1.2  1999/06/15 20:25:01  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/06/11 19:49:09  lucky
 *    Initial revision
 *
 *    Revision 1.4  1999/06/08 20:27:03  lucky
 *    *** empty log message ***
 *
 *    Revision 1.3  1999/06/07 23:47:33  lucky
 *    added client side comment lines so that stations will be
 *    identified by their SCN and locations
 *    Also, modified station colors to black (reporting) and white (silent)
 *
 *    Revision 1.2  1999/06/04 17:56:01  lucky
 *    implemented drawing of a map of stations around the event. Stations
 *    that reported picks are drawn in different color from those
 *    that were silent.
 *
 *    Revision 1.1  1999/05/05 18:05:41  lucky
 *    Initial revision
 *
 *
 */


  
/*****************************************************************

   filename:       htmlreply.c 
   module:         eqparam2html
   platforms:      solaris 2.7 (& maybe NT 4)
   date:           04/20/1999
   bug creator:    David Kragness

   description:   

   This is a set of functions to write valid html to stdout

  Topics:

*****************************************************************/


/* include the normal system stuff plus time.h to boot*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* include earthworm headers */
#include <time_ew.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <map_display_structs.h>

/* For GIF drawing */
#include <gd.h>
#include <gdfontg.h>
#include <gdfonts.h>


/* include our own header file */
#include "stainfo.h"

/* define some paramaterized macros (cringe)
   for easier processing */
#define ABS(a) ((a) > 0 ? (a) : -(a))     /* Absolute value */
#define KMtoMILE(a) ((a)*.6214)           /* convert km to miles */



typedef struct _chan_struct
{
	char	chan[10];
	float	samprate;
	float	depth;

} ChanStruct;


static void html_channel_table ();

void html_station ( EWDB_StationStruct *pStation, char *ftphost, char *ftpdir )
/******************************************************
  Function:    html_station()
  Purpose:     Write station information to stdout
  Parameters:    
      Input
      por:     Pointer to an StationStruct whose contents
               will be written in html format to stdout.

  Author: Lucky Vidmar 6/10/1999

  **********************************************************/
{

	ChanStruct	*pChans;
	int			nChans;

   nChans = 3;
	if ((pChans = (ChanStruct *) malloc (nChans * 
							sizeof (ChanStruct))) == NULL)
 	{
		logit ("", "Could not malloc pChans.\n");
		exit (-1);
	}		

   printf ("<BR><HR><BR>\n");
   printf ("<STRONG><PRE>\n");

   printf ("%s\n", "------ INFORMATION ------ <BR>");

   printf ("%30s:   %s\n", "Station Code", pStation->Sta);
   printf ("%30s:   %s\n", "Network Code", pStation->Net);
   printf ("%30s:   Under construction!\n", "Station Name");
   printf ("%30s:   Under construction!\n", "Location");
   printf ("%30s:   Under construction!\n", "Network Affiliations");
   printf ("%30s:   Under construction!\n", "Network Contact");
   printf ("%30s:   Under construction!\n", "Address");

   printf ("<BR><BR><BR><BR>\n");

   printf ("%s\n", "------ LOCATION ------ <BR>");

   printf ("%30s:   %f\n", "Lattitude", pStation->Lat);
   printf ("%30s:   %f\n", "Longitude", pStation->Lon);
   printf ("%30s:   %f\n", "Elevation", pStation->Elev);
   printf ("%30s:   Under construction!\n", "Site Description");
   printf ("%30s:   Under construction!\n", "Date Last Visited");
   printf ("%30s:   Under construction!\n", "Last Visited By");
   printf ("%30s:   Under construction!\n", "Date Last Serviced");
   printf ("%30s:   Under construction!\n", "Last Serviced By");

   printf ("<BR><BR><BR><BR>\n");

   printf ("%s\n", "------ INSTRUMENTATION ------ <BR>");

   printf ("UNDER CONSTRUCTION! This section will contain the list "
           "of seismometers, \ndataloggers, telemetry and other "
           "instruments installed at the station. \nThe following is "
           "an example of what this display may look like." );


   printf ("\n\n");

   printf ("Kinemetrics Episensor with Quanterra Q935 Datalogger\n");
   printf ("  <A HREF=#1><I>5/15/1995 to Present</I></A>\n");

   printf ("\nGuralp CMG40 with Quanterra Q935 Datalogger\n");
   printf ("  <A HREF=#1><I>1/10/1992 to Present</I></A>\n");
    

   printf ("\n\n\nDetailed Information\n\n");

   printf ("Kinemetrics Episensor with Quanterra Q935 Datalogger\n");
   printf ("  <P><A NAME=#1><I>5/15/1995 to Present</I>\n");

   strcpy (pChans[0].chan, "BHZ");
   pChans[0].samprate = 40.0;
   pChans[0].depth = 3.0;

   strcpy (pChans[1].chan, "BHE");
   pChans[1].samprate = 40.0;
   pChans[1].depth = 3.0;

   strcpy (pChans[2].chan, "BHN");
   pChans[2].samprate = 40.0;
   pChans[2].depth = 3.0;

   html_channel_table (pChans, nChans, ftphost, ftpdir);

   printf ("\nGuralp CMG40 with Quanterra Q935 Datalogger\n");
   printf ("  <P><A NAME=#2><I>1/10/1992 to Present</I>\n");
   strcpy (pChans[0].chan, "HLZ");
   pChans[0].samprate = 80.0;
   pChans[0].depth = 3.0;

   strcpy (pChans[1].chan, "HLN");
   pChans[1].samprate = 80.0;
   pChans[1].depth = 3.0;

   strcpy (pChans[2].chan, "HLE");
   pChans[2].samprate = 80.0;
   pChans[2].depth = 3.0;

   html_channel_table (pChans, nChans, ftphost, ftpdir);

   printf ("</STRONG></PRE>\n");
}


static void html_channel_table (ChanStruct *pChans, int nChans, 
											char *ftphost, char *ftpdir )
{

  int i;

  printf ("</PRE>");

  printf ("<table border=4 cellpadding=2 cellspacing=4>"
          "<tr>"
          "<td align=center><b>Channel</b></td>"
          "<td align=center><b>Sampling Rate</b></td>"
          "<td align=center><b>Sensor Depth</b></td>"
          "<td align=center><b>Response</b></td>"
          "</tr>\n");


  for (i = 0; i < nChans; i++)
  {

     printf ("<tr>" 
          "<td align=center>%s</td>"
          "<td align=center>%0.1f</td>"
          "<td align=center>%0.1f</td>"
          "<td align=center><a href=\"ftp://%s%s\">Response for %s</a></td>"
          "</tr>\n", pChans[i].chan, pChans[i].samprate,
                             pChans[i].depth, ftphost, ftpdir, pChans[i].chan);
  }

  printf ("</table>\n");
  printf ("<PRE>");


}
