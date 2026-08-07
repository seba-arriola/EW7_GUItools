/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_LockSnipReq.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_LockSnipReq.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.3  2002/06/26 22:38:53  davidk
 *     swapped two fields (idSnipreq, iReserveKey) in the BindParams struct.  They had
 *     been placed in the wrong order, and as such were getting swapped as they were
 *     passed to the SQL procedure.
 *
 *     Revision 1.2  2002/05/13 23:02:31  davidk
 *     Added an iLockTime pointer in the function spec, so that the function
 *     can pass the iLockTime value used to lock the request, back out to
 *     the caller, so that the caller can release the lock at a later time.
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
  "Begin Lock_Reserved_Snip_Req(OUT_RetCode => :OUT_RetCode,"
  " IN_idSnipReq => :IN_idSnipReq, "
  " IN_iReserveKey => :IN_iReserveKey, "
  " IN_iLockTime => :IN_iLockTime "
  "); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSnipReq"},
  {0,1,0,0,0,OA_INT,   ":IN_iReserveKey"},
  {0,1,0,0,0,OA_INT,   ":IN_iLockTime"}
};

#define	NUM_FIELDS	4

/* Insertion Struct for LockSnipReq statement */
static EWDB_OCIStatementStruct SSStatement;

static int    idSnipReq;
static int    iReserveKey;
static int    iLockTime;
static int    iRetCode;

int PrepLockSnipReqExec(EWDB_SnippetRequestStruct * pSnipReq,
                        int IN_iReserveKey, EWDB_Cursor *ppCursor);
int PostLockSnipReqExec(void);
int InitLockSnipReqStatement(char *statement, EWDB_OCIStatementStruct *pSS);

int ewdb_internal_LockSnipReq(EWDB_SnippetRequestStruct * pSnipReq,
                              int IN_iReserveKey, int * piLockTime)
{

	EWDB_Cursor pCursor;
  int rc;

  if(!pSnipReq)
  {
    logit("","ewdb_internal_LockSnipReq(): ERROR: NULL pSnipReq param.\n");
    return(EWDB_RETURN_FAILURE);
  }

	ewdb_base_SetLastOraAPIActionTime();

	if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit("", "Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepLockSnipReqExec(pSnipReq, IN_iReserveKey, &pCursor) 
    != EWDB_RETURN_SUCCESS)
	{
		logit("", "ORA_API:LockSnipReq():PrepLockSnipReqExec() failed.\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"LockSnipReq:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

  
	/* DO NOT COMMIT!! THIS IS PART OF A LARGER TRANSACTION
     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   ******************************************************/
	
	*piLockTime = iLockTime;

  rc = PostLockSnipReqExec();
  if( rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_WARNING)
    {
      return(EWDB_RETURN_FAILURE);
    }
    else
    {
      logit("", "Call to PostLockSnipReqExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }  /* end ! success */

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateSnippetRequest() */


int InitLockSnipReqStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) ||(pSS == NULL))
	{
		logit("", "InitLockSnipReqStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &(iRetCode);
	pSS->FieldArray[1].pVal = &(idSnipReq);
	pSS->FieldArray[2].pVal = &(iReserveKey);
	pSS->FieldArray[3].pVal = &(iLockTime);


	if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
  {
    logit("", "InitLockSnipReqStatement(): "
           "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}


int PrepLockSnipReqExec(EWDB_SnippetRequestStruct * pSnipReq,
                        int IN_iReserveKey, EWDB_Cursor *ppCursor)
{

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;


  idSnipReq     = pSnipReq->idSnipReq;
  iRetCode      = 0;
  iReserveKey   = IN_iReserveKey;
  iLockTime     = time(NULL);

  if(InitLockSnipReqStatement(SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit("", "Call to InitLockSnipReqStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


int PostLockSnipReqExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(iRetCode != EWDB_RETURN_SUCCESS)
  {
    logit("","PostLockSnipReqExec(): ERROR!  SQL Proc Reserve_Snippet_Requests() "
             "failed with error(%d)!\n",
          iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostLockSnipReqExec() */
