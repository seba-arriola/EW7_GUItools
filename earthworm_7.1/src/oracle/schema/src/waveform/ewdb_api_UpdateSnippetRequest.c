/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateSnippetRequest.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateSnippetRequest.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.2  2002/05/13 23:09:16  davidk
 *     Lots of little syntax-error fixes, and little changes to reflect changes in underlying calls.
 *     Now works for Concierge Mark2.
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *     Revision 1.4  2002/03/15 01:51:00  davidk
 *     modified code so that when bModifyAttemptParams is set to
 *      EWDB_WAVEFORM_REQUEST_UPDATE_NEXT_ATTEMPT, then the next attempt time
 *     for the snippet-request is scheduled for the current time plus
 *     pSnipReq->tAttemptInterval which should be set by the caller prior to the call.
 *
 *     Revision 1.3  2002/03/05 23:50:18  davidk
 *     Current version being tested on gldjimmy, already works on bsd1.
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
 */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char UPDATE_SnipReq_STRING[] =
  "Begin Update_SnipReq(OUT_RetCode => :OUT_RetCode,"
  " IN_idSnipReq => :IN_idSnipReq, IN_tNewStart => :IN_tNewStart," 
  " IN_tNewEnd => :IN_tNewEnd, IN_idExistingWaveform => :IN_idExistingWaveform,"
  " IN_tNextAttempt => :IN_tNextAttempt, "
  "IN_bModifyAttemptParams => :IN_bModifyAttemptParams, "
  "IN_bModifyResultParams => :IN_bModifyResultParams, "
  "IN_iAttemptRetCode => :IN_iAttemptRetCode, "
  "IN_iLockCheckTime => :IN_iLockCheckTime, "
  "IN_bUnlock => :IN_bUnlock, "
  "IN_sNote => :IN_sNote "
  "); End;";
 
 

#define	NUM_FIELDS 12

EWDB_OCI_SFS UpdateSnipReqBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSnipReq"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tNewStart"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tNewEnd"},
  {0,1,0,0,0,OA_EWDBID,":IN_idExistingWaveform"},
  {0,1,0,0,0,OA_INT,   ":IN_tNextAttempt"},
  {0,1,0,0,0,OA_INT,   ":IN_bModifyAttemptParams"},
  {0,1,0,0,0,OA_INT,   ":IN_bModifyResultParams"},
  {0,1,0,0,0,OA_INT,   ":IN_iAttemptRetCode"},
  {0,1,0,0,0,OA_INT,   ":IN_iLockCheckTime"},
  {0,1,0,0,0,OA_INT,   ":IN_bUnlock"},
  {0,1,0,0,0,OA_SZ,    ":IN_sNote"}
};

#define STR_LEN 20

static  int       USR_RetCode, USR_bModifyAttemptParams, USR_bModifyResultParams;
static  int    USR_tNextAttempt, USR_bUnlock = TRUE;
static  EWDB_SnippetRequestStruct USR_SnipReqStruct;
static  char USR_tStart[STR_LEN], USR_tEnd[STR_LEN]; 

/* Statement Struct for UpdateSnipReq statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepUpdateSnipReqExec(EWDB_SnippetRequestStruct * IN_pSnipReq,  
                          int IN_bModifyAttemptParams,
                          int IN_bModifyResultParams, EWDB_Cursor * ppCursor);
int PostUpdateSnipReqExec(void);
int InitUpdateSnipReqStatement(char *statement, 
                               EWDB_OCIStatementStruct *pSS);





/**********************************************************************
*********************************************************************/
int ewdb_api_UpdateSnippetRequest(EWDB_SnippetRequestStruct * IN_pSnipReq,  
                                  int IN_bModifyAttemptParams,
                                  int IN_bModifyResultParams)
{

  EWDB_Cursor pCursor;
  int rc;

  if (IN_pSnipReq == NULL )
  {
    logit ("", "EWDB_UpdateSnippetRequest():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "EWDB_UpdateSnippetRequest(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepUpdateSnipReqExec(IN_pSnipReq, IN_bModifyAttemptParams, 
                            IN_bModifyResultParams, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ORA_API:EWDB_UpdateSnippetRequest():PrepUpdateSnipReqExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_UpdateSnippetRequest():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"EWDB_UpdateSnippetRequest():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostUpdateSnipReqExec();
  if(rc == EWDB_RETURN_FAILURE)
    logit ("", "EWDB_UpdateSnippetRequest(): "
           "Call to PostUpdateSnipReqExec failed!\n");

  ewdb_base_SetLastOraAPIActionTime ();

  return(rc);
}  /* end ewdb_api_UpdateSnippetRequest() */


int InitUpdateSnipReqStatement(char *statement, 
                               EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &USR_RetCode;
	pSS->FieldArray[1].pVal = &(USR_SnipReqStruct.idSnipReq);
	pSS->FieldArray[2].pVal = USR_tStart;
	pSS->FieldArray[3].pVal = USR_tEnd;
	pSS->FieldArray[4].pVal = &(USR_SnipReqStruct.idExistingWaveform);
	pSS->FieldArray[5].pVal = &USR_tNextAttempt;
	pSS->FieldArray[6].pVal = &USR_bModifyAttemptParams;
	pSS->FieldArray[7].pVal = &USR_bModifyResultParams;
	pSS->FieldArray[8].pVal = &(USR_SnipReqStruct.iRetCode);
	pSS->FieldArray[9].pVal = &(USR_SnipReqStruct.iLockTime);
	pSS->FieldArray[10].pVal = &USR_bUnlock;
	pSS->FieldArray[11].pVal = USR_SnipReqStruct.szNote;

	ewdb_base_RequestCursor (statement, pSS, 0);

	return(EWDB_RETURN_SUCCESS);
}


int PrepUpdateSnipReqExec(EWDB_SnippetRequestStruct * IN_pSnipReq,  
                          int IN_bModifyAttemptParams,
                          int IN_bModifyResultParams, EWDB_Cursor * ppCursor)
{

  time_t CurrentTime;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = UpdateSnipReqBindArray;
  SSStatement.RecordSize = 0;

  memcpy(&USR_SnipReqStruct,IN_pSnipReq, sizeof(EWDB_SnippetRequestStruct));
  sprintf(USR_tStart,"%.4f",USR_SnipReqStruct.tStart);
  sprintf(USR_tEnd,"%.4f",USR_SnipReqStruct.tEnd);

  time(&CurrentTime);

  /* treat IN_pSnipreq->tNextAttempt as a delta time from now */
	if(IN_pSnipReq->tNextAttempt < 0)
		USR_tNextAttempt = CurrentTime;
	else
		USR_tNextAttempt = CurrentTime + (int)(IN_pSnipReq->tNextAttempt);


  USR_bModifyAttemptParams = IN_bModifyAttemptParams;
  USR_bModifyResultParams = IN_bModifyResultParams;



	if (InitUpdateSnipReqStatement (UPDATE_SnipReq_STRING,
				                   &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitUpdateSnipReqStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);

}


int PostUpdateSnipReqExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  if(USR_RetCode < 0)
  {
    logit("","%s():ERROR! %d returned by SQL Proc\n",
          "PostUpdateSnipReqExec",USR_RetCode);
    return(EWDB_RETURN_FAILURE);
  }
  else if(USR_RetCode > 0)
  {
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    return(EWDB_RETURN_SUCCESS);
  }
}


