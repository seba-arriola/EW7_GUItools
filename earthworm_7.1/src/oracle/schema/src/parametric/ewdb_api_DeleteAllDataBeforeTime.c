/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteAllDataBeforeTime.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteAllDataBeforeTime.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2004/09/09 17:22:27  davidk
 *     Re-Updated for reaper v2.
 *     Now includes a pSQLRetCode, for more precise result description.
 *
 *     Revision 1.2  2004/09/09 05:48:56  davidk
 *     Updated for reaper v2.
 *     Now includes a Local_iMaxRecsToDelete
 *     (so the caller can delete piecemeal for faster execution).
 *     Automatically forces.
 *
 *     Revision 1.1  2001/07/14 07:40:13  davidk
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin :OUT_RetCode := "
  "  Delete_Data_Before_Time(IN_tTime => :IN_tTime, "
  "                              IN_iMaxRecsToDelete => :IN_iMaxRecsToDelete); End;";

static EWDB_OCI_SFS SQLBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_tTime"},
  {0,1,0,0,0,OA_INT,":IN_iMaxRecsToDelete"}
};

#define	NUM_FIELDS	3

/* Insertion Struct for DeleteDataBeforeTime szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  Local_iRetCode;
static	int     Local_tTime;
static	int     Local_iMaxRecsToDelete;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepDeleteDataBeforeTimeExec(EWDBid IN_tTime, int IN_iMaxRecsToDelete, 
                                            EWDB_Cursor *ppCursor);
static int PostDeleteDataBeforeTimeExec(int * pRetCode);
static int InitDeleteDataBeforeTimeStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeleteAllDataBeforeTime(int IN_tTime, int IN_iMaxRecsToDelete, int * pSQLRetCode)
{

	EWDB_Cursor pCursor;
  int rc;

	if(IN_tTime < 0 || pSQLRetCode == NULL)
	{
		logit("", "ewdb_api_DeleteAllDataBeforeTime(): Invalid parameters passed in(%d %u)!\n",
          IN_tTime, pSQLRetCode);
		return EWDB_RETURN_FAILURE;
	}

	/* Set longer timeout -- need this for large events */
	ewdb_base_SetOraConnectionTimeout(15*60);

	if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_DeleteAllDataBeforeTime(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepDeleteDataBeforeTimeExec(IN_tTime, IN_iMaxRecsToDelete, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_DeleteAllDataBeforeTime(): PrepDeleteDataBeforeTimeExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeleteAllDataBeforeTime(): ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeleteAllDataBeforeTime(): ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  if((rc = PostDeleteDataBeforeTimeExec(pSQLRetCode)) == EWDB_RETURN_FAILURE)
    logit ("", "ewdb_api_DeleteAllDataBeforeTime(): PostDeleteDataBeforeTimeExec failed!\n");

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();

	return(rc);
}  /* end ewdb_api_DeleteAllDataBeforeTime() */


static int InitDeleteDataBeforeTimeStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_tTime;
	pSS->FieldArray[2].pVal = &Local_iMaxRecsToDelete;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitDeleteDataBeforeTimeStatement() */


static int PrepDeleteDataBeforeTimeExec(EWDBid IN_tTime, int IN_iMaxRecsToDelete, 
                                     EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLBindArray;
	SSStatement.RecordSize = 0;

	Local_iRetCode         = 0;
	Local_tTime            = IN_tTime;
  Local_iMaxRecsToDelete = IN_iMaxRecsToDelete;

	if(InitDeleteDataBeforeTimeStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepDeleteDataBeforeTimeExec(): InitDeleteDataBeforeTimeStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepDeleteDataBeforeTimeExec() */


static int PostDeleteDataBeforeTimeExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  *pRetCode=Local_iRetCode;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_iRetCode < 0)
  {
    logit("", "PostDeleteDataBeforeTimeExec(): SQL Proc Delete_Data_Before_Time() failed with error %d\n",
          Local_iRetCode);
    return(EWDB_RETURN_FAILURE);
  }
  else if(Local_iRetCode > 0)
  {
    return(EWDB_RETURN_WARNING);
  }
    
  return(EWDB_RETURN_SUCCESS);
}  /* end PostDeleteDataBeforeTimeExec() */

