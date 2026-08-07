
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_process_event.c,v 1.6 2001/08/07 16:53:49 lucky Exp $
 *
 *    Revision history:
 *    $Log: review_process_event.c,v $
 *    Revision 1.6  2001/08/07 16:53:49  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.5  2001/07/31 20:47:20  lucky
 *    Changed alarms nomenclature from User to Recipient
 *
 *    Revision 1.4  2001/07/28 00:45:23  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 21:55:33  davidk
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
 *    Revision 1.2  2001/06/26 17:14:36  lucky
 *    Removed debug stuff
 *
 *    Revision 1.1  2001/06/21 21:26:25  lucky
 *    Initial revision
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* include earthworm headers */
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <alarms.h>
#include <webparse.h>
#include <time_functions.h>
#include <html_common.h>
#include <ewdb_apps_utils.h>

#include <review.h>


static      long            RingId;
static      unsigned char   InstId;
static      unsigned char   ModId;

#define     ARC_MSG_LEN         100000
#define     AUTHOR_MSG_LEN      512
#define     INIT_NUM_ALARMS     100
#define     MAX_ALARMS          5000


typedef struct _webstruct
{
    int     idEvent;
    int     Type;
    int     Action;
    int     NumAlarms;
    char    AlarmsFile[MAXPATH];
    int     UseFlag[MAX_ALARMS];
} WebStruct;

static      int     NumOutAlarms;
static      int     NumPhoneAlarms;

main ()
{
	WebStruct		 			WebOpts;
	EWEventInfoStruct			EventInfo;
	AlarmList					*pAlarms;
	AlarmList					*pAlarmsOut;
	char						*configfile = "../params/review_event.d"; 
	int							retsize, i, j;
	char						filename[256];
	char						tmpstr[1000];
	char						DespaceRecipient[256];
	char						*AuthorMsg;
	FILE						*fp;
	char						InvString[256];
	int							*AlarmNumberMap;
	

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
	logit_init ("review_process_event",1,MAX_BYTES_PER_EQ,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebOpts, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebOpts)) != 0)
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

	/* 
	 * Story: First, process the event. This means either inserting
	 *  or deleting the event. Then, depending on the Alarms action 
	 *  requested, we either issue the alarms, or we go away quietly.
	 *  In either case, after everything is said and done, we give the
 	 *  user the option to return to the event_review page to continue
	 *  reviewing this event.
	 */


	/* Read in the EVENT file */
	sprintf (filename, "%s%c%d%cEvtStruct.bin", 
					ReviewTmpDir, DIR_SLASH, WebOpts.idEvent, DIR_SLASH);

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


	EventInfo.PrefOrigin.idEvent = WebOpts.idEvent;
	EventInfo.PrefOrigin.BindToEvent = TRUE;
	EventInfo.PrefOrigin.SetPreferred = TRUE;

	EventInfo.Mags[EventInfo.iPrefMag].idEvent = WebOpts.idEvent;
	EventInfo.Mags[EventInfo.iPrefMag].bBindToEvent = TRUE;
	EventInfo.Mags[EventInfo.iPrefMag].bSetPreferred = TRUE;

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
							EventInfo.Mags[EventInfo.iPrefMag].idMag;
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
	sprintf (filename, "%s%c%d%cAuthor", 
				ReviewTmpDir, DIR_SLASH, WebOpts.idEvent, DIR_SLASH);

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


	/* Process the event */

	if (WebOpts.Type == ACT_INSERT)
	{

#ifdef DO_DEBUG
logit ("", "Before put event:\n");
PrintEventInfo (&EventInfo);
#endif DO_DEBUG

		if (ewdb_apps_PutDBEventInfo (&EventInfo, tmpstr, NULL, 0) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_apps_PutDBEventInfo failed\n");
			goto shutdown;
		}
	}
	else if (WebOpts.Type == ACT_DELETE)
	{
		if (ewdb_api_DeleteEvent (WebOpts.idEvent, 0) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_DeleteEvent failed\n");
			goto shutdown;
		}
	}
	else
	{
		html_logit ("", "Unknown Type %d\n", WebOpts.Type);
		html_break ();
		goto shutdown;
	}


	printf ("<CENTER><PRE>\n");
	printf ("<hr>\n");
	printf ("<br><br>\n");

	if (WebOpts.Type == ACT_INSERT)
	{
		printf ("EVENT %u SUCCESSFULLY INSERTED INTO THE DATABASE.\n\n",
										WebOpts.idEvent);
	}
	else if (WebOpts.Type == ACT_DELETE)
	{
		printf ("EVENT %u SUCCESSFULLY DELETED FROM THE DATABASE.\n\n",
										WebOpts.idEvent);
	}
	else
	{
		logit ("", "Unknown web action: %d\n", WebOpts.Action);
		html_break ();
		goto shutdown;
	}


	/* Process the alarms, if required */

	if (WebOpts.Action == ACT_ALARMS_YES)
	{

		/* Read the binary alarms file */

		if ((pAlarms = (AlarmList *) malloc 
							(WebOpts.NumAlarms * sizeof (AlarmList))) == NULL)
		{
			html_logit ("", "Could not malloc pAlarms.\n");
			html_break ();
			goto shutdown;
		}

		if ((fp = fopen (WebOpts.AlarmsFile, "rb")) == NULL)
		{
			html_logit ("", "Could not open alarms file: %s.\n", WebOpts.AlarmsFile);
			html_break ();
			goto shutdown;
		}

		fread ((void *) pAlarms, sizeof (AlarmList), WebOpts.NumAlarms, fp);
    

		/* Reconcile with the options retrieved by the web server */

		if ((pAlarmsOut = (AlarmList *) malloc (NumOutAlarms * 
									sizeof (AlarmList))) == NULL)
		{
			html_logit ("", "Could not malloc alarms buffer \n");
			html_break();
			goto shutdown;
		}


		AlarmNumberMap = malloc (WebOpts.NumAlarms * sizeof (int));

		NumOutAlarms = 0;
		NumPhoneAlarms = 0;
		for (i = 0; i < WebOpts.NumAlarms; i++)
		{
			if (WebOpts.UseFlag[i] == TRUE)
			{

				AlarmNumberMap[NumOutAlarms] = i;

				memcpy (&pAlarmsOut[NumOutAlarms], &pAlarms[i], 
											sizeof (AlarmList));
	
				/* Read in the potentially modified text messages */
				sprintf (filename, "%s%c%d%cAlarmsMsg_%d.bin",
							ReviewTmpDir, DIR_SLASH,
							EventInfo.Event.idEvent, DIR_SLASH, i);
	
				if ((fp = fopen (filename, "rb")) == NULL)
				{
					html_logit ("", "Can't open alarms text file: %s.\n", filename);
					html_break();
					goto shutdown;
				}
	
				fread (pAlarmsOut[NumOutAlarms].AlarmMsg, 
										sizeof (char), ALARM_MSG_SIZE, fp);
				fclose (fp);
	
				if (pAlarmsOut[NumOutAlarms].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
					NumPhoneAlarms = NumPhoneAlarms + 1;
	
				NumOutAlarms = NumOutAlarms + 1;
	
			}
		}
	
		/* Execute the alarms */
		if (NumOutAlarms != 0)
		{
			/* Decode earthworm numbers */
			if ((RingId = GetKey (AlarmRing) ) == -1)
			{
				logit ("", "Invalid ring name <%s>!\n", AlarmRing);
				return EW_FAILURE;
			}

			if (GetInst (MyInstID, &InstId) != 0)
			{
				logit ("", "Invalid installation name <%s>!\n", MyInstID);
				return EW_FAILURE;
			}

			if (GetModId (MyModuleID, &ModId) != 0)
			{
				logit ("", " Invalid module name <%s>!\n", MyModuleID);
				return EW_FAILURE;
			}

			/*
			* Build the invocation string which will be included
			* in the alarm audit to indicate where the alarm
			* came from
			*/
			sprintf (InvString, "Event inserted into the database by a reviewer.");

	
			if (ExecuteAlarms (pAlarmsOut, NumOutAlarms, InvString, RingId, 
								InstId, ModId) != EW_SUCCESS)
			{
				html_logit ("", "Call to ExecuteAlarms failed.\n");
				return EW_FAILURE;
			}
		}

	} /* Action is DO_ALARMS */
	
	
	/* 
	 * If the user just deleted an event, we don't want to allow
	 * continuation of review. The event is gone, forever...
	 */

	if ((WebOpts.Action == ACT_ALARMS_YES) && (NumPhoneAlarms > 0))
	{
		printf ("<STRONG>Alarms Phone List</STRONG><BR>\n");
		printf ("The following users should be telephoned in the order listed.\n"
				"Clicking on the Message link will open a new window containing \n"
				"the message to be relayed to the user.\n\n"
				"Check the Done? checkbox after the user has been contacted.\n\n");

		if (WebOpts.Type != ACT_DELETE)
		{
			printf ("Click CONTINUE to send the updated information to the database \n"
				"and continue reviewing the current event.\n");
		}
		printf ("Click FINISH to send the updated information to the database\n"
				"and complete the review process for this event.\n");

	
		printf ("</PRE>\n");
	}
	else
	{
		if (WebOpts.Type != ACT_DELETE)
			printf ("Click on CONTINUE to continue reviewing the current event.\n");

		printf ("Click on FINISH to complete review of this event "
				 "and return to the event list.\n");
	}

	
	printf ("<FORM NAME=\"PhoneListForm\" ACTION=\"review_finish\" METHOD=POST>\n");
	printf ("<TABLE BORDER=1 CELLPADDING=2>\n");

	if ((NumPhoneAlarms > 0) && (WebOpts.Action == ACT_ALARMS_YES))
	{
		/* Put up the list of phone numbers to call */
	
		printf ("<TR VALIGN=center>\n");
		printf ("<TD COLSPAN=2 ALIGN=center>Done?</TD>\n");
		printf ("<TD COLSPAN=4 ALIGN=center>Recipient</TD>\n");
		printf ("<TD COLSPAN=3 ALIGN=center>Number</TD>\n");
		printf ("<TD COLSPAN=3 ALIGN=center>Message</TD>\n");
		printf ("</TR>\n");

		for (i = 0; i < NumOutAlarms; i++)
		{
			if (pAlarmsOut[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			{
				printf ("<TR VALIGN=center>\n");

				/* use checkbox */
				printf ("<TD ALIGN=center COLSPAN=2>");
				printf ("<INPUT TYPE=checkbox NAME=use%d></TD>\n", 
											pAlarmsOut[i].Audit.idAudit);

				/* user description */
				if (pAlarmsOut[i].Audit.Recipient.sDescription != NULL)
					printf ("<TD ALIGN=center COLSPAN=4>%s</TD>", 
								pAlarmsOut[i].Audit.Recipient.sDescription);
				else
					printf ("<TD ALIGN=center COLSPAN=5>Unknown Recipient</TD>");

				/* phone number */
				printf ("<TD ALIGN=center COLSPAN=3>%s</TD>", 
							pAlarmsOut[i].Delivery.phone.sPhoneNumber);


				/* Link to the text of the message */

				/* 
			 	 * First, fill all spaces in the recipient with characters 
				 * becuase the web server doesn't like to receive multi-word
				 * input.
				 */

				for (j = 0; j < strlen (pAlarmsOut[i].Audit.Recipient.sDescription); j++)
				{
					if (pAlarmsOut[i].Audit.Recipient.sDescription[j] == ' ')
						DespaceRecipient[j] = '~';
					else
						DespaceRecipient[j] = pAlarmsOut[i].Audit.Recipient.sDescription[j];
				}
				DespaceRecipient[j] = '\0';

				printf ("<TD ALIGN=center COLSPAN=3>"
						"<A HREF=\"review_show_alarm_msg?Recip=%s?EventID=%u?AlarmNo=%u?PhoneNo=%s\" "
						" TARGET=\"Alarm Message\">%s</A></TD>\n",
									DespaceRecipient,
									EventInfo.Event.idEvent, 
									AlarmNumberMap[i],
									pAlarmsOut[i].Delivery.phone.sPhoneNumber,
									pAlarmsOut[i].Audit.Format.sDescription);

				printf ("</TR>\n");
			}
		}	


	} /* Do we have any phone alarms to process */

	/* pass on the event ID so that the review directory can be deleted */
	printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=\"%d\">\n",
												WebOpts.idEvent);

	printf ("<TR VALIGN=center>\n");

	if (WebOpts.Type != ACT_DELETE)
	{
		printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=6>");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"CONTINUE REVIEWING\" NAME=\"continue\"></TD>\n");
	
		printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=6>");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"FINISH REVIEWING\" NAME=\"finish\"></TD></TR>\n");
	}
	else
	{
		printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=12>");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"FINISH REVIEWING\" NAME=\"finish\"></TD></TR>\n");
	}


	printf ("</TABLE></FORM>\n");


	printf ("<hr>\n");
	printf ("</CENTER>\n");

shutdown:
	ewdb_api_Shutdown();
	html_trailer (WebHost);
	logit("","review_process_event: terminating\n" );

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
          "<BODY BGCOLOR=\"lemonchiffon\" TEXT=#333333>\n" );
   printf("<center>\n");
   printf("</center>\n");

}  /* End html_header() */



/*********************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	int			index;
	WebStruct *pOptions = (WebStruct *) pUserParams;
	

/*
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if (strcmp(szVar,"EventID") == 0)
    {
		pOptions->idEvent = atoi (szVal);
    }
	else if (strncmp(szVar,"Use", 3) == 0)
    {
		/* extract index */
		index = atoi (szVar + 4);
		pOptions->UseFlag[index] = TRUE;
		NumOutAlarms = NumOutAlarms + 1;
	}
	else if (strcmp (szVar,"NumAlarms") == 0)
    {
		pOptions->NumAlarms = atoi (szVal);
	}
	else if (strcmp(szVar,"AlarmsFile") == 0)
    {
		strcpy (pOptions->AlarmsFile, szVal);
	}
	else if (strcmp(szVar,"Type") == 0)
    {
		pOptions->Type = atoi (szVal);
	}
	else if (strcmp(szVar,"Action") == 0)
    {
		pOptions->Action = atoi (szVal);
	}
	else if (strcmp(szVar,"Continue") == 0)
    {
		/* ignore */
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
