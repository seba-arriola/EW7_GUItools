/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateEvent.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateEvent.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2005/05/12 20:32:49  mark
 *     Added comments to event struct
 *
 *     Revision 1.2  2004/08/05 18:20:02  michelle
 *     corrected check on update type for dubious constant
 *
 *     Revision 1.1  2001/07/14 07:42:32  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Update_Event(OUT_RetCode => :OUT_RetCode, "
  "  IN_idEvent => :IN_idEvent, IN_iEventType => :IN_iEventType, "
  "  IN_iDubiocity => :IN_iDubiocity, IN_bArchived => :IN_bArchived, "
  "  IN_idPubComment => :IN_idPubComment, IN_idInternalComment => :IN_idInternalComment, "
  "  IN_idContribComment => :IN_idContribComment, "
  "  IN_bSetEventType => :IN_bSetEventType, IN_bSetDubiocity => :IN_bSetDubiocity, "
  "  IN_bSetArchived => :IN_bSetArchived, IN_bSetComment => :IN_bSetComment); End;";


static EWDB_OCI_SFS SQLBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"},
  {0,1,0,0,0,OA_INT,   ":IN_iEventType"},
  {0,1,0,0,0,OA_INT,   ":IN_iDubiocity"},
  {0,1,0,0,0,OA_INT,   ":IN_bArchived"},
  {0,1,0,0,0,OA_EWDBID,":IN_idPubComment"},
  {0,1,0,0,0,OA_EWDBID,":IN_idInternalComment"},
  {0,1,0,0,0,OA_EWDBID,":IN_idContribComment"},
  {0,1,0,0,0,OA_INT,   ":IN_bSetEventType"},
  {0,1,0,0,0,OA_INT,   ":IN_bSetDubiocity"},
  {0,1,0,0,0,OA_INT,   ":IN_bSetArchived"},
  {0,1,0,0,0,OA_INT,   ":IN_bSetComment"}
};

#define	NUM_FIELDS	12

/* Insertion Struct for UpdateEvent szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  Local_iRetCode;
static  int     Local_bSetEventType, Local_bSetDubiocity, Local_bSetArchived, Local_bSetComment;
static  EWDB_EventStruct Local_Event;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepUpdateEventExec(EWDB_EventStruct * pEvent, int IN_iUpdateInfoType,
                        EWDB_Cursor *ppCursor);
static int PostUpdateEventExec();
static int InitUpdateEventStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_UpdateEvent(EWDB_EventStruct * pEvent, int IN_iUpdateInfoType)
{
	EWDB_Cursor pCursor;

  if(!pEvent)
  {
		logit ("", "ewdb_api_UpdateEvent(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
  }

  if(IN_iUpdateInfoType == EWDB_UPDATE_EVENT_NONE)
  {
    /* Nothing for us to update.  Just return a warning. */
    return(EWDB_RETURN_WARNING);
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();


	if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_UpdateEvent(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepUpdateEventExec(pEvent, IN_iUpdateInfoType, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_UpdateEvent():PrepUpdateEventExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_UpdateEvent():ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_UpdateEvent():ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  /* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();

  if(PostUpdateEventExec() != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_UpdateEvent(): PostUpdateEventExec failed!\n");
		return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdateEvent() */


static int InitUpdateEventStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &(Local_Event.idEvent);
	pSS->FieldArray[2].pVal = &(Local_Event.iEventType);
	pSS->FieldArray[3].pVal = &(Local_Event.iDubiocity);
	pSS->FieldArray[4].pVal = &(Local_Event.bArchived);
	pSS->FieldArray[5].pVal = &(Local_Event.idPublicComment);
	pSS->FieldArray[6].pVal = &(Local_Event.idInternalComment);
	pSS->FieldArray[7].pVal = &(Local_Event.idContribMagComment);
	pSS->FieldArray[8].pVal = &Local_bSetEventType;
	pSS->FieldArray[9].pVal = &Local_bSetDubiocity;
	pSS->FieldArray[10].pVal = &Local_bSetArchived;
	pSS->FieldArray[11].pVal = &Local_bSetComment;

  if(ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitUpdateEventStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}  /* end InitUpdateEventStatement() */


static int PrepUpdateEventExec(EWDB_EventStruct * pEvent, int IN_iUpdateInfoType,
                               EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLBindArray;
	SSStatement.RecordSize = 0;

	Local_iRetCode = 0;

  memcpy(&Local_Event, pEvent, sizeof(EWDB_EventStruct));
  if(IN_iUpdateInfoType & EWDB_UPDATE_EVENT_EVENTTYPE)
    Local_bSetEventType = TRUE;
  else
    Local_bSetEventType = FALSE;

  if(IN_iUpdateInfoType & EWDB_UPDATE_EVENT_DUBIOCITY)
    Local_bSetDubiocity = TRUE;
  else
    Local_bSetDubiocity = FALSE;

  if(IN_iUpdateInfoType & EWDB_UPDATE_EVENT_ARCHIVED)
    Local_bSetArchived = TRUE;
  else
    Local_bSetArchived = FALSE;

  if(IN_iUpdateInfoType & EWDB_UPDATE_EVENT_COMMENT)
    Local_bSetComment = TRUE;
  else
    Local_bSetComment = FALSE;

  if(InitUpdateEventStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepUpdateEventExec(): InitUpdateEventStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);

}  /* end PrepUpdateEventExec() */


static int PostUpdateEventExec()
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_iRetCode != 0)
  {
    logit("", "PostUpdateEventExec(): SQL Proc Update_Event(%d) returned error(%d)!\n",
          Local_Event.idEvent, Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PostUpdateEventExec() */

