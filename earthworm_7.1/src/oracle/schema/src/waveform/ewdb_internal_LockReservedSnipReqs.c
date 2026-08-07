/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_LockReservedSnipReqs.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_LockReservedSnipReqs.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.2  2002/05/13 23:01:00  davidk
 *     Added a pointer to iLockTime in the function spec, so that the LockTime value used
 *     to lock the snippets could be exported back to the caller, so that the caller can
 *     actually unlock the requests at a later time.  (Novel idea).
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
  "Begin Lock_Reserved_Snip_Reqs(OUT_RetCode => :OUT_RetCode,"
  " IN_iReserveKey => :IN_iReserveKey, "
  " IN_iLockTime => :IN_iLockTime "
  "); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,   ":IN_iReserveKey"},
  {0,1,0,0,0,OA_INT,   ":IN_iLockTime"}
};

#define	NUM_FIELDS	3

/* Insertion Struct for LockReservedSnipReqs statement */
static EWDB_OCIStatementStruct SSStatement;

static int    iReserveKey;
static int    iLockTime;
static int    iRetCode;

int PrepLockReservedSnipReqsExec(int IN_iReserveKey, EWDB_Cursor *ppCursor);
int PostLockReservedSnipReqsExec(void);
int InitLockReservedSnipReqsStatement(char *statement, EWDB_OCIStatementStruct *pSS);


int ewdb_internal_LockReservedSnipReqs(int iReserveKey, int * piLockTime)
{

	EWDB_Cursor pCursor;
  int rc;

	ewdb_base_SetLastOraAPIActionTime();

	if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit("", "Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepLockReservedSnipReqsExec(iReserveKey, &pCursor) 
    != EWDB_RETURN_SUCCESS)
	{
		logit("", "ORA_API:LockReservedSnipReqs():PrepLockReservedSnipReqsExec() failed.\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"LockReservedSnipReqs:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

  
	/* DO NOT COMMIT!! THIS IS PART OF A LARGER TRANSACTION
     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   ******************************************************/
	
	*piLockTime = iLockTime;

  rc = PostLockReservedSnipReqsExec();
  if( rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_WARNING)
    {
      return(EWDB_RETURN_FAILURE);
    }
    else
    {
      logit("", "Call to PostLockReservedSnipReqsExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }  /* end ! success */

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateSnippetRequest() */


int InitLockReservedSnipReqsStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) ||(pSS == NULL))
	{
		logit("", "InitLockReservedSnipReqsStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &(iRetCode);
	pSS->FieldArray[1].pVal = &(iReserveKey);
	pSS->FieldArray[2].pVal = &(iLockTime);


	if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
  {
    logit("", "InitLockReservedSnipReqsStatement(): "
           "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}


int PrepLockReservedSnipReqsExec(int IN_iReserveKey, EWDB_Cursor *ppCursor)
{

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;


  iRetCode      = 0;
  iReserveKey   = IN_iReserveKey;
  iLockTime     = time(NULL);

  if(InitLockReservedSnipReqsStatement(SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit("", "Call to InitLockReservedSnipReqsStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


int PostLockReservedSnipReqsExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(iRetCode != EWDB_RETURN_SUCCESS)
  {
    logit("","PostLockReservedSnipReqsExec(): ERROR!  SQL Proc Reserve_Snippet_Requests() "
             "failed with error(%d)!\n",
          iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostLockReservedSnipReqsExec() */
