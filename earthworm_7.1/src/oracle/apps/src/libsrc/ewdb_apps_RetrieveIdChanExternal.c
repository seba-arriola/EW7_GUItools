/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_RetrieveIdChanExternal.c,v 1.5 2004/05/21 22:48:51 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_RetrieveIdChanExternal.c,v $
 *     Revision 1.5  2004/05/21 22:48:51  davidk
 *     Changed ewdb_apps_RetrieveIdChanExternal() to return WARNING when it
 *     can't find a given station in the DB.
 *     Modified ewdb_apps_PutDBEventInfo() to handle the warning in a non-catastrophic
 *     manner, and issue a logit() warning.
 *
 *     Revision 1.4  2004/05/12 20:24:24  michelle
 *     changed call to CreateOrAlterExternalStation to be SelectExternalStation
 *     since IF the station does not exist we DO NOT want to create a new station
 *     at zero, zero (lat,lon) which CreateOrAlter does, just look for station
 *     comments as such added to file also.
 *
 *     Revision 1.3  2001/07/20 17:40:05  lucky
 *     State of the code after much of v6.0 testing and debugging
 *
 *     Revision 1.2  2001/07/01 21:55:22  davidk
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
 *     Revision 1.1  2001/05/15 02:15:25  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>

/********************************************************************
********************************************************************/
int ewdb_apps_RetrieveIdChanExternal(EWDB_External_StationStruct *pStation) 
/* This function provides a convenient way to retrieve an idChan from an 
   Earthworm SCN.  There is nothing official or exclusive about it.  It is
   merely a convenience function that calls two others. */
{

  int rc;

	if (pStation == NULL) 
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	/*************************
	 *   Create_SCN_External    *
	 *************************/
        /* this creates if doesn't exist, but it creates with lat lon at 0, 0 */
	/* if (ewdb_api_CreateOrAlterExternalStation (pStation) != EWDB_RETURN_SUCCESS) */
        /** so don't want to create if it doesn't exist as this was creating a new station at 0, 0 
            so don't use CreateOrAlterExternalStation (which inserts into Station_External
            and causes a trigger to fire to insert new core comp and chan with zero lat lon) instead
            use SelectExternalStation to do just and only that, select 
        ***/
  rc = ewdb_api_SelectExternalStation(pStation);
	if (rc != EWDB_RETURN_SUCCESS)
	{
    if(rc == EWDB_RETURN_WARNING)
    {
      /* the SCNL was not found in the db.  Return warning. */
      return(rc);
    }
    else
    {
      logit ("", "ewdb_apps_RetrieveIdChanExternal() Error: ewdb_api_SelectExternalStation(%s %s %s %s) failed!\n",
             pStation->Station.Sta, pStation->Station.Comp, pStation->Station.Net, pStation->Station.Loc);
      return(EWDB_RETURN_FAILURE);
    }
	}

	if (pStation->StationID <= 0)
	{
		logit ("", "ewdb_api_CreateOrAlterExternalStation returned ERROR: StationID=%d!\n",
                                pStation->StationID);
		return EWDB_RETURN_FAILURE;
	}


	/*************************
	 * Get_idChan_From_SCN   *
	 *************************/
	if (ewdb_api_GetIdChanFromStationExternal (&(pStation->Station.idChan), 
                                             pStation->StationID)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_GetIdChanFromStationExternal failed!\n");
		return EWDB_RETURN_FAILURE;
	}


	if (pStation->Station.idChan <= 0)
	{
		logit ("", "ewdb_api_GetIdChanFromStationExternal returned ERROR: idChan=%d!\n",
                                pStation->Station.idChan);
		return EWDB_RETURN_FAILURE;
	}


	return EWDB_RETURN_SUCCESS;

}  /* end ewdb_apps_RetrieveIdChanExternal() */

