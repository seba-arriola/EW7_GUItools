/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetEventInfo.c,v 1.6 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetEventInfo.c,v $
 *     Revision 1.6  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2005/05/12 20:31:31  mark
 *     Added comments to event struct
 *
 *     Revision 1.4  2001/07/14 07:40:51  davidk
 *     Added support for bArchived flag.
 *
 *     Revision 1.2  2001/05/15 02:16:20  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.6  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.5  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.4  2000/05/12 23:50:45  davidk
 *     Added prototypes for the functions implemented in this file.
 *
 *     Revision 1.3  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_Event_Info(OUT_Retcode => :OUT_Retcode, IN_idEvent => :IN_idEvent, "
        " OUT_tiEventType => :OUT_tiEventType, "
        " OUT_iDubiocity => :OUT_iDubiocity, OUT_bArchived => :OUT_bArchived, "
        " OUT_sComment => :OUT_sComment, OUT_idComment => :OUT_idComment, "
		" OUT_idInternalComment => :OUT_idInternalComment, OUT_idContribComment => :OUT_idContribComment); "
		"End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"},
  {0,1,0,0,0,OA_INT,":OUT_tiEventType"},
  {0,1,0,0,0,OA_INT,":OUT_iDubiocity"},
  {0,1,0,0,0,OA_INT,":OUT_bArchived"},
  {0,1,0,0,0,OA_SZ,":OUT_sComment"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idComment"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idInternalComment"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idContribComment"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 9

/* Insertion Struct for GetEventInfo statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_EventStruct Local_Event;
static char Local_szComment[4008];

static int Local_iRetCode;

static int PrepGetEventInfoExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor);
static int PostGetEventInfoExec(EWDB_EventStruct * pEvent);
static int InitGetEventInfoStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);

/**********************************************************************
**********************************************************************/
int ewdb_api_GetEventInfo(EWDBid IN_idEvent, EWDB_EventStruct * pEvent)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idEvent <= 0 || !pEvent)
  {
    logit("","ewdb_api_GetEventInfo(): Invalid params passed in:\n"
             "IN_idEvent %d, pEvent %u\n",
          IN_idEvent, pEvent);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetEventInfoExec(IN_idEvent,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetEventInfo(): PrepGetEventInfoExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetEventInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetEventInfo(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( PostGetEventInfoExec(pEvent) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetEventInfo(): PostGetEventInfoExec failed! SQL Proc returned %d\n",
           "ewdb_api_GetEventInfo()", Local_iRetCode);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetEventInfo() */


static int InitGetEventInfoStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &(Local_Event.idEvent);
	pSS->FieldArray[2].pVal = &(Local_Event.iEventType);
	pSS->FieldArray[3].pVal = &(Local_Event.iDubiocity);
	pSS->FieldArray[4].pVal = &(Local_Event.bArchived);
	pSS->FieldArray[5].pVal = Local_szComment;
	pSS->FieldArray[6].pVal = &(Local_Event.idPublicComment);
	pSS->FieldArray[7].pVal = &(Local_Event.idInternalComment);
	pSS->FieldArray[8].pVal = &(Local_Event.idContribMagComment);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetEventInfoStatement() */


static int PrepGetEventInfoExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor)
{
  /* initialize output buffers */
  memset(&Local_Event, 0, sizeof(Local_Event));
  memset(Local_szComment, 0, sizeof(Local_szComment));

  /* copy user's data over to local buffers */
  Local_Event.idEvent=IN_idEvent;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetEventInfoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetEventInfoExec(EWDB_EventStruct * pEvent)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(!Local_iRetCode)
  {
    /* Copy any comment strings into our local event struct; since the event's comment
	 * space is smaller than what's in the comment table, make sure it fits without
	 * overflowing anything.
	 */
    strncpy(Local_Event.szComment, Local_szComment, sizeof(Local_Event.szComment)-1);
	  Local_Event.szComment[sizeof(Local_Event.szComment)-1] = 0;

    /* Copy the results to the users struct */
    memcpy(pEvent,&Local_Event,sizeof(EWDB_EventStruct));

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    return(Local_iRetCode);
  }
}   /* End PostXXX() */


