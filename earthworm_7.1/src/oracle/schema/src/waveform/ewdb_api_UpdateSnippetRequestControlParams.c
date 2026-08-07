/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateSnippetRequestControlParams.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateSnippetRequestControlParams.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.2  2002/05/13 23:10:18  davidk
 *     Changed the SQL Proc that was called by the code (was update snipreq, instead of
 *     update snipreq controlparams).  Fixed the name of the function in all of the logging
 *     statements, where there had been cut and paste errors from where this function
 *     was copied from update_snipreq.
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *
 */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char UPDATE_SR_CP_STRING[] =
  "Begin Update_SnipReqControlParams(OUT_RetCode => :OUT_RetCode,"
  " IN_idSnipReq => :IN_idSnipReq, IN_iLockCheckTime => :IN_iLockCheckTime," 
  " IN_iRequestGroup => :IN_iRequestGroup, "
  " IN_iNumAttempts => :IN_iNumAttempts"
  "); End;";
 
 

#define	NUM_FIELDS 5

static EWDB_OCI_SFS UpdateSnipReqCPBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSnipReq"},
  {0,1,0,0,0,OA_INT,   ":IN_iLockCheckTime"},
  {0,1,0,0,0,OA_INT,   ":IN_iRequestGroup"},
  {0,1,0,0,0,OA_INT,   ":IN_iNumAttempts"}
};

#define STR_LEN 20

static  int       USR_RetCode;
static  EWDB_SnippetRequestStruct USR_SnipReqStruct;

/* Statement Struct for UpdateSnipReq statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepUpdateSnipReqControlParamsExec(EWDB_SnippetRequestStruct * IN_pSnipReq,  
                                       EWDB_Cursor * ppCursor);
int PostUpdateSnipReqControlParamsExec(void);
int InitUpdateSnipReqControlParamsStatement(char *statement, 
                               EWDB_OCIStatementStruct *pSS);





/**********************************************************************
*********************************************************************/
int ewdb_api_UpdateSnippetRequestControlParams(EWDB_SnippetRequestStruct * IN_pSnipReq)
{

  EWDB_Cursor pCursor;
  int rc;

  if (IN_pSnipReq == NULL )
  {
    logit ("", "ewdb_api_UpdateSnippetRequestControlParams():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_UpdateSnippetRequestControlParams(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepUpdateSnipReqControlParamsExec(IN_pSnipReq, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ORA_API:ewdb_api_UpdateSnippetRequestControlParams():PrepUpdateSnipReqControlParamsExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_UpdateSnippetRequestControlParams():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_UpdateSnippetRequestControlParams():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostUpdateSnipReqControlParamsExec();
  if(rc == EWDB_RETURN_FAILURE)
    logit ("", "ewdb_api_UpdateSnippetRequestControlParams(): "
           "Call to PostUpdateSnipReqControlParamsExec failed!\n");

  ewdb_base_SetLastOraAPIActionTime ();

  return(rc);
}  /* end ewdb_api_UpdateSnippetRequest() */


int InitUpdateSnipReqControlParamsStatement(char *statement, 
                               EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &USR_RetCode;
	pSS->FieldArray[1].pVal = &(USR_SnipReqStruct.idSnipReq);
	pSS->FieldArray[2].pVal = &(USR_SnipReqStruct.iLockTime);
	pSS->FieldArray[3].pVal = &(USR_SnipReqStruct.iRequestGroup);
	pSS->FieldArray[4].pVal = &(USR_SnipReqStruct.iNumAttempts);

	ewdb_base_RequestCursor (statement, pSS, 0);

	return(EWDB_RETURN_SUCCESS);
}


int PrepUpdateSnipReqControlParamsExec(EWDB_SnippetRequestStruct * IN_pSnipReq,  
                                       EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = UpdateSnipReqCPBindArray;
  SSStatement.RecordSize = 0;

  memcpy(&USR_SnipReqStruct,IN_pSnipReq, sizeof(EWDB_SnippetRequestStruct));

  if (InitUpdateSnipReqControlParamsStatement (UPDATE_SR_CP_STRING,
				                   &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitUpdateSnipReqControlParamsStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);

}


int PostUpdateSnipReqControlParamsExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  if(USR_RetCode < 0)
  {
    logit("","%s():ERROR! %d returned by SQL Proc\n",
          "PostUpdateSnipReqControlParamsExec",USR_RetCode);
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


