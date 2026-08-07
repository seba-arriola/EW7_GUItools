/* *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: merge_process.c,v 1.1 2004/03/17 18:38:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: merge_process.c,v $
 *     Revision 1.1  2004/03/17 18:38:17  davidk
 *     Initial revision
 *
 *
 *************************************************************/
  

/*** #includes ***/

/* include the normal stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time_ew.h>
#include <ew_event_info.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <merge.h>

#include <review.h>

typedef struct evtlist
{

	EWDBid	idEvent;
	EWDBid	idPh;
	int		Merge;
	int		Auto;
} EvtList;

typedef struct webopts
{
	EWDBid 	idPh;
	EWDBid 	idPrefEvent;
	int 	NumEvents;
	EvtList	Events[MAX_MERGES_PER_EVENT];
} WebOpts;


static	int 	NumToMerge;
static	int 	PrefIndex;

static int		RebindEvent (int NewEventID, 
							EWEventInfoStruct *pEvent, int isPref);

int main(int argc, char * argv[])
{

	WebOpts					WebParams;
  	char					*configfile = "../params/review_event.d";
	int						i, j, k, NumFound, NumRetr, NumMerged, Pref;
	EWDBid					NewEventID, retval;
	EWEventInfoStruct		EventInfo;
    EWDB_MergeStruct 		Merge;
    EWDB_PhenomenonStruct	Phen;
	EWDB_MergeList			*pMergeBuffer;
	int						MERGE_LIST_LEN = 3;		

    DEBUG = 0;

   /* Read the configuration file (path hardcoded relative to executable)
     *********************************************************************/
    ReadConfig (configfile);

    /* Cleanup, logfile should match config file name */
    logit_init ("merge_process",1,MAX_BYTES_PER_EQ,1);

	memset (&WebParams, 0, sizeof (WebOpts));
	NumToMerge = 0;
	NumMerged = 0;
	PrefIndex = -1;

    if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
    {
        html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
        html_break ();
        goto shutdown;
    }
logit ("", "To merge %d out of %d events\n", NumToMerge, WebParams.NumEvents);
for (i = 0; i < WebParams.NumEvents; i++)
 logit ("", "Evt %d: %d %d %d %d\n", i, 
  WebParams.Events[i].idEvent,
  WebParams.Events[i].idPh,
  WebParams.Events[i].Merge,
  WebParams.Events[i].Auto);

    /* Send html header back to web server
     *************************************/
    printf("Content-type: text/html\n\n");
    printf("<html>\n");

    printf ("<HEAD><TITLE>Processing Merges for %d</TITLE></HEAD>\n", 
														WebParams.idPh );

	html_header (BackgroundColor, HeaderLogo);

    printf("<center>\n");
    printf("<H2><FONT COLOR=red>Process Merge Candidates</FONT></H2>\n");
    printf("</center>\n");

    printf("<pre>\n");


	/* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
    {
        html_logit ("", "Trouble connecting to database; exiting!\n" );
        html_break();
        goto shutdown;
    }


	if ((pMergeBuffer = (EWDB_MergeList *) malloc (MERGE_LIST_LEN *
									sizeof (EWDB_MergeList))) == NULL)
	{
		html_logit ("", "Could not malloc MergeBuffer.\n");
		html_break();
		goto shutdown;
	}


	/*
	 * PROCESS REJECTED AUTOMATIC MERGE CANDIDATES:
	 *   Remove from merge table, create new phenomenon.
	 */

	for (i = 0; i < WebParams.NumEvents; i++)
	{
		if (WebParams.Events[i].Auto == TRUE)
		{
			if (WebParams.Events[i].Merge == FALSE)
			{
logit ("", "Processing rejected merge for event %d\n", WebParams.Events[i].idEvent);
				/*
				 * Remove the existing merge, create a new phenomenon,
			 	 * merge this event with the new phenomenon.
				 */
	
				if (ewdb_api_DeleteMerge (WebParams.Events[i].idPh, WebParams.Events[i].idEvent)
								!= EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_DeleteMerge failed.\n");
   		     		html_break();
					goto shutdown;
				}
		
		
				Phen.idPh = -1;
				Phen.idPrefEvent = WebParams.Events[i].idEvent;
				strcpy (Phen.szSource, "Merge Processor");
	
				if (ewdb_api_CreatePhenomenon (&Phen) != EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_CreatePhenomenon failed.\n");
					html_break ();
					goto shutdown;
				}

				if (Phen.idPh < 0)
				{
					html_logit ("", "Call to ewdb_api_CreatePhenomenon returned %d.\n", Phen.idPh);
					html_break ();
					goto shutdown;
				}


				/* Now, merge this event with the newly created phenomenon */
	
				Merge.idMerge = -1;
				Merge.idPh = Phen.idPh;
				Merge.idEvent =  WebParams.Events[i].idEvent;
				strcpy (Merge.szSource, "Merge Processor");
	
				if (ewdb_api_CreateMerge (&Merge) != EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_CreateMerge failed.\n");
					html_break ();
					goto shutdown;
				}
	
				if (Merge.idMerge < 0)
				{
					html_logit ("", "Call to ewdb_api_CreateMerge returned %d.\n", Merge.idMerge);
					html_break ();
					goto shutdown;
				}
	
			} /* If this is a rejected merge event */

		} /* If this is an automatic merge candidate */

	} /* loop over merge candidates */


	/*
	 * Special case where there is only one event accepted for merge.
	 * Since we always merge the preferred event, this means that 
	 * no other event was selected for merging.  We processed all rejected
	 * merges above, so we have nothing else left to do...
	 */
	if (NumToMerge == 1)
	{
logit ("", "Only the preferred event to merge - exit.\n");
		goto shutdown;
	}


	/* 
	 * PROCESS APPROVED MERGE CANDIDATES:
	 * Loop through the list of accepted merge events. Retrieve info from DB.
	 * If this is the first event, 
	 *   Remove Merge
	 *   Create a new event
	 *   Merge with the phenomenon
	 *   Update the phenomenon's preferred event ID 
	 * 
	 * Otherwise 
	 *   Remove Merge 
	 *   Insert additional info to be associated with the new event
	 *   If this event is preferred, update preferred info.
	 */
	NewEventID = -1;
	for (i = 0; i < WebParams.NumEvents; i++)
	{
		if (WebParams.Events[i].Merge == TRUE)
		{

			if (i == PrefIndex)
				Pref = TRUE;
			else
				Pref = FALSE;

			/* Allocate and initialize temporary Event struct */
			if (InitEWEvent (&EventInfo) != EW_SUCCESS)
			{
				html_logit ("", "Call to InitEWEvent failed.\n");
				html_break ();
				goto shutdown;
			}

logit ("", "Call ewdb_apps_GetDBEventInfoII for %d\n", WebParams.Events[i].idEvent);
			if (ewdb_apps_GetDBEventInfoII (&EventInfo, 
							WebParams.Events[i].idEvent, -1, 0x4f) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
				html_break ();
				goto shutdown;
			}

/*
logit ("", "Retrieved the following:\n");
PrintEventInfo (&EventInfo);
*/

			if (NewEventID < 0)
			{
logit ("", "Accepted first Merge %d: %d \n", i, WebParams.Events[i].idEvent);

				/* insert a new event */
				strcpy (EventInfo.Event.szSource, "Merge Processor");
				sprintf (EventInfo.Event.szSourceEventID, "%d", EventInfo.Event.idEvent);

logit ("", "Call ewdb_api_CreateEvent with source <%s>, id <%s>\n",
EventInfo.Event.szSource, EventInfo.Event.szSourceEventID);
				if (ewdb_api_CreateEvent (&EventInfo.Event) != EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_CreateEvent failed.\n");
			   		html_break();
					goto shutdown;
				}
logit ("", "Inserted new event %d\n", EventInfo.Event.idEvent);

				NewEventID = EventInfo.Event.idEvent;


				/* Create a new merge with the new event */
				Merge.idMerge = -1;
				Merge.idPh = WebParams.idPh;
				Merge.idEvent =  NewEventID;
				strcpy (Merge.szSource, "Merge Processor");

logit ("", "Call ewdb_api_CreateMerge\n");
				if (ewdb_api_CreateMerge (&Merge) != EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_CreateMerge failed.\n");
					html_break ();
					goto shutdown;
				}
logit ("", "ewdb_api_CreateMerge returned %d\n", Merge.idMerge);

				if (Merge.idMerge < 0)
				{
					html_logit ("", "Call to ewdb_api_CreateMerge returned %d.\n", 
													Merge.idMerge);
					html_break ();
					goto shutdown;
				}


logit ("", "Call ewdb_api_UpdateMergePreference for %d, new %d\n",
WebParams.idPh, NewEventID);

				if (ewdb_api_UpdateMergePreference (WebParams.idPh, 
								NewEventID) != EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_UpdateMergePreference failed.\n");
			   		html_break();
					goto shutdown;
				}

			} /* first event to merge */

logit ("", "Calling RebindEvent with Pref %d.\n", Pref);

			if (RebindEvent (NewEventID, &EventInfo, Pref) != EW_SUCCESS)
			{
				html_logit ("", "Call to RebindEvent failed.\n");
		   		html_break();
				goto shutdown;
			}
logit ("", "Call to RebindEvent done.\n");


			/* Delete the merge with the old event */
			if (WebParams.Events[i].idPh > 0)
			{

logit ("", "Calling ewdb_api_DeleteMerge for %d and %d\n",
WebParams.Events[i].idPh, WebParams.Events[i].idEvent);

				if (ewdb_api_DeleteMerge (WebParams.Events[i].idPh, 
									WebParams.Events[i].idEvent) != EWDB_RETURN_SUCCESS)
				{
					html_logit ("", "Call to ewdb_api_DeleteMerge failed.\n");
			   		html_break();
					goto shutdown;
				}


				if (WebParams.Events[i].Auto == FALSE)
				{

					/*
					 * We have to be careful about dealing with 
					 * other phenomena here.  CHeck to see if this
					 * phenomenon has more than one event associated with it.
					 * If not, just delete the phenomenon, otherwise we must
					 * update the phenomenon's preference to the other event.
					 */



					if (ewdb_api_GetMergeList (pMergeBuffer, MERGE_LIST_LEN,
							WebParams.Events[i].idPh, (EWDB_MergeList *) NULL, 
							(EWDB_MergeList *) NULL, 
							&NumFound, &NumRetr) != EWDB_RETURN_SUCCESS)
					{
						html_logit ("", "Call to ewdb_api_GetMergeList failed.\n");
						html_break();
						goto shutdown;
					}

					if (NumFound == 0)
					{

logit ("", "Calling ewdb_api_DeletePhenomenon for %d\n", WebParams.Events[i].idPh);

						if (ewdb_api_DeletePhenomenon (WebParams.Events[i].idPh) 
														!= EWDB_RETURN_SUCCESS)
						{
							logit ("", "Call to ewdb_api_DeletePhenomenon failed.\n");
							html_break();
							goto shutdown;
						}
					}
					else
					{
						if (NumRetr == 0)
						{
							logit ("", "Call to ewdb_api_GetMergeList failed.\n");
							html_break();
							goto shutdown;
						}

						/* Use the first event */
logit ("", "Call ewdb_api_UpdateMergePreference for %d, new %d\n",
WebParams.Events[i].idPh, pMergeBuffer[0].idEvent);

						if (ewdb_api_UpdateMergePreference (WebParams.Events[i].idPh, 
								pMergeBuffer[0].idEvent) != EWDB_RETURN_SUCCESS)
						{
							html_logit ("", "Call to ewdb_api_UpdateMergePreference failed.\n");
			   				html_break();
							goto shutdown;
						}
					}
				}
			} /* If idPh is valid, i.e., this is a located event */
			else
			{
				/* If this is a coincidence event */




			}

			free (EventInfo.pChanInfo);
			EventInfo.iNumAllocChans = 0;
			EventInfo.iNumChans = 0;

			NumMerged = NumMerged + 1;


		} /* is this an accepted merge */

	} /* loop over approved merge events */



	printf ("<PRE><STRONG><CENTER>\n");
	printf ("%d Events merged into new event %d!\n\n", NumMerged, NewEventID);
	printf ("</PRE></STRONG></CENTER>\n");

shutdown:
    ewdb_api_Shutdown();
    html_trailer (WebHost, FooterLogo);
    logit("","merge_process: terminating\n" );

    return (0);
}


int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	WebOpts * pOptions= (WebOpts *) pUserParams;
	int			index;

logit ("", "szVar=<%s>, szVal=<%s>\n", szVar, szVal);

	if(strcmp(szVar,"idPh") == 0)
	{
		pOptions->idPh = atoi(szVal);
	}
	else if(strcmp(szVar,"idPrefEvent") == 0)
	{
		pOptions->idPrefEvent = atoi(szVal);
	}
	else if(strcmp(szVar,"NumEvents") == 0)
	{
		pOptions->NumEvents = atoi(szVal);
	}
	else if(strncmp(szVar,"Evt", 3) == 0)
	{
		/* Extract event id and position, e.g., Evt0 33444000 */
		index = atoi (&(szVar[3]));

		if (index >= MAX_MERGES_PER_EVENT)
		{
			logit ("", "Invalid index, beyond max number of merges\n");
			return (-1);
		}

		pOptions->Events[index].idEvent = atoi (szVal);
	}
	else if(strncmp(szVar,"Ph", 2) == 0)
	{
		/* Extract ph id and position, e.g., Evt0 33444000 */
		index = atoi (&(szVar[2]));

		if (index >= MAX_MERGES_PER_EVENT)
		{
			logit ("", "Invalid index, beyond max number of merges\n");
			return (-1);
		}

		pOptions->Events[index].idPh = atoi (szVal);
	}
	else if(strncmp(szVar,"Auto", 4) == 0)
	{
		/* Extract event id and position, e.g., Auto0 33444000 */
		index = atoi (&(szVar[4]));

		if (index >= MAX_MERGES_PER_EVENT)
		{
			logit ("", "Invalid index, beyond max number of merges\n");
			return (-1);
		}

		pOptions->Events[index].Auto = TRUE;
	}
	else if(strncmp(szVar,"Merge", 5) == 0)
	{
		index = atoi (&(szVar[5]));
		pOptions->Events[index].Merge = TRUE;

		if (index != PrefIndex)
			NumToMerge = NumToMerge + 1;
	}
	else if(strcmp(szVar,"PrefEvt") == 0)
	{
		index = atoi (szVal);
		pOptions->Events[index].Merge = TRUE;
		NumToMerge = NumToMerge + 1;

		PrefIndex = index;
	}
	else
	{
		html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}
	return(0);

}  /* End of SetVars() */




static int		RebindEvent (int NewEventID, 
							EWEventInfoStruct *pEvent, int isPref)
{

	int		j, k, retval;

	if ((NewEventID <= 0) || (pEvent == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_SUCCESS;
	}


	/* bind this merge origin with the new event */
	if (pEvent->GotLocation == TRUE)
	{

logit ("", "Call ewdb_api_BindToEvent for origin %d event %d\n",
pEvent->PrefOrigin.idOrigin, NewEventID);

		if (ewdb_api_BindToEvent (NewEventID, CORE_TABLE_ORIGIN,
				pEvent->PrefOrigin.idOrigin, &retval) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_BindToEvent failed.\n");
			return EW_FAILURE;
		}
	
logit ("", "New bind created %d\n", retval);

		if (isPref == TRUE)
		{
			/* Set as preferred */
logit ("", "Call ewdb_api_SetPrefer for origin %d event %d\n",
pEvent->PrefOrigin.idOrigin, NewEventID);

			if (ewdb_api_SetPrefer (NewEventID, CORE_TABLE_ORIGIN,
					pEvent->PrefOrigin.idOrigin, &retval) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_SetPrefer failed.\n");
				return EW_FAILURE;
			}
	
logit ("", "prefer set %d\n", retval);
		}

		/* bind this merge magnitudes with the new event */
		for (j = 0; j < pEvent->iNumMags; j++)
		{
logit ("", "Call ewdb_api_BindToEvent for mag %d event %d\n",
pEvent->Mags[j].idOrigin, NewEventID);

			if (ewdb_api_BindToEvent (NewEventID, CORE_TABLE_MAGNITUDE,
					pEvent->Mags[j].idMag, &retval) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateEvent failed.\n");
				return EW_FAILURE;
			}

logit ("", "New bind created %d\n", retval);

			if (isPref == TRUE)
			{
				if (j == pEvent->iPrefMag)
				{
	
					/* Set as preferred */
logit ("", "Call ewdb_api_SetPrefer for mag %d event %d\n",
pEvent->Mags[j].idMag, NewEventID);

					if (ewdb_api_SetPrefer (NewEventID, CORE_TABLE_MAGNITUDE,
							pEvent->Mags[j].idMag, &retval) != EWDB_RETURN_SUCCESS)
					{
						html_logit ("", "Call to ewdb_api_SetPrefer failed.\n");
						return EW_FAILURE;
					}
	
logit ("", "prefer set %d\n", retval);
				}
			}
		} /* loop over magnitudes */

	} /* Do we have a location */


	/* bind this merge coincidence, if any, with the new event */
	if (pEvent->GotTrigger == TRUE)
	{

logit ("", "Call ewdb_api_BindToEvent for coincidence %d event %d\n",
pEvent->CoincEvt.idCoincidence, NewEventID);

		if (ewdb_api_BindToEvent (NewEventID, CORE_TABLE_COINCIDENCE, 		
				pEvent->CoincEvt.idCoincidence, &retval) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_BindToEvent failed.\n");
			return EW_FAILURE;
		}

logit ("", "New bind created %d\n", retval);

	} /* do we have a coincidence */



	/* bind the waveforms with the new event */
	for (j = 0; j < pEvent->iNumChans; j++)
	{
		for (k = 0; k < pEvent->pChanInfo[j].iNumWaveforms; k++)
		{
			if (ewdb_api_BindToEvent (NewEventID, CORE_TABLE_WAVEFORMDESC,
					pEvent->pChanInfo[j].Waveforms[k].idWaveform, 
									&retval) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateEvent failed.\n");
				return EW_FAILURE;
			}
		} /* loop over waveforms */

	} /* loop over channels */
				


	return EW_SUCCESS;


}
