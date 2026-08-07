
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_PutCoincidenceInfo.c,v 1.1 2002/05/28 17:25:41 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_PutCoincidenceInfo.c,v $
 *     Revision 1.1  2002/05/28 17:25:41  lucky
 *     Initial revision
 *
 *
 */


#include <string.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>


int ewdb_apps_PutCoincidenceInfo (EWCoincEvtStruct *pCoinc)
{

	int							i;
	EWDB_External_StationStruct	ExtStation;

	if (pCoinc == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	memset (&ExtStation, 0, sizeof(EWDB_External_StationStruct));


    /*
     * Extract author: story - we need the idEvent in order
     * to insert this coincidence event. We must rely on the external
     * source/author designations and hope that the DB will
     * map those into the same idEvent as before when we call CreateEvent.
     * The triglist message contains the logo as the author string
     * of the form xxxxxxxxx:xxxxxxxxx - we want the first part
     * of that. The message also tells us the external event id.
     */

    if (pCoinc->sAuthor[9] == ':')
    {
        strncpy (pCoinc->Event.szSource, pCoinc->sAuthor, 9);
        pCoinc->Event.szSource[9] = '\0';
    }
    else
    {
        logit ("e", "Author string not a logo <%s>; will use it anyway.\n",
															pCoinc->sAuthor);
        strcpy (pCoinc->Event.szSource, pCoinc->sAuthor);
    }
    strcpy (pCoinc->Event.szSourceEventID, pCoinc->sEventID); 


	/* Insert the Event */
	pCoinc->Event.iEventType = EWDB_EVENT_TYPE_COINCIDENCE;

	if (ewdb_api_CreateEvent (&pCoinc->Event) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_CreateEvent failed.\n");
		return EWDB_RETURN_FAILURE;
	}


	/* Make sure that we inserted the Event correctly */
	if (pCoinc->Event.idEvent < 0)
	{
		logit ("", "Call to ewdb_api_CreateEvent failed: %d\n", 
										pCoinc->Event.idEvent);
		return EWDB_RETURN_FAILURE;
	}

	/* Insert the Coincidence */

	pCoinc->CoincEvt.idEvent = pCoinc->Event.idEvent;
	pCoinc->CoincEvt.bBindToEvent = TRUE;
	strcpy (pCoinc->CoincEvt.szSource, pCoinc->Event.szSource);

	if (ewdb_api_CreateCoincidence (&pCoinc->CoincEvt) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_CreateCoincidence failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	/* Make sure that we inserted the Coincidence correctly */
	if (pCoinc->CoincEvt.idCoincidence < 0)
	{
		logit ("", "Call to ewdb_api_CreateCoincidence failed: %d\n", 
				pCoinc->CoincEvt.idCoincidence);
		return EWDB_RETURN_FAILURE;
	}
logit ("", "Created Coincidence %d\n", pCoinc->CoincEvt.idCoincidence);


	/* insert the individual trigger information */
	for (i = 0; i < pCoinc->iNumTrigs; i++)
	{

		memset (&ExtStation, 0, sizeof (EWDB_External_StationStruct));
		strcpy (ExtStation.Station.Sta, pCoinc->pChanTrigs[i].Station.Sta);
		strcpy (ExtStation.Station.Comp, pCoinc->pChanTrigs[i].Station.Comp);
		strcpy (ExtStation.Station.Net, pCoinc->pChanTrigs[i].Station.Net);

		if (ewdb_apps_RetrieveIdChanExternal (&ExtStation) 
														!= EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_apps_RetrieveIdChanExternal failed.\n");
			return EWDB_RETURN_FAILURE;
		}

		pCoinc->pChanTrigs[i].Station.idChan = ExtStation.Station.idChan;

		pCoinc->pChanTrigs[i].Trigger.idCoincidence = 
							pCoinc->CoincEvt.idCoincidence;
		pCoinc->pChanTrigs[i].Trigger.idChan = 
							pCoinc->pChanTrigs[i].Station.idChan;

		if (ewdb_api_CreateTrigger (&pCoinc->pChanTrigs[i].Trigger) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateTrigger failed.\n");
			return EWDB_RETURN_FAILURE;
		}

	} /* loop over channels */

	return EWDB_RETURN_SUCCESS;

}  /* end ewdb_apps_PutCoincidenceInfo() */

