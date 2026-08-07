/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteEvent.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteEvent.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     cleaned up the function as part of the API cleanup.
 *
 *     Revision 1.2  2001/05/15 02:16:20  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.7  2001/02/21 08:44:04  davidk
 *     removed unneccessary #includes, including one for ewdb_system_support.h
 *     which no longer exists.
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Delete_Event(OUT_RetCode => :OUT_RetCode,IN_idEvent => :IN_idEvent,"
  "IN_bDeleteWaveformDataOnly => :IN_bDeleteWaveformDataOnly); End;";

static EWDB_OCI_SFS SQLBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idEvent"},
  {0,1,0,0,0,OA_INT,":IN_bDeleteWaveformDataOnly"},
};

#define	NUM_FIELDS	3

/* Insertion Struct for DeleteEvent szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  Local_iRetCode;
static	EWDBid	Local_idEvent;
static	int		  Local_bDeleteTraceOnly;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepDeleteEventExec(EWDBid IN_idEvent, int IN_bDeleteTraceOnly,
                         EWDB_Cursor *ppCursor);
static int PostDeleteEventExec();
static int InitDeleteEventStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeleteEvent(int IN_idEvent, int IN_bDeleteTraceOnly)
{
	EWDB_Cursor pCursor;
  int rc;

	if(IN_idEvent <= 0)
	{
		logit ("", "ewdb_api_DeleteEvent(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	/* Set longer timeout -- need this for large events */
	ewdb_base_SetOraConnectionTimeout(15*60);

	if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_DeleteEvent(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepDeleteEventExec(IN_idEvent, IN_bDeleteTraceOnly, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "wdb_api_DeleteEvent(): PrepDeleteEventExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeleteEvent(): ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeleteEvent(): ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  if((rc=PostDeleteEventExec()) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_DeleteEvent(): PostDeleteEventExec failed!\n");
    return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  /* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();

	return(rc);
}  /* end ewdb_api_DeleteEvent() */


static int InitDeleteEventStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idEvent;
	pSS->FieldArray[2].pVal = &Local_bDeleteTraceOnly;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepDeleteEventExec(EWDBid IN_idEvent, int IN_bDeleteTraceOnly,
                               EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLBindArray;
	SSStatement.RecordSize = 0;

	Local_iRetCode         = 0;
	Local_idEvent          = IN_idEvent;
	Local_bDeleteTraceOnly = IN_bDeleteTraceOnly;

	if(InitDeleteEventStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepDeleteEventExec(): InitDeleteEventStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


static int PostDeleteEventExec()
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_iRetCode < 0)
  {
    logit("","PostDeletePeakAmpsBeforeTimeExec(): SQL Proc Delete_PeakAmps_Before_Time() returned error %d.\n",
          Local_iRetCode);
    return(EWDB_RETURN_FAILURE);
  }
  else if(Local_iRetCode > 0)
  {
    return(EWDB_RETURN_WARNING);
  }
  
  return(EWDB_RETURN_SUCCESS);
}
