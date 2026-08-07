
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: process.c,v 1.5 2001/02/28 17:30:23 lucky Exp $
 *
 *    Revision history:
 *    $Log: process.c,v $
 *    Revision 1.5  2001/02/28 17:30:23  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.4  2000/10/12 17:57:59  lucky
 *    added RevSrcs
 *
 *    Revision 1.3  2000/09/18 17:24:53  lucky
 *    Final version before v5.1
 *
 *    Revision 1.2  2000/08/09 16:58:31  lucky
 *    Lint Cleanup
 *
 *    Revision 1.1  2000/08/07 19:52:06  lucky
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
#include <ewdb_ora_api.h> 
#include <webparse.h>
#include <time_functions.h>
#include <map_display_structs.h>

#include "../include/review_function.h"


#define     ARC_MSG_LEN         100000
#define     AUTHOR_MSG_LEN      512

void		html_delete (WebOptionsStruct *);
void		html_insert (WebOptionsStruct *);


main ()
{
	WebOptionsStruct 			WebParams;
	DBEventInfoStruct			EventInfo;
	char						*configfile = "../params/eqreview.d"; 
	int							retsize, i, j;
	char						cmd[256];
	char						filename[256];
	char						tmpstr[1000];
	char						*AuthorMsg;
	FILE						*fp;
	

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
	logit_init ("process",1,1024,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		goto shutdown;
	}

	/* Open connection to database
	 *****************************/
	if (EWDB_OraAPIInit (DBuser, DBpassword, DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n" );
		html_break();
		goto shutdown;
	}

	logit ("t", "Connected to the DB!\n");


	/* If Delete flag is set, we delete the event from the DBMS */
	if (WebParams.Action == ACT_DELETE)
	{
		logit ("t", "Deleting Event: %d!\n", WebParams.idEvent);

		if (ewdb_api_DeleteEvent (WebParams.idEvent, 0) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "process: Call to ewdb_api_DeleteEvent failed.\n");
			goto shutdown;
		}

		logit ("t", "DONE Deleting Event: %d!\n", WebParams.idEvent);

		html_delete (&WebParams);
	}


	/* If Insert flag is set, we insert the event into the DBMS */
	else if (WebParams.Action == ACT_INSERT)
	{

		/* Get the original Author Logo from file */


		if ((AuthorMsg = malloc (AUTHOR_MSG_LEN * sizeof (char))) == NULL)
		{
			html_logit ("", "Can't malloc AuthorMsg.\n");
			html_break ();
			goto shutdown;	
		}

		/* build the file name */
		sprintf (filename, "%s/%d/Author", ReviewTmpDir, WebParams.idEvent);

		if ((fp = fopen (filename, "rt")) == NULL)
		{
			html_logit ("", "Can't open %s.n", filename);
			html_break ();
			goto shutdown;	
		}


		retsize = fread ((void *) AuthorMsg, sizeof (char), AUTHOR_MSG_LEN, fp);

		fclose (fp);
		
		AuthorMsg[retsize] = '\0';


		/*  Build a new author string */

		/* Is this a previously reviewed event? */
		if (strncmp (AuthorMsg, "REV_", 4) != 0)
			sprintf (tmpstr, "REV_%s", AuthorMsg);
		else
			strcpy (tmpstr, AuthorMsg);


		/* Read in the evt file */
		sprintf (filename, "%s/%d/EvtStruct.bin", ReviewTmpDir, WebParams.idEvent);

		if ((fp = fopen (filename, "rb")) == NULL)
		{
			html_logit ("", "Can't open %s.n", filename);
			html_break ();
			goto shutdown;	
		}


		fread ((void *) &EventInfo, sizeof (DBEventInfoStruct), 1, fp);

        /* Read the channel info structs */
        if ((EventInfo.pChanInfo = (DBChannelDataStruct *) malloc
                    (EventInfo.iNumChans * sizeof (DBChannelDataStruct))) == NULL)
        {
            html_logit ("", "Can't malloc pChanInfo.\n");
            html_break ();
            goto shutdown;
        }
        EventInfo.iNumAllocChans = EventInfo.iNumChans;

        fread ((void *) EventInfo.pChanInfo, sizeof (DBChannelDataStruct),
                                                        EventInfo.iNumChans, fp);

		fclose (fp);


		EventInfo.PrefOrigin.idEvent = WebParams.idEvent;
		EventInfo.PrefOrigin.BindToEvent = TRUE;
		EventInfo.PrefOrigin.SetPreferred = TRUE;

		EventInfo.PrefMag.idEvent = WebParams.idEvent;
		EventInfo.PrefMag.BindToEvent = TRUE;
		EventInfo.PrefMag.SetPreferred = TRUE;

		/* HACK HACK HACK alert */
		/*
		 * It appears that GetDBEventInfo does not fill in
		 * the PhaseAmp structures. But, they are used 
		 * in PutDBEventInfo to build the DurCoda type of 
		 * tables. So, we need to fill PhaseAmp information 
		 * from the info available elsewhere
		 */

		for (i = 0; i < EventInfo.iNumChans; i++)
		{
			for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
			{
				strcpy (EventInfo.pChanInfo[i].PhaseAmps[j].szMagType, "DurCoda");
				EventInfo.pChanInfo[i].PhaseAmps[j].idPick = 
						EventInfo.pChanInfo[i].Arrivals[j].idPick;
				EventInfo.pChanInfo[i].PhaseAmps[j].idChan = 
						EventInfo.pChanInfo[i].Arrivals[j].idChan;

				EventInfo.pChanInfo[i].PhaseAmps[j].idMagnitude = 
								EventInfo.PrefMag.idMag;

				EventInfo.pChanInfo[i].PhaseAmps[j].dMag = 
						EventInfo.pChanInfo[i].Stamags[j].dMag;
				EventInfo.pChanInfo[i].PhaseAmps[j].dWeight = 
						EventInfo.pChanInfo[i].Stamags[j].dWeight;
				EventInfo.pChanInfo[i].PhaseAmps[j].tCodaTermObs = 
						EventInfo.pChanInfo[i].Arrivals[j].tObsPhase +
						EventInfo.pChanInfo[i].Stamags[j].dMeasurementObs;
				EventInfo.pChanInfo[i].PhaseAmps[j].tCodaTermXtp = 
						EventInfo.pChanInfo[i].Arrivals[j].tObsPhase +
						EventInfo.pChanInfo[i].Stamags[j].dMeasurementCalc;
			}
		}
		
logit ("", "Inserting into DB: %d chans\n", EventInfo.iNumChans);
logit ("", "tOrigin(%d)=%0.2f, Mag(%d)=%0.2f\n",
                EventInfo.PrefOrigin.idOrigin,
                EventInfo.PrefOrigin.tOrigin,
                EventInfo.PrefMag.idMag,
                EventInfo.PrefMag.dMagAvg);

for (i = 0; i < EventInfo.iNumChans; i++)
{
    logit ("", "Chan %d: picks=%d, waves=%d, stamags=%d\n", i,
                    EventInfo.pChanInfo[i].iNumArrivals,
                    EventInfo.pChanInfo[i].iNumWaveforms,
                    EventInfo.pChanInfo[i].iNumStaMags);
}

		
		if (PutDBEventInfo (&EventInfo, tmpstr, NULL, 0) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to PutDBEventInfo failed.\n");
			html_break ();
			goto shutdown;	
		}

		free (AuthorMsg);

		html_insert (&WebParams);
	}
	else
	{
		html_logit ("", "Unknown Action %d\n", WebParams.Action);
		html_break ();
		goto shutdown;
	}


shutdown:
	EWDB_OraAPIShutdown();

	/* DELETE THE REVIEW DIRECTORY */
	sprintf (cmd, "rm -rf %s/%d\n", ReviewTmpDir, WebParams.idEvent);
	system (cmd);

	logit ("", "Review directory deleted!\n");

	html_trailer();
	logit("","process: terminating\n" );

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
	printf ("EVENT %u SUCCESSFULLY DELETED FROM THE DATABASE.\n\n",
									pOptions->idEvent);

	printf ("<br>\n");
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
	printf ("REVIEWED EVENT %u SUCCESSFULLY INSERTED INTO THE DATABASE.\n\n",
									pOptions->idEvent);

	printf ("<br>\n");

	printf ("<hr>\n");
	printf ("</CENTER>\n");

}
