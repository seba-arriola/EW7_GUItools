/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateSnippetRequest.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateSnippetRequest.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.3  2002/07/16 19:25:29  davidk
 *     Changed the code to call SQL proc Find_Or_Create_SnipReq() instead
 *     of Create_SnipReq(), as part of the initiative to reduce the number
 *     of duplicate/overlapping snippets in the database.
 *
 *     Revision 1.2  2002/05/13 22:58:59  davidk
 *     Changed tInitialAttempt to the correct variable tInitialRequest, reflecting the time
 *     that the request was for data was recorded in the DB.
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *     Revision 1.2  2002/03/05 23:25:52  davidk
 *     Increaesed the length of the local string values that hold the
 *     double-floats as they are passed to Oracle.  They were 15, but there should've been
 *     some 15-character numbers, so we ended up with garbage attached on the
 *     back end, and thus a non-numeric value (and an overflow).
 *
 *     Revision 1.1  2001/07/23 16:52:54  davidk
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
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Find_Or_Create_SnipReq(OUT_RetCode => :OUT_RetCode,"
  " OUT_idSnipReq => :OUT_idSnipReq, IN_idChan => :IN_idChan,"
  " IN_tStart => :IN_tStart,IN_tEnd => :IN_tEnd,"
  " IN_idEvent => :IN_idEvent, IN_iNumAttempts => :IN_iNumAttempts,"
  " IN_tInitialRequest => :IN_tInitialRequest,"
  " IN_iRequestGroup => :IN_iRequestGroup, "
  " IN_bForce => 0 /* hard coded to not force */ "
  "); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idSnipReq"},
  {0,1,0,0,0,OA_EWDBID,":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tStart"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tEnd"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"},
  {0,1,0,0,0,OA_INT,   ":IN_iNumAttempts"},
  {0,1,0,0,0,OA_INT,   ":IN_tInitialRequest"},
  {0,1,0,0,0,OA_INT,   ":IN_iRequestGroup"}
};

#define	NUM_FIELDS	9
#define STRING_LEN 20

/* Insertion Struct for CreateSnipReq statement */
static EWDB_OCIStatementStruct SSStatement;

static char sztStart[STRING_LEN], sztEnd[STRING_LEN];
static int  iRetCode;

static	EWDB_SnippetRequestStruct LocalSnipReqStruct;

int PrepCreateSnipReqExec(EWDB_SnippetRequestStruct *pSnipReq, EWDB_Cursor *ppCursor);
int PostCreateSnipReqExec(EWDB_SnippetRequestStruct *pSnipReq);
int InitCreateSnipReqStatement(char *statement, EWDB_OCIStatementStruct *pSS);


int ewdb_internal_CreateSnippetRequest(EWDB_SnippetRequestStruct *pSnipReq)
{

	EWDB_Cursor pCursor;
  int rc;

	if(pSnipReq == NULL)
	{
		logit("", "Null EWDB_SnipReqStruct passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit("", "Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepCreateSnipReqExec(pSnipReq, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit("", "ORA_API:CreateSnipReq():PrepCreateSnipReqExec() failed.\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"CreateSnipReq:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction(all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport(hEWDBC, hEWDBC,"CreateSnipReq:ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
	
  rc = PostCreateSnipReqExec(pSnipReq);
  if( rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_WARNING)
    {
      return(EWDB_RETURN_FAILURE);
    }
    else
    {
      logit("", "Call to PostCreateSnipReqExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }  /* end ! success */

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateSnippetRequest() */


int InitCreateSnipReqStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) ||(pSS == NULL))
	{
		logit("", "InitCreateSnipReqStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &(iRetCode);
	pSS->FieldArray[1].pVal = &(LocalSnipReqStruct.idSnipReq);
	pSS->FieldArray[2].pVal = &(LocalSnipReqStruct.idChan);
	pSS->FieldArray[3].pVal = sztStart;
	pSS->FieldArray[4].pVal = sztEnd;
	pSS->FieldArray[5].pVal = &(LocalSnipReqStruct.idEvent);
	pSS->FieldArray[6].pVal = &(LocalSnipReqStruct.iNumAttempts);
	pSS->FieldArray[7].pVal = &(LocalSnipReqStruct.tInitialRequest);
	pSS->FieldArray[8].pVal = &(LocalSnipReqStruct.iRequestGroup);


	if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
  {
    logit("", "InitCreateSnipReqStatement(): "
           "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}


int PrepCreateSnipReqExec(EWDB_SnippetRequestStruct *pSnipReq, EWDB_Cursor *ppCursor)
{

	if((pSnipReq == NULL) ||(ppCursor == NULL))
	{
		logit("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;


	/* Copy the incoming struct into the local struct */
	memcpy(&LocalSnipReqStruct, pSnipReq, sizeof(EWDB_SnippetRequestStruct));

  iRetCode = 0;

  sprintf(sztStart,"%.4f",LocalSnipReqStruct.tStart);
  sprintf(sztEnd,"%.4f",LocalSnipReqStruct.tEnd);

  if(InitCreateSnipReqStatement(SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit("", "Call to InitCreateSnipReqStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;

}

int PostCreateSnipReqExec(EWDB_SnippetRequestStruct *pSnipReq)
{
  EWDB_Cursor pCursor;
  
  if(pSnipReq == NULL)
  {
    logit("", "Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pSnipReq->idSnipReq=LocalSnipReqStruct.idSnipReq;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(iRetCode < 0)
  {
    logit("","PostCreateSnipReqExec(): ERROR!  SQL Proc Find_Or_Create_SnipReq() "
             "failed with error(%d)!\n",
          iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateSnipReqExec() */
