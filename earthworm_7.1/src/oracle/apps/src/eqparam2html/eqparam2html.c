
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqparam2html.c,v 1.32 2003/07/03 16:27:22 patton Exp $
 *    Revision history:
 *
 *    $Log: eqparam2html.c,v $
 *    Revision 1.32  2003/07/03 16:27:22  patton
 *    Added new optional command for NIEC
 *
 *    Revision 1.31  2002/09/17 16:21:20  lucky
 *    Added check for other origins before showing the link allowing their review
 *
 *    Revision 1.30  2002/06/19 16:16:34  lucky
 *    Added support for displaying multiple mag types, beyond just Md and ML
 *
 *    Revision 1.28  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.27  2002/05/28 17:24:39  lucky
 *    *** empty log message ***
 *
 *    Revision 1.26  2002/03/19 17:24:40  lucky
 *    Moved links2oragif to html_common
 *
 *    Revision 1.25  2002/03/18 22:19:59  lucky
 *    Moved summary and coincidence_summary HTML routines to html_common.c in libsrc
 *
 *    Revision 1.24  2002/03/15 15:46:53  lucky
 *    Added coincidence trigger stuff
 *
 *    Revision 1.23  2001/11/20 16:42:12  lucky
 *    Fixed to work under NT, and with events with no channels.
 *
 *    Revision 1.22  2001/08/30 02:13:46  lucky
 *    Fixed so that it works under NT.
 *
 *    Revision 1.21  2001/07/20 17:31:37  davidk
 *    changed Set_ewdb_apps_GetDBEventInfo_Debug(1); to  ewdb_apps_Set_GetDBEventInfo_Debug(1); because of a change in fucntion name.
 *
 *    Revision 1.20  2001/07/01 21:55:14  davidk
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
 *    Revision 1.19  2001/06/21 21:52:59  lucky
 *    Added support for two distinct local mag picks - peak2peak and zero2peak.
 *    The choice is installation specific, so there is a new config option LM_method.
 *
 *    Revision 1.18  2001/06/11 17:00:49  lucky
 *    Added support for amplitude picks and magnitudes
 *
 *    Revision 1.17  2001/05/24 22:23:26  lucky
 *    Fixed to reflect the new EWEventInfo structure as well as EventList struct
 *
 *    Revision 1.16  2001/05/15 02:15:10  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.15  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.14  2000/12/21 00:18:32  davidk
 *    added iNumSPDs and SPDS variables to support new WaveformLinks config file command.
 *    Changed ewdb_apps_GetDBEventInfo() call to new ewdb_apps_GetDBEventInfoII() call.  Oops I hardcoded
 *    the flags for the call to 0x4f instead of using the flags #defined in ew_event_info.h
 *    Oh well.....
 *
 *    Revision 1.13  2000/10/11 20:15:33  lucky
 *    Free all allocated channels, not just the ones that were filled in.
 *
 *    Revision 1.12  2000/10/11 18:03:50  lucky
 *    Removed call to InitEWEventINfo -- it is called inside ewdb_apps_GetDBEventInfo.
 *
 *    Revision 1.11  2000/10/10 20:53:17  lucky
 *    Fixed the call to InitEWEvent
 *
 *    Revision 1.10  2000/09/20 18:03:08  lucky
 *    Rewrote to look like eqreview: calls ewdb_apps_GetDBEventInfo and processes the Arrivals
 *    from it.
 *    Cleaned up formatting and many silly logit calls.
 *
 *    Revision 1.9  2000/09/19 21:47:38  lucky
 *    More cleanup of logit messages
 *
 *    Revision 1.7  2000/08/10 23:27:53  bogaert
 *    took out reference to PGE 
 *
 *    Revision 1.6  2000/08/07 19:39:34  lucky
 *    Added WebHost option.
 *
 *    Revision 1.5  2000/05/31 16:47:36  lucky
 *    Modified to reuse some code in the review/alarm applications
 *
 *    Revision 1.4  2000/03/31 23:13:16  davidk
 *    shortened yet added information to a debugging statement.
 *
 *    Revision 1.3  2000/03/31 19:33:41  davidk
 *    *** empty log message ***
 *
 *    Revision 1.2  1999/11/09 16:49:48  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/20 23:56:24  davidk
 *    Initial revision
 *
 *    Revision 1.3  1999/06/07 23:48:13  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  1999/06/04 17:56:52  lucky
 *    Added a call to htmp_map which draws a map of the event and stations
 *    around it on the parameters page.
 *
 *    Revision 1.1  1999/05/05 18:05:41  lucky
 *    Initial revision
 *
 *
 */
  
/*****************************************************************

   filename:       eqparam2html.c 
   module:         eqparam2html
   platforms:      solaris 2.7 (& maybe NT 4)
   project:        Earthworm V
   date:           04/20/1999
   bug creator:    David Kragness

   description:   

 *  Given an eventid, eqparam2html grabs the event's hypocenter 
 *  and parametric data out of the database and prints the
 *  info out in a pretty html form that the user can read.
 *  It also provides links to the record section displays for
 *  a certain event by providing hyperlinks to ora2rsec_gif and
 *  its derivatives.

  Topics:

*****************************************************************/


/* include the normal system stuff pluss errno to boot. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* I forget what we grab out of unistd, but we grab something
   and to support multiple platforms this ugly ifdef is needed */
#ifndef _WINNT  /* DavidK 990119 */
# include <unistd.h> /* Winnt has no idea what this is, ha ha!! */
#endif

/* include earthworm headers */
#include <earthworm.h> 
#include <kom.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <ewdb_apps_utils.h> 

/* include our own header file */
#include "eqparam2html.h"


/* Globals to set from configuration file 
 ****************************************/
int   DEBUG;                     /* debug flag */    
extern int errno;   /* system error variable */

int iNumTGDs=0;             /* Number of "seismic display program
                                    links to add at bottom of page */
TraceGifDisplayStruct TGDS[MAX_TGDS];



main( int argc, char **argv )
/******************************************************
  Function:    main()
  Purpose:     main() is the main function.  It calls all
               neccessary support functions to get you 
               parameters for a given seismic event.  It also
               provides you links to record section displays
               for the requested event.  For people like me
               who don't know what record sections are, they
               are wiggles, like you would see on a seismograph.
  Parameters:    
      Input
      argc:   Number of command like arguments
      argv:   Array of pointers to command line arguments

  Author: DK, before 04/15/1999

  **********************************************************/
{
   /* hard code the config file name.  That's good  Cleanup!! */
   char          *configfile = "../params/eqparam2html.d";
   int           *eventlist;              /* list of eventids to process       */
   int            nevent;                 /* #eventids in list                 */
   EWDBid            eventid;             /* current eventid we're processing  */
   int   i, iev, NumOrigFound, NumOrigRetr;

	EWDB_OriginStruct		TmpOrigin;
	EWEventInfoStruct		EventInfo;




/* Initialize some stuff
 ***********************/
   DEBUG = 0;

   Origins_not_via_Review = 1;


   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>eqparam2html: event details</TITLE></HEAD>\n");

   /* Cleanup, logfile should match config file name */
   /*logit_init ("eqparam2html",1,1024,1);*/  


/* Read the configuration file (path hardcoded relative to executable)
 *********************************************************************/
   ReadConfig( configfile );

   /* Cleanup, logfile should match config file name */
 logit_init ("eqparam2html",1,1024,1);  

 

/* Send html header back to web server
 *************************************/


   html_header (BackgroundColor, HeaderLogo, HeaderTag);

   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Earthworm DBMS<br></FONT></H1>\n" );
   printf("<H2><FONT COLOR=red>Earthquake Parameters retrieved from DBMS</FONT></H2>\n");
   printf("</center>\n");



/* Get list of eventids from da web client
 *********************************/
   if( InputEventIds( &eventlist, &nevent ) )
   {
      logit("o","InputEventIds failed; exiting!\n" );
      html_break();
      goto shutdown;
   }


/* Open connection to database
 *****************************/

   if( ewdb_api_Init(DBuser,DBpassword,DBservice ) != 0 )
   {
      logit( "o", "Trouble connecting to database; exiting!\n" );
      html_break();
      goto shutdown;
   }


/* Loop over list of eventid's from stdin & actually do the work
 ***************************************************************/
   for( iev=0; iev<nevent; iev++ )
   {
      eventid = eventlist[iev];

		/* Get gory details for this event from the database 
		***************************************************/
	    ewdb_apps_Set_GetDBEventInfo_Debug(1);

		if (ewdb_apps_GetDBEventInfoII (&EventInfo, eventid, 
									-1, 0x4f) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_apps_GetDBEventInfo failed\n");
			html_break ();
			continue;
		}


	if (EventInfo.GotLocation == TRUE)
	{	
		html_map (&EventInfo, DEBUG);
		html_principal_summary (&EventInfo, FALSE);
		logit ("e", "Origins_not_via_Review = %d\n", Origins_not_via_Review);

		  printf("<pre>This Solution is the Preferred Solution.\n</pre>");

		/* See if there are any other origins for this event */
		if (ewdb_api_GetOriginsForEvent (eventid, &TmpOrigin, &NumOrigFound, 
									&NumOrigRetr, 1) == EWDB_RETURN_FAILURE)
		{
			html_logit ("", "Call to ewdb_api_GetOriginsForEvent failed.\n");
			html_break();
			continue;
		}

		if (NumOrigFound > 1)
		{
			if (Origins_not_via_Review == 0)
			{	
				printf("<A HREF=\"review_other_solutions?EventID=%u\" "
								"target=\"Review Other Solutions\">"
								"CLICK HERE FOR OTHER SOLUTIONS</A>\n", eventid);
			}

			if (Origins_not_via_Review == 1)
			{	
				logit( "e", "Origins_not_via_Review (eqparam2html)\n");
				printf("<A HREF=\"view_other_solutions?EventID=%u\" "
								"target=\"View Other Solutions\">"
								"CLICK HERE FOR OTHER SOLUTIONS</A>\n", eventid);
			}
		}

		html_arrivals (&EventInfo);
	}

#ifdef DO_COINC
	if (EventInfo.GotTrigger == TRUE)
	{	
		html_coincidence_summary (&EventInfo, FALSE);
	}
#endif DO_COINC


      /* print record section link buttons */
      html_links2ora2rsec_gif(eventid, iNumTGDs, TGDS); 

   } /* end for each eventid from stdin */
   

shutdown:
   free( eventlist );
   ewdb_api_Shutdown();
   html_trailer (WebHost, FooterLogo, FooterTag);
   logit("t","eqparam2html: terminating\n" );

   return( 0 );
}  /* end main */


int CompareValueDist( const void *p1, const void *p2 )
/******************************************************
  Function:    CompareValueDist()
  Purpose:     Compare two items in a list for equality or
               greater than or less than.  A callout function
               from qsort() to allow the user to compare values
               of two items in a list.
  Parameters:    
      Input
      p1:     A pointer to item #1
      p1:     A pointer to item #2

  Return Value:   0 if items 1 and 2 are equal.
                 -1 if item 1 is less than item 2
                  1 if item 2 is less than item 1
  Author:
               DK, before 04/15/1999, actually it was probably
               written by Dietz in 1998, and then screwed up
               by DK in 1999.

  **********************************************************/
{
   StationEQStruct *srt1 = (StationEQStruct *) p1;
   StationEQStruct *srt2 = (StationEQStruct *) p2;

   if(srt1->pPick && srt2->pPick)
   {
     if(srt1->pPick->dDist < srt2->pPick->dDist)   return(-1);
     if(srt1->pPick->dDist > srt2->pPick->dDist)   return( 1);
     return(0);
   }

   if(srt1->pPick)
     return(1);
   else if (srt2->pPick)
     return(-1);
   else
     return(0);

}


/******************************************************
  Function:    CompareStamagValue()
  Purpose:     Compare two items in a list for equality or
               greater than or less than.  A callout function
               from qsort() to allow the user to compare values
               of two items in a list.
  Parameters:    
      Input
      p1:     A pointer to item #1
      p1:     A pointer to item #2

  Return Value:   0 if items 1 and 2 are equal.
                 -1 if item 1 is less than item 2
                  1 if item 2 is less than item 1
  Author:
               DK, before 04/15/1999, actually it was probably
               written by Dietz in 1998, and then screwed up
               by DK in 1999.

  **********************************************************/
int CompareStaMagValue( const void *p1, const void *p2 )
{
   EWDB_StationMagStruct *srt1 = (EWDB_StationMagStruct *) p1;
   EWDB_StationMagStruct *srt2 = (EWDB_StationMagStruct *) p2;

   if(srt1->idChan < srt2->idChan)   return(-1);
   if(srt1->idChan > srt2->idChan)   return( 1);
   return(0);
}

/******************************************************
  Function:    CompareArrivalValue()
  Purpose:     Compare two items in a list for equality or
               greater than or less than.  A callout function
               from qsort() to allow the user to compare values
               of two items in a list.
  Parameters:    
      Input
      p1:     A pointer to item #1
      p1:     A pointer to item #2

  Return Value:   0 if items 1 and 2 are equal.
                 -1 if item 1 is less than item 2
                  1 if item 2 is less than item 1
  Author:
               DK, before 04/15/1999, actually it was probably
               written by Dietz in 1998, and then screwed up
               by DK in 1999.

  **********************************************************/
int CompareArrivalValue( const void *p1, const void *p2 )
{
   EWDB_ArrivalStruct *srt1 = (EWDB_ArrivalStruct *) p1;
   EWDB_ArrivalStruct *srt2 = (EWDB_ArrivalStruct *) p2;

   if(srt1->idChan < srt2->idChan)   return(-1);
   if(srt1->idChan > srt2->idChan)   return( 1);
   return(0);
}




