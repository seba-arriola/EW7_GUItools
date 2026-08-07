/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateEvent.c,v 1.5 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateEvent.c,v $
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2003/08/11 19:26:00  lucky
 *     Removed superfluous debug stuff
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     cleaned up the function as part of the API cleanup.  Added improved
 *     error loggin, handling, and reporting.
 *
 *     Revision 1.2  2001/05/15 02:16:19  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_Event(OUT_idEvent => :OUT_idEvent,IN_tiEventType => :IN_tiEventType,"
  "IN_iDubiocity => :IN_iDubiocity, IN_sComment => :IN_sComment,IN_sSource => :IN_sSource,"
  "IN_sSourceEventID => :IN_sSourceEventID); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idEvent"},
  {0,1,0,0,0,OA_INT,":IN_tiEventType"},
  {0,1,0,0,0,OA_INT,":IN_iDubiocity"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_SZ,":IN_sSourceEventID"},
};

#define	NUM_FIELDS	6


/* Insertion Struct for CreateEvent szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	EWDB_EventStruct	Local_EventStruct;

static int PrepCreateEventExec (EWDB_EventStruct *pEvent, EWDB_Cursor *ppCursor);
static int PostCreateEventExec (EWDB_EventStruct *pEvent);
static int InitCreateEventStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreateEvent(EWDB_EventStruct *pEvent)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pEvent == NULL)
	{
		logit ("", "ewdb_api_CreateEvent(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_CreateEvent(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateEventExec (pEvent, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateEvent(): PrepCreateEventExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateEvent(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreateEvent(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  rc = PostCreateEventExec(pEvent);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateEvent(): PostCreateEventExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreatePeakAmp() */


static int InitCreateEventStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &(Local_EventStruct.idEvent);
	pSS->FieldArray[1].pVal = &(Local_EventStruct.iEventType);
	pSS->FieldArray[2].pVal = &(Local_EventStruct.iDubiocity);
	pSS->FieldArray[3].pVal = (Local_EventStruct.szComment);
	pSS->FieldArray[4].pVal = (Local_EventStruct.szSource);
	pSS->FieldArray[5].pVal = (Local_EventStruct.szSourceEventID);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepCreateEventExec(EWDB_EventStruct *pEvent, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incoming struct into the local struct */
	memcpy (&Local_EventStruct, pEvent, sizeof (EWDB_EventStruct));

  if(InitCreateEventStatement (SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateEventExec(): InitCreateEventStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateEventExec() */


static int PostCreateEventExec(EWDB_EventStruct *pEvent)
{
  EWDB_Cursor pCursor;
  
  pEvent->idEvent=Local_EventStruct.idEvent;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(pEvent->idEvent <= 0)
  {
    if(pEvent->idEvent == -1)
      logit("","PostCreateEventExec():  SQL Proc Create_Event(%s/%s) returned "
                "\"Unknown error\".  See SQL debug table for details.\n",
            Local_EventStruct.szSource, Local_EventStruct.szSourceEventID);
    else
      logit("","PostCreateEventExec():  SQL Proc Create_Event(%s/%s) returned "
               "the following error(%d).  Please see that proc for details.\n",
            Local_EventStruct.szSource, Local_EventStruct.szSourceEventID,
            pEvent->idEvent);
    return(EWDB_RETURN_WARNING);
  }
  
	return (EWDB_RETURN_SUCCESS);
}  /* end PostCreateEventExec() */

