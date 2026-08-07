
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: review_process.c,v 1.7 2001/05/15 18:47:38 davidk Exp $
 *
 *    Revision history:
 *    $Log: review_process.c,v $
 *    Revision 1.7  2001/05/15 18:47:38  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.6  2001/02/28 17:30:23  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.5  2000/12/06 17:49:49  lucky
 *    *** empty log message ***
 *
 *    Revision 1.4  2000/09/21 19:13:38  lucky
 *    Moved EWDB_atot_msec to time_functions where it was fixed to return
 *    seconds in UTC, rather than localtime.
 *
 *    Revision 1.3  2000/09/18 17:25:21  lucky
 *    Final version before v5.1
 *
 *    Revision 1.2  2000/08/09 16:58:31  lucky
 *    Lint Cleanup
 *
 *    Revision 1.1  2000/08/07 19:54:01  lucky
 *    Initial revision
 *
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time_ew.h>
#include <time_ew.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <time_functions.h>
#include <webparse.h>
#include "../include/review_function.h"


#define			MAX_PARAM_VALUE		32 
#define			PH_TIME_LEN 		30
#define			ARC_MSG_LEN         100000


/* Homemade struct for grabbing event webparams */
typedef struct _PickStruct
{
	char	Sta[10];
	char	Comp[10];
	char	Net[10];
	char	PhTime[MDPCPE][PH_TIME_LEN];
	int		PickWt[MDPCPE];
	int		DelFlag[MDPCPE];
} ReviewPickStruct;


typedef struct _WebOptionsStructReview
{
	int 				EventID;
	double 				Mag;
	int 				Done;
	int 				Cancel;
	int 				Refresh;
	ReviewPickStruct	picks[DB_MAX_PHS_PER_EQ];
} WebOptionsStructReview;


int                 CurrPickInd;
ReviewPickStruct    *pCurrPick;

void			html_cancel (int);
void			html_refresh (int);
void			html_header ();
static	int		UpdateEvtFile (WebOptionsStructReview *);


int main()
{

	int					i, j;
	WebOptionsStructReview	Options;
	char				*configfile = "../params/eqreview.d";
	char				cmd[1024];


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("rev_proc", 1, 1024, 1);
	logit ("", "review_process: starting.\n");

	for (i = 0; i < DB_MAX_PHS_PER_EQ; i++)
	{
		for (j = 0; j < MDPCPE; j++)
			Options.picks[i].DelFlag[j] = 0;
	}

	CurrPickInd = 0;
	pCurrPick = NULL;
	if (Webparse_GetAndProcessWebParams ((void *) (&Options)) == 1)
	{
		html_header (); 
		html_logit ("", "Call to GetAndProcessWebParams() failed\n");
		return EW_FAILURE;
	}

	/* Update the event file by combining the form data with the SAC files */
	if (UpdateEvtFile (&Options) != EW_SUCCESS)
	{
		html_logit ("", "Call to UpdateEvtFile() failed\n");
		return EW_FAILURE;
	}


	/* 
	 * The user requested that the eqreview page be refreshed.
	 * Call eqreview with ACT_REFRESH before putting anything 
	 * else on the screen.
	 */
	if (Options.Refresh == 1)
	{
		ReviewFunction (Options.EventID, ACT_REFRESH, 0);
		return EW_SUCCESS;
	}


	html_header (); 

	/* 
		If the user cancelled out of the eqreview page
		we will remove the temporary review directory so 
		that someone else can come along and review without
		inconvenience.
	 */

	if (Options.Cancel == 1)
	{
		sprintf (cmd, "/bin/rm -rf %s/%d", ReviewTmpDir, Options.EventID);
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
			"OnClick=\"window.location.href='eqreview\?EventID=%u\?Action=%d'; return true;\">\n",
								Options.EventID, ACT_RELOCATE);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"Delete Event\" "
			"OnClick=\"window.location.href='confirm\?EventID=%u\?Action=%d'; return true;\">\n",
								Options.EventID, ACT_DELETE);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"Re-Insert Event\" "
			"OnClick=\"window.location.href='confirm\?EventID=%u\?Action=%d'; return true;\">\n",
								Options.EventID, ACT_INSERT);
	printf ("</TD>\n");

	printf ("</TR>\n");
	printf ("</TABLE>\n");
	printf ("</FORM>\n");
	printf ("</CENTER>\n");

	logit ("", "review_process: terminating.\n");

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
	int				chan, arr;
	char			tmp[64];


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
		 *  Del1 denotes delete arrival 1 of current channel
		 */
	
		/* extract arrival number */
		arr = atoi(&(szVar[3]));

		if (arr > DB_MAX_PHS_PER_EQ)
		{
			logit ("", "Max number of phases (%d) exceeded.\n", DB_MAX_PHS_PER_EQ);
			return -1;
		}

		pCurrPick->DelFlag[arr] = 1;

	}
    else if (strncmp (szVar, "Val", 3) == 0)
	{
		/* extract arrival number */
		arr = atoi(&(szVar[3]));

		if (i > DB_MAX_PHS_PER_EQ)
		{
			logit ("", "Max number of phases (%d) exceeded.\n", DB_MAX_PHS_PER_EQ);
			return -1;
		}

		strcpy (pCurrPick->PhTime[arr], szVal);

	}
    else if (strncmp (szVar, "Wt", 2) == 0)
	{
		/* extract arrival number */
		arr = atoi(&(szVar[2]));

		if (i > DB_MAX_PHS_PER_EQ)
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
    else if (strcmp (szVar, "EventID") == 0)
	{
		pOptions->EventID = atoi (szVal);
	}
    else if (strcmp (szVar, "Mag") == 0)
	{
		pOptions->Mag = atof (szVal);
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
	tmp = (float) atoi (pTemp) * 3600.00;


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
	tmp = tmp + (float) atoi (pTemp) * 60.00;


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
	tmp = tmp + ((float) atoi (pTemp)/100.0);


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
	printf ("</center>\n");
	printf ("<br><br><hr>\n");
}


void		html_refresh (int EventID)
{

	printf ("<hr><br><br>\n");
	printf ("<center>\n");
	printf ("<A HREF=\"eqreview\?EventID=%u\?Action=%d\">Click here to REFRESH the review page for event %d</A>\n", EventID, ACT_REFRESH, EventID);
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
	char				*configfile = "../params/eqreview.d";
	char  				EvtFileName[256];
	char  				filename[256];
	EWEventInfoStruct	EventInfo;
	EWEventInfoStruct	OutEventInfo;
	FILE				*fp;
	int					OutChans, OutArr;
	double				secs;
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
	sprintf (EvtFileName, "%s/%d/EvtStruct.bin", ReviewTmpDir, Options->EventID);

	/* Read the contents of the evt File */
	if ((fp = fopen (EvtFileName, "rb")) == NULL)
	{
		html_header ();
		html_logit ("", "Can't open %s\n", EvtFileName);
		return EW_FAILURE;
	}

	fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* Read the channel info structs */
	if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
			(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		html_header ();
		html_logit ("", "Can't malloc pChanInfo.\n");
		return EW_FAILURE;
	}
	EventInfo.iNumAllocChans = EventInfo.iNumChans;

	fread ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct),
													EventInfo.iNumChans, fp);

	fclose (fp);

	/* Allocate pChanInfo for OutEventInfo */
	if ((OutEventInfo.pChanInfo = (EWChannelDataStruct *) malloc 
			(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		html_header ();
		html_logit ("", "Could not malloc pChanInfo.\n");
		return EW_FAILURE;
	}

	OutEventInfo.iNumChans = 0;
	OutEventInfo.iNumAllocChans = EventInfo.iNumChans;
	
	for (i = 0; i < OutEventInfo.iNumAllocChans; i++)
		InitEWChan (&OutEventInfo.pChanInfo[i]);


	/* Copy Db structs to outgoing DB structs, before modifying */
	memcpy (&OutEventInfo.Event, &EventInfo.Event, sizeof (EWDB_EventStruct));
	memcpy (&OutEventInfo.PrefMag, &EventInfo.PrefMag, sizeof (EWDB_MagStruct));
	memcpy (&OutEventInfo.PrefOrigin, &EventInfo.PrefOrigin, sizeof (EWDB_OriginStruct));


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

		GotRev = FALSE;

		sprintf (filename, "%s/%d/%s.%s.%s.rev", 
						ReviewTmpDir, Options->EventID,
						EventInfo.pChanInfo[i].Station.Sta,
						EventInfo.pChanInfo[i].Station.Comp,
						EventInfo.pChanInfo[i].Station.Net);

		if (stat  (filename, &statbuf) == 0)
		{
			/* .rev file exists: Use ArcFile data */
			GotRev = TRUE;

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
         * if we have no picks, but we have unpicked waveforms,
         * copy the waveform channel for later use.
         */

        if ((EventInfo.pChanInfo[i].iNumArrivals == 0) &&
                (EventInfo.pChanInfo[i].iNumWaveforms > 0))
        {
            OutEventInfo.pChanInfo[OutChans].idChan =
                                    EventInfo.pChanInfo[i].idChan;
            OutEventInfo.pChanInfo[OutChans].iNumWaveforms =
                                    EventInfo.pChanInfo[i].iNumWaveforms;
            OutEventInfo.pChanInfo[OutChans].bResponseIsValid =
                                    EventInfo.pChanInfo[i].bResponseIsValid;
            OutEventInfo.pChanInfo[OutChans].ResponseInfo =
                                EventInfo.pChanInfo[i].ResponseInfo;

            OutEventInfo.pChanInfo[OutChans].iNumArrivals = 0;
            OutEventInfo.pChanInfo[OutChans].iNumStaMags = 0;

            memcpy (&OutEventInfo.pChanInfo[OutChans].Station,
                    &EventInfo.pChanInfo[i].Station, sizeof (EWDB_StationStruct));

            for (j = 0; j < EventInfo.pChanInfo[i].iNumWaveforms; j++)
            {

                memcpy (&OutEventInfo.pChanInfo[OutChans].Waveforms[j],
                        &EventInfo.pChanInfo[i].Waveforms[j],
                                        sizeof (EWDB_WaveformStruct));
            }

            OutChans = OutChans + 1;
        }


		OutArr = 0;
		for (j = 0; j < EventInfo.pChanInfo[i].iNumArrivals; j++)
		{
			/* 
				If delete flag is not set, copy the current 
				channel info to the outgoing struct, then
				update the pick time with the web input value
			 */
	
			if ((pCurrPick != NULL) && (pCurrPick->DelFlag[j] == 1))
			{
				;
			}
			else
			{
				OutEventInfo.pChanInfo[OutChans].idChan = 
										EventInfo.pChanInfo[i].idChan;
				OutEventInfo.pChanInfo[OutChans].iNumWaveforms = 
										EventInfo.pChanInfo[i].iNumWaveforms;
				OutEventInfo.pChanInfo[OutChans].bResponseIsValid = 
										EventInfo.pChanInfo[i].bResponseIsValid;
				OutEventInfo.pChanInfo[OutChans].ResponseInfo = 
										EventInfo.pChanInfo[i].ResponseInfo;
				OutEventInfo.pChanInfo[OutChans].iNumArrivals = 
							OutEventInfo.pChanInfo[OutChans].iNumArrivals + 1;
				OutEventInfo.pChanInfo[OutChans].iNumStaMags = 
							OutEventInfo.pChanInfo[OutChans].iNumStaMags + 1;

				memcpy (&OutEventInfo.pChanInfo[OutChans].Station, 
						&EventInfo.pChanInfo[i].Station, sizeof (EWDB_StationStruct));

				memcpy (&OutEventInfo.pChanInfo[OutChans].Arrivals[OutArr], 
						&EventInfo.pChanInfo[i].Arrivals[j], sizeof (EWDB_ArrivalStruct));

				memcpy (&OutEventInfo.pChanInfo[OutChans].Stamags[OutArr], 
						&EventInfo.pChanInfo[i].Stamags[j], 
						sizeof (EWDB_StationMagStruct));

				/* If we did not review this channel with the applet,
				 * then use the web form data 
				 */
				if (GotRev == FALSE)
				{
					/* update pick time */
					EWDB_atot_msec (pCurrPick->PhTime[j], &secs);
		
					OutEventInfo.pChanInfo[OutChans].Arrivals[j].tObsPhase = secs;
		
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

		if (OutArr > 0)
			OutChans = OutChans + 1;

	} /* i-loop over channels */

	OutEventInfo.iNumChans = OutChans;

	/* Update the magnitude */
	OutEventInfo.PrefMag.dMagAvg = Options->Mag;


	/* Now rewrite the evt file with the modified info */

	if ((fp = fopen (EvtFileName, "wb")) == NULL)
	{
		html_header ();
		html_logit ("", "Can't open %s\n", EvtFileName);
		return EW_FAILURE;
	}

    OutEventInfo.iNumAllocChans = OutEventInfo.iNumChans;

    fwrite ((void *) &OutEventInfo, sizeof (EWEventInfoStruct), 1, fp);

    /* append channel info structs */

    fwrite ((void *) OutEventInfo.pChanInfo, sizeof (EWChannelDataStruct),
                                                    OutEventInfo.iNumChans, fp);

    fclose (fp);

	return EW_SUCCESS;

}


void	html_header ()
{
	/* Send header of reply back to web server */
	printf("Content-type: text/html\n\n");
	printf("<html>\n");

	printf ("<HEAD><TITLE>Review Confirmation</TITLE></HEAD>\n"
							"<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
	printf("<center>\n");
	printf("<H2><FONT COLOR=red>Review Confirmation Page</FONT></H2>\n");
	printf("</center>\n");

	printf("<pre>\n");
}
