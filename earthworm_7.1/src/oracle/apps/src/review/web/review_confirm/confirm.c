
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: confirm.c,v 1.7 2001/05/15 18:47:36 davidk Exp $
 *
 *    Revision history:
 *    $Log: confirm.c,v $
 *    Revision 1.7  2001/05/15 18:47:36  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.6  2001/02/28 17:30:23  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.5  2001/01/02 20:17:01  lucky
 *    Include global vars for configuration options from eqreview.h
 *
 *    Revision 1.4  2000/10/12 17:57:38  lucky
 *    Added RevSrcs options
 *
 *    Revision 1.3  2000/09/18 17:23:57  lucky
 *    Final version before v5.1
 *
 *    Revision 1.2  2000/08/09 16:58:31  lucky
 *    Lint Cleanu
 *
 *    Revision 1.1  2000/08/07 19:46:40  lucky
 *    Initial revision
 *
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* I forget what we grab out of unistd, but we grab something
   and to support multiple platforms this ugly ifdef is needed */
#ifndef _WINNT  /* DavidK 990119 */
# include <unistd.h> 
#endif

/* include earthworm headers */
#include <earthworm.h>
#include <kom.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <webparse.h>
#include <time_functions.h>
#include <map_display_structs.h>

#include "../include/review_function.h"


#define     ARC_MSG_LEN         100000

void		html_delete (WebOptionsStruct *);
void		html_insert (WebOptionsStruct *);


main ()
{
	WebOptionsStruct 			WebParams;
	char						*configfile = "../params/eqreview.d"; 
	

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Send html header back to web server
	 *************************************/
	html_header();

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("confirm",1,1024,1);  

	logit ("", "confirm: starting.\n");


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		goto shutdown;
	}

	/* If Delete flag is set, we delete the event from the DBMS */
	if (WebParams.Action == ACT_DELETE)
	{
		html_delete (&WebParams);
	}


	/* If Insert flag is set, we insert the event into the DBMS */
	else if (WebParams.Action == ACT_INSERT)
	{
		html_insert (&WebParams);
	}
	else
	{
		html_logit ("", "Unknown Action %d\n", WebParams.Action);
		html_break ();
		goto shutdown;
	}


shutdown:
	html_trailer();
	logit("","confirm: terminating\n" );

	return (0);

}  /* end main */



int SetDefaultParams(SetVarsUserStruct * pParams)
/******************************************************
  Function:    SetDefaultParams()
  Purpose:     Set parameters to any default values that 
               we care about.  As this involves hard coded
               parameters in a source file, it is pretty 
               obsolete, but it 0's out a couple of things
               and sounds cool, so we keep it around.
               DEFAULT constants are set in getlist.h
  Parameters:    
      Output
      pParams: 
               the mother of all structs, contains everything
               we know and most of what we care about.

  Author:
               DK, before 04/15/1999

  Internal Functions Called:
               
  Library Functions Called: 
               atotm_date(),atotm_time()

  EW Library Functions Called:

  External Library Functions Called:
               
  System Functions Called:  

  **********************************************************/
{
  /* ECSData  Earthquake Criteria Data (from map_display_structs.h)*/
  pParams->ECSData.MaxLat         = DEFAULT_MAXLAT;
  pParams->ECSData.MinLat         = DEFAULT_MINLAT;
  pParams->ECSData.MaxLon         = DEFAULT_MAXLON;
  pParams->ECSData.MinLon         = DEFAULT_MINLON;

  pParams->ECSData.MaxZ           = DEFAULT_MAXZ;
  pParams->ECSData.MinZ           = DEFAULT_MINZ;
  pParams->ECSData.MaxMag         = DEFAULT_MAXMAG;
  pParams->ECSData.MinMag         = DEFAULT_MINMAG;

  pParams->ECSData.SourceType     = DEFAULT_SOURCE_TYPE;
  pParams->ECSData.Source[0]      = 0;
  pParams->ECSData.EventType      = DEFAULT_EVENT_TYPE;
  pParams->ECSData.MaxEvents      = DEFAULT_MAX_EVENTS;
  pParams->ECSData.LastDaysOnly   = TRUE;
  pParams->ECSData.NumberOfDays   = DEFAULT_NUMBER_OF_DAYS;

  EWDB_atotm_date(&(pParams->ECSData.tmMaxDate),DEFAULT_END_DATE_STR);
  EWDB_atotm_time(&(pParams->ECSData.tmMaxDate),DEFAULT_END_TIME_STR);
  EWDB_atotm_date(&(pParams->ECSData.tmMinDate),DEFAULT_START_DATE_STR);
  EWDB_atotm_time(&(pParams->ECSData.tmMinDate),DEFAULT_START_TIME_STR);

  /* LPSData  Location/Projection Data (from map_display_structs.h)*/
  pParams->LPSData.PixelWidth     = 400;
  pParams->LPSData.PixelHeight    = 400;
  pParams->LPSData.pPSI           = NULL;
  pParams->LPSData.PSILen         = 0;

  /* MSISData  MapServer Image Data (from map_display_structs.h)*/
  pParams->MSISData.ImageFormat = 0;
  pParams->MSISData.ImageFormatsSupported 
                                  = IMAGE_GIF;

  /* WGSSData  Web GUI State String Data (from map_display_structs.h)*/
  /* WGSSData */
  pParams->WGSSData.ShowMap       = 0;
  pParams->WGSSData.ClickEffect   = CLICK_VIEW_EVENTS;
  pParams->WGSSData.StationClickEffect   = EWDB_STATION_DONT_DISPLAY_STATIONS;
  pParams->WGSSData.Options       = 0;
  pParams->WGSSData.XClick        = 0;
  pParams->WGSSData.YClick        = 0;
  pParams->WGSSData.GUIStateString[0]
                                  = 0;
  return(0);
}  /* End SetDefaultParams() */




/******************************************************
     Dumps HTML header, title, and beginning page
 **********************************************************/
void	html_header( void )
{

   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Event Review Confirmation</TITLE></HEAD>\n" 
          "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
   printf("<center>\n");
   printf("</center>\n");

}  /* End html_header() */




/***********************************************************
       Write html to prompt the user to confirm the deletion
 ***************************************************************/
void		html_delete (WebOptionsStruct *pOptions)
{


	printf ("<CENTER><PRE>\n");
	printf ("<hr>\n");
	printf ("<br><br>\n");
	printf ("This action will permanently delete event %u from the database.\n\n",
									pOptions->idEvent);
	printf ("Are you sure? \n\nClick on CANCEL to go back, \n"
				"DELETE WITHOUT ALARMS to bypass the alarms and delete the event,\n"
				"or DELETE WITH ALARMS to review alarms before deleting the event.\n");

	printf ("<FORM NAME=\"ButtonForm\" ACTION=\"NULL\" METHOD=POST>\n");
	printf ("<TABLE>\n");

	printf ("<TR><TD COLSPAN=2 ALIGN=center>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"CANCEL\" "
			"OnClick=\"window.location.href='eqreview\?EventID=%u\?Action=%d'; return true;\">\n",
							pOptions->idEvent, ACT_NULL);
	printf ("</TD></TR>\n");

	printf ("<TR><TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"DELETE WITHOUT ALARMS\" "
			"OnClick=\"window.location.href='process\?EventID=%u\?Action=%d\?Type=%d'; return true;\">\n",
							pOptions->idEvent, ACT_ALARMS_NO, ACT_DELETE);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"DELETE WITH ALARMS\" "
			"OnClick=\"window.location.href='process\?EventID=%u\?Action=%d\?Type=%d'; return true;\">\n",
							pOptions->idEvent, ACT_ALARMS_YES, ACT_DELETE);
	printf ("</TD></TR>\n");

    printf ("</TABLE>\n");
    printf ("</FORM>\n");
	printf ("<hr>\n");


	printf ("</CENTER>\n");

}

/***********************************************************
       Write html to prompt the user to confirm the insertion
 ***************************************************************/
void		html_insert (WebOptionsStruct *pOptions)
{

	printf ("<CENTER><PRE>\n");
	printf ("<hr>\n");
	printf ("<br><br>\n");
	printf ("This action will update event %u in the database.\n\n",
									pOptions->idEvent);
	printf ("Are you sure? \n\nClick on CANCEL to go back, \n"
				"INSERT WITHOUT ALARMS to bypass the alarms and insert the event, \n"
				"or INSERT WITH ALARMS to review alarms before inserting the event.\n");

	printf ("<FORM NAME=\"ButtonForm\" ACTION=\"NULL\" METHOD=POST>\n");
	printf ("<TABLE>\n");

	printf ("<TR><TD COLSPAN=2 ALIGN=center>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"CANCEL\" "
			"OnClick=\"window.location.href='eqreview\?EventID=%u\?Action=%d'; return true;\">\n",
							pOptions->idEvent, ACT_NULL);
	printf ("</TD></TR>\n");


	printf ("<TR><TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"INSERT WITHOUT ALARMS\" "
			"OnClick=\"window.location.href='process\?EventID=%u\?Action=%d\?Type=%d'; return true;\">\n",
							pOptions->idEvent, ACT_ALARMS_NO, ACT_INSERT);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"INSERT WITH ALARMS\" "
			"OnClick=\"window.location.href='process\?EventID=%u\?Action=%d\?Type=%d'; return true;\">\n",
							pOptions->idEvent, ACT_ALARMS_YES, ACT_INSERT);
	printf ("</TD></TR>\n");

    printf ("</TR>\n");
    printf ("</TABLE>\n");
    printf ("</FORM>\n");
	printf ("<hr>\n");


	printf ("</CENTER>\n");

}
