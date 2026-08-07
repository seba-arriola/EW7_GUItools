/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    $Id: ewdb_api_GetEventID.c,v 1.3 2005/06/15 19:03:12 davidk Exp $                                                */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_api_GetEventID.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2004/11/02 23:23:50  davidk
 *     Function was using user's pointers for oracle data references, and was not forcing
 *     rebinds in between calls.
 *     Function was not written in the "approved" style.  Rather than try to debug it,
 *     I just copied the values into a template in the approved style.
 *
 *
 ************************************************************/

/* ewdb_api_GetEventID.c */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin Get_Event_ID (OUT_idEvent => :OUT_idEvent, "
  "IN_sSource => :IN_sSource,"
  "IN_sSourceEventID => :IN_sSourceEventID); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,":OUT_idEvent"},
  {0,1,0,0,0,OA_SZ,    ":IN_sSource"},
  {0,1,0,0,0,OA_SZ,    ":IN_sSourceEventID"},
};

#define	NUM_FIELDS	3

static	EWDBid Local_idEvent;
static  char Local_szSource[256], Local_szSourceEventID[256];

/* Insertion Struct for Get_idChan_From_Station_External szStatement */
static EWDB_OCIStatementStruct SSStatement;


/* Prototypes of functions defined in this file */
static int InitGetEventIDStatement(char *szStatement,
                                       EWDB_OCIStatementStruct *pSS);
static int PrepGetEventIDExec(char *IN_szSource, char *IN_szSourceEventID, EWDB_Cursor *ppCursor);
static int PostGetEventIDExec(EWDBid *pidEvent);

int ewdb_api_GetEventID(EWDBid *pidEvent, char *IN_szSource, char *IN_szSourceEventID)
{
  
  EWDB_Cursor pCursor;
  int rc;
  
	if (!pidEvent ||(IN_szSource == NULL) || (IN_szSourceEventID == NULL))
  {
    logit("", "ewdb_api_GetEventID(): ERROR: NULL input params!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  ewdb_base_SetLastOraAPIActionTime();
  
  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
    /* Establishes connection, and performs binding!?! */
  {
    logit("", "ewdb_api_GetEventID(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  if(PrepGetEventIDExec(IN_szSource, IN_szSourceEventID, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetEventID(): PrepGetEventIDExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,
                         "ewdb_api_GetEventID(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction(all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetEventID(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  rc = PostGetEventIDExec(pidEvent);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetEventID(): PostGetEventIDExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetIdChanFromStationExternal() */


static int InitGetEventIDStatement(char *szStatement,
                                   EWDB_OCIStatementStruct *pSS)
{
  
  pSS->FieldArray[0].pVal = &Local_idEvent;
  pSS->FieldArray[1].pVal = Local_szSource;
  pSS->FieldArray[2].pVal = Local_szSourceEventID;
  
  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));

}  /* end InitGetEventIDStatement() */


static int PrepGetEventIDExec(char *IN_szSource, char *IN_szSourceEventID, EWDB_Cursor *ppCursor)
{
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  strncpy(Local_szSource, IN_szSource, sizeof(Local_szSource)-1);
  Local_szSource[sizeof(Local_szSource)-1] = 0x00;

  strncpy(Local_szSourceEventID, IN_szSourceEventID, sizeof(Local_szSourceEventID)-1);
  Local_szSourceEventID[sizeof(Local_szSourceEventID)-1] = 0x00;

  if(InitGetEventIDStatement(SQL_STRING,&SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepGetEventIDExec(): InitGetEventIDStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetEventIDExec() */


static int PostGetEventIDExec(EWDBid *pidEvent)
{
  EWDB_Cursor pCursor;
  
  *pidEvent=Local_idEvent;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);
  
  if((Local_idEvent != -1) && (Local_idEvent <= 0))
  {
    logit("","%s: ERROR!  SQL Proc Get_Event_ID() returned error(%d)!\n",
          "PostGetEventIDExec()", Local_idEvent);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetEventIDExec() */
