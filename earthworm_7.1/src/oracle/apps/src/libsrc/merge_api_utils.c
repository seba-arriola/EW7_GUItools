/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: merge_api_utils.c,v 1.3 2002/06/28 21:06:22 lucky Exp $
 *    Revision history:
 *
 *    $Log: merge_api_utils.c,v $
 *    Revision 1.3  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.2  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2002/03/22 20:02:38  lucky
 *    Initial revision
 *
 */



#include <merge.h>
#include <ewdb_apps_utils.h>

#define         INIT_MERGE_LIST_SIZE       50


char  envEW_LOG[MAXPATH];        /* where environment variable EW_LOG
                                        will be stored */
char  DBservice[EQP_MAXWORD];    /* DBMS instance to interact with */
char  DBuser[EQP_MAXWORD];       /* connect to database as */
char  DBpassword[EQP_MAXWORD];   /* Password to datasource */
char  WebHost[MAXPATH];          /* machine running the webserver */
char  BackgroundColor[MAXPATH];
char  HeaderLogo[MAXPATH];
char  FooterLogo[MAXPATH];
char  HeaderTag[MAXPATH];
char  FooterTag[MAXPATH];

int   DEBUG;

/*************************** EWDB_CheckForMerges ***************************/
int		EWDB_RetrieveMerges (EWEventInfoStruct *pEvent, EWDB_CandidateList *pCandidateList, 
			int ListLen, int *NumMerges, EWDB_MergeCriteria *pCrit)
{

	int					i, j, done, ret, found, prefmag;
	int					NumFound, NumRetr, DBFlags;
	EWDB_MergeList		*pMergeList;
	EWDB_MergeList		MinCrit;
	EWDB_MergeList		MaxCrit;
	EWEventInfoStruct	EventInfo;
	

	if ((pEvent == NULL) || (pCandidateList == NULL) || 
							(ListLen <= 0) || (pCrit == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	memset (&MinCrit, 0, sizeof (EWDB_MergeList));
	memset (&MaxCrit, 0, sizeof (EWDB_MergeList));

	/* 
	 * Algorithm: 
	 *  (1) retrieve list of events fitting in a specified criteria-box
	 *  (2) for each event retrieved,
	 *         retrieve the full glory from db
	 *         compare against the current event
	 *         if merge is suggested, see if this phenomenon 
	 *           is already in the list
	 *         if not, add it to the merge list 
	 */

	if ((pMergeList = (EWDB_MergeList *) malloc 
			(INIT_MERGE_LIST_SIZE  * sizeof(EWDB_MergeList))) == NULL)
	{
		logit ("", "Could not allocate space for the merge list.\n");
		return EW_FAILURE;
	}

	if (pCrit->LatDiff > 0.0)
	{
		MinCrit.dLat = pEvent->PrefOrigin.dLat - pCrit->LatDiff;
		MaxCrit.dLat = pEvent->PrefOrigin.dLat + pCrit->LatDiff;
	}

	if (pCrit->LonDiff > 0.0)
	{
		MinCrit.dLon = pEvent->PrefOrigin.dLon - pCrit->LonDiff;
		MaxCrit.dLon = pEvent->PrefOrigin.dLon + pCrit->LonDiff;
	}

	if (pCrit->DepthDiff > 0.0)
	{
		MinCrit.dDepth = pEvent->PrefOrigin.dDepth - pCrit->DepthDiff;
		MaxCrit.dDepth = pEvent->PrefOrigin.dDepth + pCrit->DepthDiff;
	}

	if (pCrit->OriginTimeDiff > 0.0)
	{
		MinCrit.tOrigin = pEvent->PrefOrigin.tOrigin - pCrit->OriginTimeDiff;
		MaxCrit.tOrigin = pEvent->PrefOrigin.tOrigin + pCrit->OriginTimeDiff;
	}

	if (pCrit->MagDiff > 0.0)
	{
		if ((prefmag = pEvent->iPrefMag) >= 0)
		{
			MinCrit.dPrefMag = pEvent->Mags[prefmag].dMagAvg - pCrit->MagDiff;
			MaxCrit.dPrefMag = pEvent->Mags[prefmag].dMagAvg + pCrit->MagDiff;
		}
	}



	if (ewdb_api_GetMergeList (pMergeList, INIT_MERGE_LIST_SIZE, -1,
					&MinCrit, &MaxCrit, &NumFound, &NumRetr) != EWDB_RETURN_SUCCESS) 
	{
		logit ("", "Call to ewdb_api_GetMergeList failed.\n");
		return EW_FAILURE;
	}
				
	if (NumRetr < NumFound)
	{
		/* reallocate and retry */
		logit ("", "Reallocating merge list: %d-%d\n", NumRetr, NumFound);

		free (pMergeList);

		if ((pMergeList = (EWDB_MergeList *) malloc 
			(NumFound  * sizeof(EWDB_MergeList))) == NULL)
		{
			logit ("", "Could not allocate space for the merge list.\n");
			return EW_FAILURE;
		}

		if (ewdb_api_GetMergeList (pMergeList, NumRetr, -1,
					&MinCrit, &MaxCrit, &NumFound, &NumRetr) != EWDB_RETURN_SUCCESS) 
		{
			logit ("", "Call to ewdb_api_GetMergeList failed.\n");
			return EW_FAILURE;
		}
	}

#ifdef DO_DEBUG
logit ("", "Retrieved %d potential merge candidates\n", NumRetr);
logit ("", "This event: %f, %f, %f, %f\n",
pEvent->PrefOrigin.tOrigin,
pEvent->PrefOrigin.dLat,
pEvent->PrefOrigin.dLon,
pEvent->PrefOrigin.dDepth);

for (i = 0; i < NumRetr; i++)
{
 logit ("", "%d: %d %d %d, %f, %f, %f, %f \n", i,
pMergeList[i].idEvent, 
pMergeList[i].idMerge, 
pMergeList[i].idPh, 
pMergeList[i].tOrigin, 
pMergeList[i].dLat, 
pMergeList[i].dLon, 
pMergeList[i].dDepth);
}
#endif DO_DEBUG

	*NumMerges = 0;

	for (i = 0; i < NumRetr; i++)
	{
		/* get this event's full glory */

		if (InitEWEvent (&EventInfo) != EW_SUCCESS)
		{
			logit ("", "Call to InitEWEvent failed.\n");
			return EW_FAILURE;
		}


		DBFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
					| GETEWEVENTINFO_STAMAGS | GETEWEVENTINFO_COMPINFO |
					GETEWEVENTINFO_WAVEFORM_DESCS;


		if (ewdb_apps_GetDBEventInfoII (&EventInfo,
				pMergeList[i].idEvent, -1, DBFlags) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_apps_GetDBEventInfoII failed for %d.\n", 
								pMergeList[i].idEvent);
			return EW_FAILURE;
		}


		/* compare the two events */
		ret = MergeDeterminator (pEvent, &EventInfo);
		if (ret < 0)
		{
			logit ("", "Call to MergeDeterminator failed for %d.\n",
							EventInfo.Event.idEvent);
			return EW_FAILURE;
		}

		if (ret == 1)
		{
			/* Event belongs to an phenomenon -- find it in the list */

			found = FALSE;
			for (j = 0; ((j < *NumMerges) && (found == FALSE)); j++)
			{
				if (pCandidateList[j].idPh == pMergeList[i].idPh)
					found = TRUE;
			}

			if (found == FALSE)
			{
				pCandidateList[*NumMerges].idPh = pMergeList[i].idPh;
				pCandidateList[*NumMerges].idPrefEvent = pMergeList[i].idPrefEvent;

				if ((*NumMerges = *NumMerges + 1) >= ListLen)
				{
					logit ("", "Max number of merges (%d) reached. \n", *NumMerges);
					free (EventInfo.pChanInfo);
					free (pMergeList);
					return EW_SUCCESS;
				}
			}
		}


		free (EventInfo.pChanInfo);

	} /* for loop over retrieved candidate events */
	
	free (pMergeList);

	return EW_SUCCESS;

}


/*************************** MergeDeterminator ***************************/
static	int		MergeDeterminator (EWEventInfoStruct *pEvt1, 
										EWEventInfoStruct *pEvt2)
{

	int					i, j, done;
	int					NumFound, NumRetr, DBFlags;


	if ((pEvt1 == NULL) || (pEvt2 == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return -1;
	}

	return 1;

}



/*************************** EWDB_CheckMergePreference ***************************/
int		EWDB_CheckMergePreference (EWEventInfoStruct *pEvent, 
										EWDBid idPrefEvent, int *Preference)
{

	EWEventInfoStruct	PrefEvent;
	int					i, j, done, ret, found, prefmag;
	int					NumFound, NumRetr, DBFlags;
	EWDB_MergeList		*pMergeList;
	EWDB_MergeList		MinCrit;
	EWDB_MergeList		MaxCrit;


	*Preference = 0;

	if ((pEvent == NULL) || (idPrefEvent <= 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	/* get the event that is current preferred */
	if (InitEWEvent (&PrefEvent) != EW_SUCCESS)
	{
		logit ("", "Call to InitEWEvent failed.\n");
		return EW_FAILURE;
	}


    DBFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
                    | GETEWEVENTINFO_STAMAGS | GETEWEVENTINFO_COMPINFO |
                    GETEWEVENTINFO_WAVEFORM_DESCS;

	if (ewdb_apps_GetDBEventInfoII (&PrefEvent, 
				idPrefEvent, -1, DBFlags) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
		free (PrefEvent.pChanInfo);
		return EW_FAILURE;
	}


	/* 
	 * Insert here some code to determine whether one 
	 * event is preferred over the other....
	 */

/*
	*Preference = 1;
*/
	*Preference = -1;


	free (PrefEvent.pChanInfo);

	return EW_SUCCESS;

}



/**************************************************************************/
/**************************************************************************/
int		EWDB_ConsolidateMergeList (EWDB_MergeList *pMergeList, int MergeLen,
						EWDB_PhenomenaMergeList *pConsMerge, int *NumCons)
{

	int 	i, j, found, index;

	if ((pMergeList == NULL) || (pConsMerge == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	for (i = 0; i < MergeLen; i++)
	{
		pConsMerge[i].NumMerges = 0;
	}

	*NumCons = 0;

	for (i = 0; i < MergeLen; i++)
	{

		found = FALSE;
		for (j = 0; j < *NumCons; j++)
		{
			if (pConsMerge[j].idPh == pMergeList[i].idPh)
			{
				pConsMerge[j].NumMerges = pConsMerge[j].NumMerges + 1;
				found = TRUE;

				if (pMergeList[i].idEvent == pMergeList[i].idPrefEvent)
				{
					/* This is the "authoritative" location */
					pConsMerge[j].idPh = pMergeList[i].idPh;
					pConsMerge[j].PrefEvt.Event.idEvent = pMergeList[i].idPrefEvent;
					pConsMerge[j].PrefEvt.dOT = pMergeList[i].tOrigin;
					pConsMerge[j].PrefEvt.dLat = pMergeList[i].dLat;
					pConsMerge[j].PrefEvt.dLon = pMergeList[i].dLon;
					pConsMerge[j].PrefEvt.dDepth = pMergeList[i].dDepth;
					pConsMerge[j].PrefEvt.dPrefMag = pMergeList[i].dPrefMag;
					pConsMerge[j].PrefEvt.idOrigin = -1;
	
					strcpy (pConsMerge[j].szSource, pMergeList[i].szSource);
					strcpy (pConsMerge[j].szHumanReadable, 
													pMergeList[i].szHumanReadable);
				}
			}
		}


		if (found == FALSE)
		{
			pConsMerge[*NumCons].idPh = pMergeList[i].idPh;
			pConsMerge[*NumCons].PrefEvt.Event.idEvent = pMergeList[i].idPrefEvent;
					
			if (pMergeList[i].idEvent == pMergeList[i].idPrefEvent)
			{
				/* This is the "authoritative" location */
				pConsMerge[*NumCons].PrefEvt.dOT = pMergeList[i].tOrigin;
				pConsMerge[*NumCons].PrefEvt.dLat = pMergeList[i].dLat;
				pConsMerge[*NumCons].PrefEvt.dLon = pMergeList[i].dLon;
				pConsMerge[*NumCons].PrefEvt.dDepth = pMergeList[i].dDepth;
				pConsMerge[*NumCons].PrefEvt.dPrefMag = pMergeList[i].dPrefMag;
				pConsMerge[*NumCons].PrefEvt.idOrigin = -1;

				strcpy (pConsMerge[*NumCons].szSource, pMergeList[i].szSource);
				strcpy (pConsMerge[*NumCons].szHumanReadable, 
												pMergeList[i].szHumanReadable);
			}

			pConsMerge[*NumCons].NumMerges = pConsMerge[*NumCons].NumMerges + 1;
			*NumCons = *NumCons + 1;
		}
	}

	return EW_SUCCESS;

}
