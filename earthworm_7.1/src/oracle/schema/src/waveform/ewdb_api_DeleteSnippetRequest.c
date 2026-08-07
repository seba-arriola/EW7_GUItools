/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteSnippetRequest.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteSnippetRequest.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/05/15 02:16:17  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.3  2001/02/21 09:48:38  davidk
 *     Added a proper RCS header comment for the file.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


char DELETE_SnipReq_STRING[] =
  "Begin Delete_SnipReq(OUT_RetCode => :OUT_RetCode,"
  " IN_idSnipReq => :IN_idSnipReq, "
  " IN_iLockCheckTime => :IN_iLockCheckTime); End;";

EWDB_OCI_SFS DeleteSnipReqBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSnipReq"},
  {0,1,0,0,0,OA_INT,":IN_iLockCheckTime"}
};

#define	NUM_FIELDS	3

/* Insertion Struct for DeleteSnipReq statement */
EWDB_OCIStatementStruct SSDeleteSnipReq;

static	int		  iRetCode;
static	EWDBid	idSnipReq;
static  int     iLockTime;

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepDeleteSnipReqExec (EWDBid IN_idSnipReq, int IN_iLockTime, EWDB_Cursor *ppCursor);
int PostDeleteSnipReqExec(int * pRetCode);
int InitDeleteSnipReqStatement (char *statement, EWDB_OCIStatementStruct *pSS);



/**********************************************************************
**********************************************************************/
int ewdb_api_DeleteSnippetRequest (EWDBid IN_idSnipReq, int IN_iLockTime)
{

  int RetCode;

	EWDB_Cursor pCursor;

	if (IN_idSnipReq <= 0)
	{
		logit ("", "EWDB_DeleteSnippetRequest(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_DeleteSnippetRequest(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepDeleteSnipReqExec (IN_idSnipReq, IN_iLockTime, &pCursor) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:EWDB_DeleteSnippetRequest():PrepDeleteSnipReqExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_DeleteSnippetRequest:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"EWDB_DeleteSnippetRequest:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if (PostDeleteSnipReqExec (&RetCode) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostDeleteSnipReqExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (RetCode != 0)
	{
		logit ("", "SQL call to DeleteSnipReq() returned error: %d\n", RetCode);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	return EWDB_RETURN_SUCCESS;

}  


int InitDeleteSnipReqStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &iRetCode;
	pSS->FieldArray[1].pVal = &idSnipReq;
	pSS->FieldArray[2].pVal = &iLockTime;

  if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}

int PrepDeleteSnipReqExec (EWDBid IN_idSnipReq, int IN_iLockTime, 
                           EWDB_Cursor *ppCursor)
{

	if (ppCursor == NULL)
	{
		logit ("", "PrepDeleteSnipReqExec(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSDeleteSnipReq.NumOfFields = NUM_FIELDS;
	SSDeleteSnipReq.FieldArray = DeleteSnipReqBindArray;
	SSDeleteSnipReq.RecordSize = 0;

	iRetCode = 0;
	idSnipReq = IN_idSnipReq;
  iLockTime = IN_iLockTime;

	if (InitDeleteSnipReqStatement (DELETE_SnipReq_STRING, &SSDeleteSnipReq) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitDeleteSnipReqStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSDeleteSnipReq.pCda;
	
	return EWDB_RETURN_SUCCESS;

}

int PostDeleteSnipReqExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSDeleteSnipReq.pCda;
  
  *pRetCode=iRetCode;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return (EWDB_RETURN_SUCCESS);
}
