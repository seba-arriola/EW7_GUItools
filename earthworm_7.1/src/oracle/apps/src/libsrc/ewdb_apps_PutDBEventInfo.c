
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_PutDBEventInfo.c,v 1.16 2005/05/05 19:17:09 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_PutDBEventInfo.c,v $
 *     Revision 1.16  2005/05/05 19:17:09  davidk
 *     Fixed a bug in function where OriginStruct.sSource was being used instead
 *     of OriginStruct.sRealSource for the author string for the origin.
 *
 *     Revision 1.15  2004/11/16 05:17:52  davidk
 *     Removed  ewdb_apps_PutDBEventInfo_DBPicks(); this function is only used within
 *     hydra, and the function already exists in the hydra source tree.
 *     Prototype was removed from ewdb_apps_utils.h in 08/2003.
 *
 *     Revision 1.14  2004/05/21 22:48:51  davidk
 *     Changed ewdb_apps_RetrieveIdChanExternal() to return WARNING when it
 *     can't find a given station in the DB.
 *     Modified ewdb_apps_PutDBEventInfo() to handle the warning in a non-catastrophic
 *     manner, and issue a logit() warning.
 *
 *     Revision 1.13  2003/07/01 17:39:52  lucky
 *     Bug fixes on NEIS v1.0
 *
 *     Revision 1.12  2003/03/11 17:15:13  lucky
 *     *** empty log message ***
 *
 *     Revision 1.11  2002/10/29 18:48:47  lucky
 *     Added origin version number tracking to hypoinverse inserter
 *
 *     Revision 1.10  2002/09/10 17:17:04  lucky
 *     Stable scaffold
 *
 *     Revision 1.9  2002/06/28 21:06:22  lucky
 *     Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *     in connection with the GIOC scaffold.
 *
 *     Revision 1.8  2002/05/28 17:25:41  lucky
 *     *** empty log message ***
 *
 *     Revision 1.7  2001/07/20 17:40:05  lucky
 *     State of the code after much of v6.0 testing and debugging
 *
 *     Revision 1.6  2001/07/01 21:55:22  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.5  2001/06/26 19:56:17  lucky
 *     Added and deleted debug stuff
 *
 *     Revision 1.4  2001/06/21 21:37:20  lucky
 *     Fixed magtypes for Local Magnitudes
 *
 *     Revision 1.3  2001/06/21 21:25:00  lucky
 *     State of the code after LocalMag review was implemented and partially tested.
 *
 *     Revision 1.2  2001/06/06 20:54:46  lucky
 *     Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *     in progress - checkin in for the sake of safety.
 *
 *     Revision 1.1  2001/05/15 02:15:27  davidk
 *     Initial revision
 *
 *     Revision 1.8  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.7  2000/11/01 18:39:06  lucky
 *     *** empty log message ***
 *
 *     Revision 1.6  2000/09/07 21:16:21  lucky
 *     Final version after the Review pages were demonstrated.
 *
 *     Revision 1.5  2000/08/28 15:36:54  lucky
 *     Added a new parameter NewEvent. When set to 1, we call CreateEvent, otherwise
 *     we just run with the idEvent passed to us in the pEventInfo structure.
 *
 *     Revision 1.4  2000/08/25 18:19:17  lucky
 *     Implemented new (explicitly allocated) pChanInfo, and multiple types of Arrivals.
 *
 *     Revision 1.2  2000/06/13 18:55:51  lucky
 *     Cleaned up debugging statements.
 *
 *     Revision 1.1  2000/06/07 22:15:36  lucky
 *     Initial revision
 *
 *
 */


#include <string.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>


int ewdb_apps_PutDBEventInfo(EWEventInfoStruct *pEventInfo, 
                                     char *author,
                                     char *szEventID, int NewEvent)
{

	int							i, j;
	int							idML, idMd;
	EWDB_External_StationStruct Station;
	int             			rc = EWDB_RETURN_SUCCESS;
  int iRetCode;


	if ((pEventInfo == NULL) || (author == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	memset (&Station, 0, sizeof(EWDB_External_StationStruct));
	idML = -1;
	idMd = -1;

	/* Set the author and external event id strings */
	strncpy (pEventInfo->Event.szSource, author,
						(sizeof(pEventInfo->Event.szSource) - 1));


	if (szEventID != NULL)
	{
		strncpy (pEventInfo->Event.szSourceEventID, szEventID,
					(sizeof (pEventInfo->Event.szSourceEventID) - 1));
	}
	else
	{
		sprintf (pEventInfo->Event.szSourceEventID, "%d",
						pEventInfo->Event.idEvent);
	}


	if (NewEvent == 1)
	{
		if (ewdb_api_CreateEvent (&pEventInfo->Event) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateEvent failed.\n");
			return EWDB_RETURN_FAILURE;
		}


		/* Make sure that we inserted the Event correctly */
		if (pEventInfo->Event.idEvent < 0)
		{
			logit ("", "Call to ewdb_api_CreateEvent failed: %d\n", 
					pEventInfo->Event.idEvent);
			return EWDB_RETURN_FAILURE;
		}
	}


	/* Insert the Origin */

	pEventInfo->PrefOrigin.idEvent = pEventInfo->Event.idEvent;

	strcpy (pEventInfo->PrefOrigin.sRealSource, pEventInfo->Event.szSource);
	strcpy (pEventInfo->PrefOrigin.szSourceEventID, pEventInfo->Event.szSourceEventID);


	if (ewdb_api_CreateOrigin (&pEventInfo->PrefOrigin) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_CreateOrigin failed.\n");
		return EWDB_RETURN_FAILURE;
	}


	/* Make sure that we inserted the Origin correctly */
	if (pEventInfo->PrefOrigin.idOrigin < 0)
	{
		logit ("", "Call to ewdb_api_CreateOrigin failed: %d\n", 
				pEventInfo->PrefOrigin.idOrigin);
		return EWDB_RETURN_FAILURE;
	}

	/* Insert the Magnitudes */

	for (i = 0; i < pEventInfo->iNumMags; i++)
	{

		if (i == pEventInfo->iPrefMag)
		{
			pEventInfo->Mags[i].bSetPreferred = TRUE;
		}
		else
			pEventInfo->Mags[i].bSetPreferred = FALSE;

		strcpy (pEventInfo->Mags[i].szSource, pEventInfo->Event.szSource);
		pEventInfo->Mags[i].idEvent = pEventInfo->Event.idEvent;
		pEventInfo->Mags[i].idOrigin = pEventInfo->PrefOrigin.idOrigin;

		if (ewdb_api_CreateMagnitude (&pEventInfo->Mags[i]) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateMagnitude failed for Mag %d.\n", i);
			return EWDB_RETURN_FAILURE;
		}

		/* Make sure that we inserted the Magnitude correctly */
		if (pEventInfo->Mags[i].idMag < 0)
		{
			logit ("", "Call to ewdb_api_CreateMagnitude failed: %d\n", pEventInfo->Mags[i].idMag);
			return EWDB_RETURN_FAILURE;
		}

		if (pEventInfo->Mags[i].iMagType == MAGTYPE_DURATION)
			idMd = pEventInfo->Mags[i].idMag;
		else if ((pEventInfo->Mags[i].iMagType == MAGTYPE_LOCAL_PEAK2PEAK) || 
		          (pEventInfo->Mags[i].iMagType == MAGTYPE_LOCAL_ZERO2PEAK))
			idML = pEventInfo->Mags[i].idMag;
		else
		{
			logit ("", "Mag %d: Unknown mag type %d\n", i, pEventInfo->Mags[i].iMagType);
		}
	}

	/* insert the arrival information */
	for (i = 0; i < pEventInfo->iNumChans; i++)
	{
		strncpy (Station.Station.Sta, pEventInfo->pChanInfo[i].Station.Sta,
							(sizeof (Station.Station.Sta) - 1));
		strncpy (Station.Station.Comp, pEventInfo->pChanInfo[i].Station.Comp,
							(sizeof (Station.Station.Comp) - 1));
		strncpy (Station.Station.Net, pEventInfo->pChanInfo[i].Station.Net,
							(sizeof (Station.Station.Net) - 1));


    /********   DK CLEANUP, we can't die just because one channel insertion failed ***************/
		if ((iRetCode = ewdb_apps_RetrieveIdChanExternal (&Station)) != EWDB_RETURN_SUCCESS)
		{
      if(iRetCode == EWDB_RETURN_WARNING)
      {
        /* Per Alex,
           We changed mode of operation.  We are now ignoring data from
           channels that have not been loaded into the database ahead of time.
           ewdb_apps_RetrieveIdChanExternal() now returns warning when a channel is not found.
           05/21/2004 DK
         **********************************************************/

        logit ("", "ewdb_apps_PutDBEventInfo():  WARNING: SCNL (%s,%s,%s,%s) not found in DB!\n",
          Station.Station.Sta, Station.Station.Comp, 
          Station.Station.Net, Station.Station.Loc);
        continue;
      }
      else
      {
        logit ("", "Call to ewdb_apps_RetrieveIdChanExternal failed for(%s %s %s %s).\n",
               Station.Station.Sta, Station.Station.Comp, 
               Station.Station.Net, Station.Station.Loc);
        return(EWDB_RETURN_FAILURE);
      }
		}

		for (j = 0; j < pEventInfo->pChanInfo[i].iNumArrivals; j++)
		{
			pEventInfo->pChanInfo[i].Arrivals[j].idChan = Station.Station.idChan;
			pEventInfo->pChanInfo[i].Arrivals[j].idOrigin = 
								pEventInfo->PrefOrigin.idOrigin;

			if (ewdb_api_CreateArrival (&pEventInfo->pChanInfo[i].Arrivals[j]) 
												!= EWDB_RETURN_SUCCESS)
			{
				logit ("", "Call to ewdb_api_CreateArrival failed. Arrival %d\n", j);
				rc = EWDB_RETURN_WARNING;
			}
		}


		for (j = 0; j < pEventInfo->pChanInfo[i].iNumStaMags; j++)
		{
			pEventInfo->pChanInfo[i].Stamags[j].idChan = Station.Station.idChan;

			if (pEventInfo->pChanInfo[i].Stamags[j].iMagType == MAGTYPE_DURATION)
			{
				pEventInfo->pChanInfo[i].Stamags[j].idMagnitude = idMd;

				pEventInfo->pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.idPick =
							pEventInfo->pChanInfo[i].Arrivals[j].idPick;

				if (ewdb_api_InsertCodaWithMag (&pEventInfo->pChanInfo[i].Stamags[j]) 
														!= EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_InsertCodaWithMag failed.  Arrival %d\n", j);
					rc=EWDB_RETURN_WARNING;
	
				}
			}
			else if
			 ((pEventInfo->pChanInfo[i].Stamags[j].iMagType == MAGTYPE_LOCAL_PEAK2PEAK) || 
			  (pEventInfo->pChanInfo[i].Stamags[j].iMagType == MAGTYPE_LOCAL_ZERO2PEAK))
			{
				pEventInfo->pChanInfo[i].Stamags[j].idMagnitude = idML;

				if (ewdb_apps_InsertPeakAmpWithMag (&pEventInfo->pChanInfo[i].Stamags[j]) 
														!= EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_apps_InsertPeakAmpWithMag failed.  Arrival %d\n", j);
					rc=EWDB_RETURN_WARNING;
	
				}
			}
			else
			{
				logit ("", "Chan %d, Stamag %d: unknown mag type %d\n", i, j,
								pEventInfo->pChanInfo[i].Stamags[j].iMagType);
			}
	

		} /* Loop over stamags */

	} /* loop over channels */

	return(rc);

}  /* end ewdb_api_PutDBEventInfo() */






