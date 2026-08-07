
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: review_next_action.c,v 1.11 2003/06/10 14:41:20 lucky Exp $
 *
 *    Revision history:
 *    $Log: review_next_action.c,v $
 *    Revision 1.11  2003/06/10 14:41:20  lucky
 *    *** empty log message ***
 *
 *    Revision 1.10  2002/09/10 17:28:47  lucky
 *    Stable scaffold
 *
 *    Revision 1.9  2002/05/28 19:34:48  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.8  2002/05/28 17:27:23  lucky
 *    *** empty log message ***
 *
 *    Revision 1.7  2002/03/22 20:03:33  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.6  2001/08/10 21:04:51  lucky
 *    NT review and display partially implemented
 *
 *    Revision 1.5  2001/08/07 16:53:49  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.4  2001/07/28 00:45:23  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 21:55:32  davidk
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
 *    Revision 1.2  2001/06/26 17:12:59  lucky
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

#include <time_ew.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <time_functions.h>
#include <webparse.h>
#include <review.h>


#define			MAX_PARAM_VALUE		32 
#define			PH_TIME_LEN 		30
#define			ARC_MSG_LEN         100000


/* Homemade struct for grabbing event webparams */
typedef struct _PickStruct
{
	char	Sta[10];
	char	Comp[10];
	char	Net[10];
	int		PickWt[MDPCPE];
	int		DelPickFlag[MDPCPE];
	int		DelMagFlag[MDPCPE];
} ReviewPickStruct;


typedef struct _WebOptionsStructReview
{
	int 				EventID;
	int 				OriginID;
	char 				EvtDir[1024];
	int 				MapState;
	int 				UnpickedState;
	double 				Md;
	double 				ML;
	int 				PrefMag;
	int 				Done;
	int 				Cancel;
	int 				Refresh;
	int 				GetUnpicked;
	ReviewPickStruct	picks[DB_MAX_PHS_PER_EQ];
} WebOptionsStructReview;


int                 CurrPickInd;
ReviewPickStruct    *pCurrPick;

void			html_cancel (int);
void			html_refresh (int);
static	int		UpdateEvtFile (WebOptionsStructReview *);


static void	html_local_header (void);

int main()
{

	int					i, j;
	WebOptionsStructReview	Options;
	char				*configfile = "../params/review_event.d";
	char				cmd[1024];


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("review_next_action", 1, MAX_BYTES_PER_EQ, 1);
	logit ("", "review_next_action: starting.\n");

	Options.ML = -1;
	Options.Md = -1;
	Options.PrefMag = -1;
	for (i = 0; i < DB_MAX_PHS_PER_EQ; i++)
	{
		for (j = 0; j < MDPCPE; j++)
		{
			Options.picks[i].DelPickFlag[j] = 0;
			Options.picks[i].DelMagFlag[j] = 0;
		}
	}

	CurrPickInd = 0;
	pCurrPick = NULL;
	if (Webparse_GetAndProcessWebParams ((void *) (&Options)) == 1)
	{
		html_local_header (); 
		html_logit ("", "Call to GetAndProcessWebParams() failed\n");
		return EW_FAILURE;
	}

	if ((Options.EventID <= 0) || (Options.OriginID <= 0))
	{
		html_local_header (); 
		html_logit ("", "Invalid idEvent or idOrigin\n");
		return EW_FAILURE;
	}

	sprintf (Options.EvtDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, Options.OriginID);

	/* Update the event file by combining the form data with the SAC files */
	if (UpdateEvtFile (&Options) != EW_SUCCESS)
	{
		html_local_header (); 
		html_logit ("", "Call to UpdateEvtFile() failed\n");
		return EW_FAILURE;
	}


	/* 
	 * The user requested that the review_event page be refreshed.
	 * Call review_event with ACT_REFRESH before putting anything 
	 * else on the screen.
	 */
	if (Options.Refresh == 1)
	{
		ReviewFunction (Options.EventID, Options.OriginID, ACT_REFRESH, 
						Options.MapState, Options.UnpickedState);
		return EW_SUCCESS;
	}


	/* 
	 * The user requested that the we retrieve unpicked traces.
	 * Call review_event with ACT_GETUNPICKED before putting anything 
	 * else on the screen.
	 */
	if (Options.GetUnpicked == 1)
	{
		ReviewFunction (Options.EventID, Options.OriginID, 
								ACT_GETUNPICKED, Options.MapState, TRUE);
		return EW_SUCCESS;
	}



	html_local_header (); 

	/* 
		If the user cancelled out of the review_event page
		we will remove the temporary review directory so 
		that someone else can come along and review without
		inconvenience.
	 */

	if (Options.Cancel == 1)
	{
#ifdef _WINNT
		sprintf (cmd, "rmdir /Q /S %s", Options.EvtDir);
#else
		sprintf (cmd, "/bin/rm -rf %s", Options.EvtDir);
#endif _WINNT
		system (cmd);

		html_cancel (Options.EventID);

		return EW_SUCCESS;
	}
		

	/* 
		Put up buttons allowing the user to delete event, relocate it,
		or insert into the database as is 
	 */
	printf ("<center>\n");
	printf ("CLICK ON A BUTTON TO SELECT NEXT ACTION\n");

	printf ("<FORM NAME=\"ButtonForm\" ACTION=\"NULL\" METHOD=POST>\n");
	printf ("<table> <tr>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"Relocate\" "
			"OnClick=\"window.location.href='review_event\?EventID=%u?OriginID=%u?"
			"Action=%d?ShowMap=%d?ShowUnpicked=%d'; return true;\">\n",
					Options.EventID, Options.OriginID, ACT_RELOCATE, 
					Options.MapState, Options.UnpickedState);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"Delete Event\" "
			"OnClick=\"window.location.href='review_confirm?EventID=%u?"
			"OriginID=%u?Action=%d'; return true;\">\n",
							Options.EventID, Options.OriginID, ACT_DELETE);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"Re-Insert Event\" "
			"OnClick=\"window.location.href='review_confirm?EventID=%u?"
			"OriginID=%d?Action=%d'; return true;\">\n",
							Options.EventID, Options.OriginID, ACT_INSERT);
	printf ("</TD>\n");

	printf ("</TR>\n");
	printf ("</TABLE>\n");
	printf ("</FORM>\n");
	printf ("</CENTER>\n");

	logit ("", "review_next_action: terminating.\n");

	html_trailer (WebHost, FooterLogo, FooterTag);

	return EW_SUCCESS;

}  /* End of main() */




/******************************************************
  Purpose:     Webparse_Client_SetVars() is a callout 
               function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  Webparse_Client_SetVars() is 
               called for each parameter - value pair
 **********************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action they want in the SetVars() function. */

	WebOptionsStructReview *pOptions = (WebOptionsStructReview *) pUserParams;
	int				i, j;
	int				arr;



	if (strcmp (szVar, "scn") == 0)
	{
		/*
         * This is a signal that we got a new channel.
         * Extract the scn and start a new entry.
         */

		pCurrPick = &pOptions->picks[CurrPickInd];
		CurrPickInd = CurrPickInd + 1;

		i = 0;
		j = 0;
		while (szVal[i] != '_')
		{
			pCurrPick->Sta[j] = szVal[i];
			i = i + 1;
			j = j + 1;
		}
		pCurrPick->Sta[j] = '\0';

		i = i + 1;
		j = 0;
		while (szVal[i] != '_')
		{
			pCurrPick->Comp[j] = szVal[i];
			i = i + 1;
			j = j + 1;
		}
		pCurrPick->Comp[j] = '\0';

		i = i + 1;
		j = 0;
		while (szVal[i] != '_')
		{
			pCurrPick->Net[j] = szVal[i];
			i = i + 1;
			j = j + 1;
		}
		pCurrPick->Net[j] = '\0';
	}
    else if (strncmp (szVar, "Del", 3) == 0)
	{
		/* 
		 * NOTE: Parameters have complex formats: 
		 *  DelPick1 denotes delete arrival 1 of current channel
		 *  DelMags1 denotes delete stamag 1 of current channel
		 */
	
		/* extract arrival or stamag number */
		arr = atoi(&(szVar[7]));

		if (arr > DB_MAX_PHS_PER_EQ)
		{
			logit ("", "Max number of phases (%d) exceeded.\n", DB_MAX_PHS_PER_EQ);
			return -1;
		}


		/* extract what do delete */
		if (strncmp (szVar+3, "Pick", 4) == 0)
			pCurrPick->DelPickFlag[arr] = 1;
		else if (strncmp (szVar+3, "Magn", 4) == 0)
			pCurrPick->DelMagFlag[arr] = 1;
		else
		{
			logit ("", "Invalid Del designation <%s>\n", szVar);
			return -1;
		}
	}
    else if (strncmp (szVar, "Wt", 2) == 0)
	{
		/* extract arrival number */
		arr = atoi(&(szVar[2]));

		if (CurrPickInd > DB_MAX_PHS_PER_EQ)  /* DK changed from i to CurrPickInd
                                             as it seemed like that was the
                                             variable desired.  06/30/01 */
		{
			logit ("", "Max number of phases (%d) exceeded.\n", DB_MAX_PHS_PER_EQ);
			return -1;
		}

		pCurrPick->PickWt[arr] = atoi (szVal);

	}
    else if (strcmp (szVar, "Done") == 0)
	{
		pOptions->Done = 1;
	}
    else if (strcmp (szVar, "Cancel") == 0)
	{
		pOptions->Cancel = 1;
	}
    else if (strcmp (szVar, "Refresh") == 0)
	{
		pOptions->Refresh = 1;
	}
    else if (strcmp (szVar, "GetUnpicked") == 0)
	{
		pOptions->GetUnpicked = 1;
	}
    else if (strcmp (szVar, "EventID") == 0)
	{
		pOptions->EventID = atoi (szVal);
	}
    else if (strcmp (szVar, "OriginID") == 0)
	{
		pOptions->OriginID = atoi (szVal);
	}
    else if (strcmp (szVar, "Map") == 0)
	{
		pOptions->MapState = atoi (szVal);
	}
    else if (strcmp (szVar, "Unpicked") == 0)
	{
		pOptions->UnpickedState = atoi (szVal);
	}
    else if (strcmp (szVar, "Md") == 0)
	{
		if (strcmp (szVal, "N/A") != 0)
			pOptions->Md = atof (szVal);
	}
    else if (strcmp (szVar, "ML") == 0)
	{
		if (strcmp (szVal, "N/A") != 0)
			pOptions->ML = atof (szVal);
	}
    else if (strcmp (szVar, "PrefMag") == 0)
	{
		pOptions->PrefMag = atoi (szVal);
	}
	else
	{
		logit ("", "Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}


	return (0);

}




int GetSeconds (float *seconds, char * pBuffer)
/******************************************************
  Purpose:     Convert an Ascii time string in the 
               form 00:00:00.00 to the number of 
			   seconds since midnight.
 **********************************************************/
{

	/*00:00:00.00 */
	char 	*pTemp;
	float	tmp;


	if ((pBuffer == NULL) || (seconds == NULL))
	{
		printf ("GetSeconds: Invalid arguments passed in\n");
		return -1;
	}

	pTemp = pBuffer;


	/* Get the hour */
	pTemp = strchr (pTemp, ':');
	pTemp = pTemp - 2;

	if(pTemp == NULL)
	{
		printf ("GetSeconds: Returning -1 at hour\n");
		return (-1);
	}
	else
		pTemp = pTemp + 1; /* move past the ':' */


	/* number of seconds since midnight for the current hour */
	tmp = (float) ((atoi (pTemp)) * 3600.00);


	/* Get the minute */
	pTemp = strchr (pTemp,':');
	if(pTemp == NULL)
	{
		printf ("GetSeconds: Returning -1 at minute\n");
		return (-1);
	}
	else
		pTemp = pTemp + 1; /* move past the ':' */

	/* add number of seconds for current minutes */
	tmp = tmp + (float) (atoi(pTemp) * 60.00);


	/* Get the second */
	pTemp = strchr (pTemp,':');
	if(pTemp == NULL)
	{
		printf ("GetSeconds: Returning -1 at seconds\n");
		return (-1);
	}
	else
		pTemp = pTemp + 1; /* move past the '.' */


	/* add number of seconds */
	tmp = tmp + (float) atoi (pTemp);


	/* Get the milisecond */
	pTemp = strchr (pTemp,'.');
	if(pTemp == NULL)
	{
		printf ("GetSeconds: Returning -1 at seconds\n");
		return (-1);
	}
	else
		pTemp = pTemp + 1; /* move past the '.' */


	/* add number of miliseconds */
	tmp = tmp + ((float) (atoi (pTemp)/100.0));


	printf ("Calculated %f seconds \n", tmp);

	*seconds = tmp;

	return(0);

} 


void		html_cancel (int EventID)
{

	printf ("<hr><br><br>\n");
	printf ("<center>\n");
	printf ("Review of Event %u TERMINATED - Review directory removed.\n", EventID);
	printf ("<br><br>\n");

	html_trailer (WebHost, FooterLogo, FooterTag);

	printf ("</center>\n");
	printf ("<br><br><hr>\n");
}


void		html_refresh (int EventID)
{

	printf ("<hr><br><br>\n");
	printf ("<center>\n");
	printf ("<A HREF=\"review_event\?EventID=%u\?Action=%d\">Click here to REFRESH the review page for event %d</A>\n", EventID, ACT_REFRESH, EventID);
	printf ("<br><br>");
	printf ("</center>\n");
	printf ("<br><br><hr>\n");
}



/**************************************************************
*
* UpdateEvtFile: Build a new Evt struct by combining the data from
*   the web form with the data in SAC files.
*
*  Algorithm: 
*    for every channel:
*      if a .rev file exists, use data from the EvtStruct file,
*      otherwise use the information from the web form.
*
**************************************************************/
static	int		UpdateEvtFile (WebOptionsStructReview *Options)
{

	int					i, j;
	int					NumPickChans;
	char				*configfile = "../params/review_event.d";
	char  				EvtFileName[256];
	char  				filename[256];
	char  				cmd[300];
	EWEventInfoStruct	EventInfo;
	EWEventInfoStruct	OutEventInfo;
	FILE				*fp;
	int					OutChans, OutArr;
	struct stat 		statbuf;
	int					GotRev;


	/******************************************************
		Read the arc file, convert it into DB structs.
		Check against the DelFlag and get rid of the
		deleted picks, while updating the pick times
		of the picks that remain 

		When done, write the file back out.

		Offer buttons to the user: Delete, Relocate, or
			Insert as is.

	******************************************************/

	/* build the evt file name */
	sprintf (EvtFileName, "%s%cEvtStruct.bin", Options->EvtDir, DIR_SLASH);

	/* Read the contents of the evt File */
	if ((fp = fopen (EvtFileName, "rb")) == NULL)
	{
		html_local_header ();
		html_logit ("", "Can't open %s\n", EvtFileName);
		return EW_FAILURE;
	}

	fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* Read the channel info structs */
	if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
			(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		html_local_header ();
		html_logit ("", "Can't malloc pChanInfo.\n");
		return EW_FAILURE;
	}
	EventInfo.iNumAllocChans = EventInfo.iNumChans;

	fread ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct),
													EventInfo.iNumChans, fp);
	fclose (fp);

#ifdef DO_DEBUG
logit ("", "Before updating\n");
PrintEventInfo (&EventInfo);
#endif DO_DEBUG
#undef DO_DEBUG

	/* Allocate pChanInfo for OutEventInfo */
	if ((OutEventInfo.pChanInfo = (EWChannelDataStruct *) malloc 
			(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		html_local_header ();
		html_logit ("", "Could not malloc pChanInfo.\n");
		return EW_FAILURE;
	}

	OutEventInfo.iNumChans = 0;
	OutEventInfo.iNumAllocChans = EventInfo.iNumChans;
	
	for (i = 0; i < OutEventInfo.iNumAllocChans; i++)
		InitEWChan (&OutEventInfo.pChanInfo[i]);

	/* Copy Db structs to outgoing DB structs, before modifying */
	memcpy (&OutEventInfo.Event, &EventInfo.Event, sizeof (EWDB_EventStruct));
	memcpy (&OutEventInfo.CoincEvt, &EventInfo.CoincEvt, sizeof (EWDB_CoincEventStruct));
	memcpy (&OutEventInfo.PrefOrigin, &EventInfo.PrefOrigin, sizeof (EWDB_OriginStruct));
	memcpy (OutEventInfo.Mags, EventInfo.Mags, MAX_MAGS_PER_ORIGIN * sizeof (EWDB_MagStruct));

	OutEventInfo.iNumMags = EventInfo.iNumMags;
	OutEventInfo.iPrefMag = EventInfo.iPrefMag;
	OutEventInfo.iMd = EventInfo.iMd;
	OutEventInfo.iML = EventInfo.iML;
	OutEventInfo.GotLocation = EventInfo.GotLocation;
	OutEventInfo.GotTrigger = EventInfo.GotTrigger;


	/* Count how many channels have an arrival associated with them */
	NumPickChans = 0;
	for (i = 0; i < EventInfo.iNumChans; i++)
	{
		if (EventInfo.pChanInfo[i].iNumArrivals > 0)
			NumPickChans = NumPickChans + 1;
	}


	/* Go through the picks and update them based on the web input */
	OutChans = 0;

	for (i = 0; i < EventInfo.iNumChans; i++)
	{
logit ("", "Processing channel %d\n", i);
		GotRev = FALSE;

		/* 
		 * If the .rev file exists, we know that this
		 * channel was modified using the applet. 
		 * We'll trust whatever the applet said, not
		 * the information on the web form.
		 */
		sprintf (filename, "%s%c%s.%s.%s.rev", 
						Options->EvtDir, DIR_SLASH, 
						EventInfo.pChanInfo[i].Station.Sta,
						EventInfo.pChanInfo[i].Station.Comp,
						EventInfo.pChanInfo[i].Station.Net);

		if (stat  (filename, &statbuf) == 0)
		{
			/* .rev file exists: Use existing EvtStruct data */
			GotRev = TRUE;

			/* 
			 * Remove the .rev file so that next time through
			 * the process we are not fooled by it.
			 */
#ifdef _WINNT
			sprintf (cmd, "del /Q %s", filename);
#else
			sprintf (cmd, "/bin/rm -f %s", filename);
#endif _WINNT
			system (cmd);
		} /* if .rev file exists */
		else
		{
			GotRev = FALSE;
		}


        /* Find this pick in the list retrieved from the web server */
        pCurrPick = NULL;
        for (j = 0; ((j < CurrPickInd) && (pCurrPick == NULL)); j++)
        {
            if ( (strcmp (EventInfo.pChanInfo[i].Station.Sta, Options->picks[j].Sta) == 0) &&
                (strcmp (EventInfo.pChanInfo[i].Station.Comp, Options->picks[j].Comp) == 0) &&
                (strcmp (EventInfo.pChanInfo[i].Station.Net, Options->picks[j].Net) == 0))
            {
                pCurrPick = &Options->picks[j];
            }
        }


        /*
         * first copy waveform and response info 
         */

		OutEventInfo.pChanInfo[OutChans].idChan =
							EventInfo.pChanInfo[i].idChan;

		OutEventInfo.pChanInfo[OutChans].bResponseIsValid =
							EventInfo.pChanInfo[i].bResponseIsValid;

		memcpy (&OutEventInfo.pChanInfo[OutChans].ResponseInfo,
							&EventInfo.pChanInfo[i].ResponseInfo,
							sizeof (EWDB_ChanTCTFStruct));

		memcpy (&OutEventInfo.pChanInfo[OutChans].Station,
				&EventInfo.pChanInfo[i].Station, sizeof (EWDB_StationStruct));

		OutEventInfo.pChanInfo[OutChans].iNumArrivals = 0;

		OutEventInfo.pChanInfo[OutChans].iNumWaveforms = 
					EventInfo.pChanInfo[OutChans].iNumWaveforms;


		OutEventInfo.pChanInfo[OutChans].iNumStaMags =  0;
		for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
		{
			if (EventInfo.pChanInfo[i].Stamags[j].iMagType != MAGTYPE_DURATION)
			{
				OutEventInfo.pChanInfo[OutChans].iNumStaMags = 
					OutEventInfo.pChanInfo[OutChans].iNumStaMags + 1;

				memcpy (&OutEventInfo.pChanInfo[OutChans].Stamags[j],
							&EventInfo.pChanInfo[i].Stamags[j],
										sizeof (EWDB_StationMagStruct));
			}
		}


		for (j = 0; j < EventInfo.pChanInfo[i].iNumWaveforms; j++)
		{
			memcpy (&OutEventInfo.pChanInfo[OutChans].Waveforms[j],
							&EventInfo.pChanInfo[i].Waveforms[j],
										sizeof (EWDB_WaveformStruct));
		}

		for (j = 0; j < EventInfo.pChanInfo[i].iNumTriggers; j++)
		{
			memcpy (&OutEventInfo.pChanInfo[OutChans].Triggers[j],
							&EventInfo.pChanInfo[i].Triggers[j],
										sizeof (EWDB_TriggerStruct));
		}

		OutEventInfo.pChanInfo[OutChans].iNumTriggers = 
					EventInfo.pChanInfo[OutChans].iNumTriggers;




		OutArr = 0;
		for (j = 0; j < EventInfo.pChanInfo[i].iNumArrivals; j++)
		{
			/* 
				Copy the current channel info to the outgoing struct.
				If delete flag is not set, update the pick time with 
				the web input value. Otherwise, skip the pick
			 */

			if ((pCurrPick != NULL) && (pCurrPick->DelPickFlag[j] == 1))
			{
				/* 
				 * We want to delete the pick, but leave everything else.
				 */
			}
			else
			{
				OutEventInfo.pChanInfo[OutChans].iNumArrivals = 
							OutEventInfo.pChanInfo[OutChans].iNumArrivals + 1;

				memcpy (&OutEventInfo.pChanInfo[OutChans].Arrivals[OutArr], 
						&EventInfo.pChanInfo[i].Arrivals[j], sizeof (EWDB_ArrivalStruct));


				if ((GotRev == TRUE) || 
					((pCurrPick != NULL) && (pCurrPick->DelMagFlag[j] != 1)))
				{
					OutEventInfo.pChanInfo[OutChans].iNumStaMags = 
								OutEventInfo.pChanInfo[OutChans].iNumStaMags + 1;

					memcpy (&OutEventInfo.pChanInfo[OutChans].Stamags[OutArr], 
							&EventInfo.pChanInfo[i].Stamags[j], 
							sizeof (EWDB_StationMagStruct));
				}

				/* If we did not review this channel with the applet,
				 * then use the web form data 
				 */
				if (GotRev == FALSE)
				{
					/* update pick weight -- convert to dSigma */
					if (pCurrPick->PickWt[j] == 3)
						OutEventInfo.pChanInfo[OutChans].Arrivals[j].dSigma = 0.08;
					else if (pCurrPick->PickWt[j] == 2)
						OutEventInfo.pChanInfo[OutChans].Arrivals[j].dSigma = 0.05;
					else if (pCurrPick->PickWt[j] == 1)
						OutEventInfo.pChanInfo[OutChans].Arrivals[j].dSigma = 0.03;
					else if (pCurrPick->PickWt[j] == 0)
						OutEventInfo.pChanInfo[OutChans].Arrivals[j].dSigma = 0.02;
					else
						OutEventInfo.pChanInfo[OutChans].Arrivals[j].dSigma = 99.99;

				} /* channel not reviewed */
				else
				{
					/* Use original ArcFile data (updated by retrieve_pick */
					OutEventInfo.pChanInfo[OutChans].Arrivals[j].tObsPhase = 
								EventInfo.pChanInfo[i].Arrivals[j].tObsPhase;
					OutEventInfo.pChanInfo[OutChans].Arrivals[j].dSigma = 
								EventInfo.pChanInfo[i].Arrivals[j].dSigma;
				}
	
				OutArr = OutArr + 1;
			}

		} /* j-loop over arrivals */

		OutChans = OutChans + 1;

	} /* i-loop over channels */

	OutEventInfo.iNumChans = OutChans;

	/* Update the magnitude information */

	if (OutEventInfo.iMd >= 0)
	{
		/* Update existing Md value */
		OutEventInfo.Mags[OutEventInfo.iMd].dMagAvg = (float)Options->Md;
	}
	else
	{
		if (Options->Md >= 0)
		{
			/* If we got a new Md (hardcoded), add it */ 
			OutEventInfo.Mags[OutEventInfo.iNumMags].dMagAvg =
													(float)Options->Md;
			OutEventInfo.Mags[OutEventInfo.iNumMags].iMagType = MAGTYPE_DURATION;
			OutEventInfo.Mags[OutEventInfo.iNumMags].iNumMags = 0;
			OutEventInfo.Mags[OutEventInfo.iNumMags].dMagErr = 0;
			OutEventInfo.Mags[OutEventInfo.iNumMags].bBindToEvent = TRUE;

			OutEventInfo.iMd = OutEventInfo.iNumMags;
			OutEventInfo.iNumMags = OutEventInfo.iNumMags + 1;
		}
	}



	if (OutEventInfo.iML >= 0)
	{
		/* Update existing ML value */
		OutEventInfo.Mags[OutEventInfo.iML].dMagAvg = (float)Options->ML;
	}
	else
	{
		if (Options->ML >= 0)
		{
			/* If we got a new ML (hardcoded), add it */ 
			OutEventInfo.Mags[OutEventInfo.iNumMags].dMagAvg =
													(float)Options->ML;
			OutEventInfo.Mags[OutEventInfo.iNumMags].iMagType = LM_LocalMagType;
			OutEventInfo.Mags[OutEventInfo.iNumMags].iNumMags = 0;
			OutEventInfo.Mags[OutEventInfo.iNumMags].dMagErr = 0;
			OutEventInfo.Mags[OutEventInfo.iNumMags].bBindToEvent = TRUE;

			OutEventInfo.iML = OutEventInfo.iNumMags;
			OutEventInfo.iNumMags = OutEventInfo.iNumMags + 1;
		}
	}

	
	/* 
	 * this is a giant hack, until we figure out how we handle
	 * all the various magnitude types.
	 */
	if (OutEventInfo.GotLocation == TRUE)
	{
		if (Options->PrefMag == MAGTYPE_DURATION)
			OutEventInfo.iPrefMag = OutEventInfo.iMd;
		else if (Options->PrefMag == LM_LocalMagType)
			OutEventInfo.iPrefMag = OutEventInfo.iML;
		else
		{
			logit ("", "Invalid Preferred Magnitude type: %d.\n", Options->PrefMag);
/*
			return EW_FAILURE;
*/
		}
	}


	/* Now rewrite the evt file with the modified info */

	if ((fp = fopen (EvtFileName, "wb")) == NULL)
	{
		html_local_header ();
		html_logit ("", "Can't open %s\n", EvtFileName);
		return EW_FAILURE;
	}

    OutEventInfo.iNumAllocChans = OutEventInfo.iNumChans;

    fwrite ((void *) &OutEventInfo, sizeof (EWEventInfoStruct), 1, fp);

    /* append channel info structs */

    fwrite ((void *) OutEventInfo.pChanInfo, sizeof (EWChannelDataStruct),
                                                    OutEventInfo.iNumChans, fp);

    fclose (fp);

#ifdef DO_DEBUG
logit ("", "After update:\n");
PrintEventInfo (&OutEventInfo);
#endif DO_DEBUG
#undef DO_DEBUG

	return EW_SUCCESS;

}


static void	html_local_header ()
{
	/* Send header of reply back to web server */
	printf("Content-type: text/html\n\n");
	printf("<html>\n");

	printf ("<HEAD><TITLE>Next Review Step </TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	printf("<center>\n");
	printf("<H2><FONT COLOR=red>Select Next Review Step</FONT></H2>\n");
	printf("</center>\n");

	printf("<pre>\n");
}
