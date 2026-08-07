
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: process.c,v 1.6 2001/05/15 18:47:37 davidk Exp $
 *
 *    Revision history:
 *    $Log: process.c,v $
 *    Revision 1.6  2001/05/15 18:47:37  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
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
#include <alarms.h>
#include <time_functions.h>
#include <map_display_structs.h>

#include "../include/review_function.h"


#define     ARC_MSG_LEN         100000
#define     AUTHOR_MSG_LEN      512
#define     INIT_NUM_ALARMS     100

int		html_delete (int);
int		html_insert (EWEventInfoStruct *, char *);
int		html_doalarms (EWEventInfoStruct *, int);


main ()
{
	WebOptionsStruct 			WebParams;
	EWEventInfoStruct			EventInfo;
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
	logit_init ("process",1,MAX_FORMAT_LEN,1);  


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
	if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n" );
		html_break();
		goto shutdown;
	}

	/* If Insert or DoAlarms flag is set, we read the event info first */
	if ((WebParams.Type == ACT_INSERT) || (WebParams.Type == ACT_DELETE))
	{

		/* Read in the EVENT file */
		sprintf (filename, "%s/%d/EvtStruct.bin", ReviewTmpDir, WebParams.idEvent);

		if ((fp = fopen (filename, "rb")) == NULL)
		{
			html_logit ("", "Can't open %s\n", filename);
			html_break ();
			goto shutdown;	
		}


		fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

        /* Read the channel info structs */
        if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
                    (EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
        {
            html_logit ("", "Can't malloc pChanInfo.\n");
            html_break ();
            goto shutdown;
        }
        EventInfo.iNumAllocChans = EventInfo.iNumChans;

        fread ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct),
                                                        EventInfo.iNumChans, fp);

		fclose (fp);


		EventInfo.PrefOrigin.idEvent = WebParams.idEvent;
		EventInfo.PrefOrigin.BindToEvent = TRUE;
		EventInfo.PrefOrigin.SetPreferred = TRUE;

		EventInfo.PrefMag.idEvent = WebParams.idEvent;
		EventInfo.PrefMag.bBindToEvent = TRUE;
		EventInfo.PrefMag.bSetPreferred = TRUE;

		/* HACK HACK HACK alert */
		/*
		 * It appears that ewdb_apps_GetDBEventInfo does not fill in
		 * the PhaseAmp structures. But, they are used 
		 * in ewdb_apps_PutDBEventInfo to build the DurCoda type of 
		 * tables. So, we need to fill PhaseAmp information 
		 * from the info available elsewhere
		 */

		for (i = 0; i < EventInfo.iNumChans; i++)
		{
			for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
			{
				if(EventInfo.pChanInfo[i].Stamags[j].iMagType == 0 /* DK CLEANUP */)
				{
					EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.idPick = 
						EventInfo.pChanInfo[i].Arrivals[j].idPick;
				EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaTermObs = 
						EventInfo.pChanInfo[i].Arrivals[j].tObsPhase +
						EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaDurObs;
				EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaTermXtp = 
						EventInfo.pChanInfo[i].Arrivals[j].tObsPhase +
						EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaDurXtp;
				}
				EventInfo.pChanInfo[i].Stamags[j].idChan = 
						EventInfo.pChanInfo[i].Arrivals[j].idChan;

				EventInfo.pChanInfo[i].Stamags[j].idMagnitude = 
								EventInfo.PrefMag.idMag;
			}
		}



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

		free (AuthorMsg);

		if (WebParams.Type == ACT_INSERT)
		{
			if (WebParams.Action == ACT_ALARMS_NO)
			{
				if (html_insert (&EventInfo, tmpstr) != EW_SUCCESS)
				{
					html_logit ("", "Call to html_insert failed\n");
					goto shutdown;
				}
			}
			else
			{
				if (html_doalarms (&EventInfo, WebParams.Type) != EW_SUCCESS)
				{
					html_logit ("", "Call to html_doalarms failed\n");
					goto shutdown;
				}
				else
				{
					goto noremove;
				}
			}
		}
		else if (WebParams.Type == ACT_DELETE)
		{
			
			if (WebParams.Action == ACT_ALARMS_NO)
			{
				if (html_delete (EventInfo.Event.idEvent) != EW_SUCCESS)
				{
					html_logit ("", "Call to html_delete failed\n");
					goto shutdown;
				}
			}
			else
			{
				if (html_doalarms (&EventInfo, WebParams.Type) != EW_SUCCESS)
				{
					html_logit ("", "Call to html_doalarms failed\n");
					goto shutdown;
				}
				else
				{
					goto noremove;
				}
			}
		}
	}

	else
	{
		html_logit ("", "Unknown Action %d\n", WebParams.Action);
		html_break ();
		goto shutdown;
	}


shutdown:

	/* DELETE THE REVIEW DIRECTORY */
	sprintf (cmd, "rm -rf %s/%d\n", ReviewTmpDir, WebParams.idEvent);
	system (cmd);

	logit ("", "Review directory deleted!\n");

noremove:
	ewdb_api_Shutdown();
	html_trailer (WebHost);
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
   printf("<HEAD><TITLE>Alarms Confirmation</TITLE></HEAD>\n" 
          "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
   printf("<center>\n");
   printf("</center>\n");

}  /* End html_header() */




/***********************************************************
       Delete the event and put up html confirming the action
 ***************************************************************/
int			html_delete (int idEvent)
{

	int		i, j;

	if (idEvent < 0)
	{
		html_logit ("", "Invalid arguments passed in\n");
		return EW_FAILURE;
	}

	if (ewdb_api_DeleteEvent (idEvent, 0) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "process: Call to ewdb_api_DeleteEvent failed.\n");
		return EW_FAILURE;
	}
	else
	{
		logit ("t", "DONE Deleting Event: %d!\n", idEvent);

		printf ("<CENTER><PRE>\n");
		printf ("<hr>\n");
		printf ("<br><br>\n");
		printf ("EVENT %u SUCCESSFULLY DELETED FROM THE DATABASE.\n\n", idEvent);

		printf ("<br>\n");
		printf ("<hr>\n");
		printf ("</CENTER>\n");

		return EW_SUCCESS;
	}
}

/***********************************************************
     Insert the event and put up html confirming the action
 ***************************************************************/
int		html_insert (EWEventInfoStruct *pEvent, char *author)
{

	int		i, j;

	if ((pEvent == NULL) || (author == NULL))
	{
		html_logit ("", "Invalid arguments passed in\n");
		return EW_FAILURE;
	}


#ifdef DO_DEBUG
logit ("", "Inserting into DB: %d chans.\n", pEvent->iNumChans);
logit ("", "tOrigin(%d)=%0.2f, Mag(%d)=%0.2f\n",
                pEvent->PrefOrigin.idOrigin,
                pEvent->PrefOrigin.tOrigin,
                pEvent->PrefMag.idMag,
                pEvent->PrefMag.dMagAvg);

for (i = 0; i < pEvent->iNumChans; i++)
{
    logit ("", "Chan %d: picks=%d, waves=%d, stamags=%d\n", i,
                    pEvent->pChanInfo[i].iNumArrivals,
                    pEvent->pChanInfo[i].iNumWaveforms,
                    pEvent->pChanInfo[i].iNumStaMags);

	for (j = 0; j < pEvent->pChanInfo[i].iNumStaMags; j++)
	{
		logit ("", "StaMag %d: mag=%0.1f, wt=%0.1f, type=%s\n", j,
						pEvent->pChanInfo[i].Stamags[j].dMag,
						pEvent->pChanInfo[i].Stamags[j].dWeight,
						pEvent->pChanInfo[i].Stamags[j].szMagType);

		logit ("", "PhaseAmp %d: mag=%0.1f, wt=%0.1f, type=%s\n", j,
						pEvent->pChanInfo[i].Stamags[j].dMag,
						pEvent->pChanInfo[i].Stamags[j].dWeight,
						pEvent->pChanInfo[i].Stamags[j].szMagType);
	}

}
#endif DO_DEBUG
		

	if (ewdb_apps_PutDBEventInfo (pEvent, author, NULL, 0) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_apps_PutDBEventInfo failed.\n");
		return EW_FAILURE;
	}
	else
	{
		printf ("<CENTER><PRE>\n");
		printf ("<hr>\n");
		printf ("<br><br>\n");
		printf ("REVIEWED EVENT %u SUCCESSFULLY INSERTED INTO THE DATABASE.\n\n",
											pEvent->Event.idEvent);

		printf ("<br>\n");

		printf ("<hr>\n");
		printf ("</CENTER>\n");
	}
	
		return EW_SUCCESS;
}


/***********************************************************
       Write html to process the alarm list
***************************************************************/
int		html_doalarms (EWEventInfoStruct *pEvent, int Type)
{

	ewdb_AlarmList				*pAlarms;
	int							i, del, isInsert, AlarmsBufSize, NumRetr, NumFound;
	char						filename[512];
	FILE						*fp;

	if (pEvent == NULL) 
	{
		html_logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	/* Retrieve the list of alarms from the DB */

	if ((pAlarms = (ewdb_AlarmList *) malloc (INIT_NUM_ALARMS * 
						sizeof (ewdb_AlarmList))) == NULL)
	{
		html_logit ("", "Could not malloc alarms buffer \n");
		return EW_FAILURE;
	}

	if (Type == ACT_INSERT)
		isInsert = TRUE;
	else
		isInsert = FALSE;

    if (DetermineAlarms (pEvent, 0, isInsert, pAlarms, INIT_NUM_ALARMS, 
								&NumFound, &NumRetr) != EW_SUCCESS)
   	{
        html_logit ("", "Call to DetermineAlarms failed.\n");
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Alarm list too small - got %d alarms, allocating.\n", NumRetr);

		free (pAlarms);

		if ((pAlarms = (ewdb_AlarmList *) malloc (NumRetr * 
							sizeof (ewdb_AlarmList))) == NULL)
		{
			html_logit ("", "Could not malloc alarms buffer \n");
			return EW_FAILURE;
		}
	
		if (DetermineAlarms (pEvent, 0, isInsert, pAlarms, NumRetr, 
								&NumFound, &NumRetr) != EW_SUCCESS)
		{
			html_logit ("", "Call to DetermineAlarms failed.\n");
			return EW_FAILURE;
		}
	}


	/* 
	 * Write the text of alarm message out to files, so 
	 * that the user can review and modify it
	 */

	for (i = 0; i < NumRetr; i++)
	{
		sprintf (filename, "%s/%d/AlarmsMsg_%d.bin", ReviewTmpDir, pEvent->Event.idEvent, i);

		if ((fp = fopen (filename, "wb")) == NULL)
		{
			html_logit ("", "Can't open %s\n", filename);
			return EW_FAILURE;
		}

		fwrite (pAlarms[i].AlarmMsg, sizeof (char), ALARM_MSG_SIZE,  fp);
		fclose (fp);
	}

		
	/* Send header of reply back to web server */
	printf ("<HEAD><TITLE>Alarms Confirmation</TITLE></HEAD>\n"
							"<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
	printf("<center>\n");
	printf("<H2><FONT COLOR=red>Alarms Confirmation Page</FONT></H2>\n");
	printf("</center>\n");

	printf ("<HR><BR><CENTER><PRE><STRONG>\n");
	printf ("REVIEW ALARMS for event %d</STRONG>\n\n"
			"The following %d alarms will be issued. \n"
			"Click the Send? checkbox to cancel a particular alarm.\n\n"
			"Click FINISH to issue the Email and Pager alarms. \n"
			"The list of Phone alarms will be presented on the next page.\n", 
							pEvent->Event.idEvent, NumRetr);


	printf ("<BR><BR><P>\n");

	printf ("</PRE>\n");

	printf ("<FORM NAME=\"AlarmForm\" ACTION=\"process_alarms\" METHOD=POST>\n");
	if (NumRetr != 0)
	{

		printf ("<TABLE border=2 cellspacing=2>");

		printf ("<TR align=center>\n");
		printf ("<TD ALIGN=center COLSPAN=2>Send?</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=5>User</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=3>Method</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=5>Delivery Point</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=3>Format</TD>\n");
		printf ("</TR>\n");

		/* 
		 * Story: we want to display alarms grouped by the 
		 * delivery mechanism (emails, pagers, etc) -- this makes
		 * it easier to see which alarms should or should not be
		 * sent. Unfortunately, we don't want to re-sort the alarm
	  	 * list because it is already sorted in the priority order.
		 *
		 * So, the solution is to go through the list several times, 
		 * each time putting up only the particular delivery types.
	 	 */

		for (del = 0; del < NUM_MECHANISMS; del++)
		{

			for (i = 0; i < NumRetr; i++)
			{

				if (pAlarms[i].Delivery.DelMethodInd == DelMethodArray[del].idMethod)
				{

					printf ("<TR align=center>\n");
	
					/* Put up the select checkbox */
					printf ("<TD ALIGN=center COLSPAN=2> "
								"<INPUT NAME=\"Use_%d\" TYPE=CHECKBOX CHECKED></TD>\n", i); 
	
					/* user */
					if (pAlarms[i].Audit.User.sDescription != NULL)
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Audit.User.sDescription);
					else
						printf ("<TD COLSPAN=5>  </TD>\n");


					/* Show alarm info */
					/* delivery mechanism */
					if (pAlarms[i].Delivery.DelMethodInd == IND_EMAIL)
					{
						printf ("<TD COLSPAN=3>Email</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Delivery.email.sAddress);
					}
					else if (pAlarms[i].Delivery.DelMethodInd == IND_PAGER)
					{
						printf ("<TD COLSPAN=3>Pager</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Delivery.pager.sNumber);
					}
					else if (pAlarms[i].Delivery.DelMethodInd == IND_PHONE)
					{
						printf ("<TD COLSPAN=3>Phone</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Delivery.phone.sPhoneNumber);
					}
					else
					{
						html_logit ("", "Unknown delivery method: %d.\n", 
												pAlarms[i].Delivery.DelMethodInd);
						return EW_FAILURE;
					}
			
					/* format */
					printf ("<TD COLSPAN=3><A HREF=\"review_alarm_msg?EventID=%u?AlarmNo=%u\" "
									" TARGET=\"Review Message\">%s</A></TD>\n", 
											pEvent->Event.idEvent, i, 
											pAlarms[i].Audit.Format.sDescription);

					printf ("</TR>\n");

				} /* if this is the delivery method of interest */

			} /* loop over alarms */

		} /* loop over delivery methods */

		printf ("<TR><TD COLSPAN=18 ALIGN=center>\n");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"Finish\" NAME=\"Finish\">\n");
		printf ("<INPUT TYPE=hidden NAME=\"isInsert\" VALUE=%d>\n", isInsert);
		printf ("<INPUT TYPE=hidden NAME=\"NumAlarms\" VALUE=%d>\n", NumRetr);
		printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=%d>\n", 
									pEvent->Event.idEvent);

		printf("</TABLE></FORM>\n");

		printf ("<PRE>\n "
			"NOTE: The list of phone alarms will be presented "
			"after clicking FINISH.\n<HR>");

	} /* if we got any alarms */
	else
	{
		printf ("<CENTER>\n");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"Finish\" NAME=\"Finish\">\n");
		printf ("<INPUT TYPE=hidden NAME=\"NumAlarms\" VALUE=0>\n");
	
		printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=%d>\n", 
									pEvent->Event.idEvent);

		printf("<P><HR></PRE>\n");
		printf("</CENTER></TABLE></FORM>\n");
	}


	return EW_SUCCESS;

}


