/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_ReserveSnippetRequests.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_ReserveSnippetRequests.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.2  2002/05/13 22:58:28  davidk
 *     Fixed misspelled copies of tThreshold.
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
  "Begin Reserve_Snippet_Requests(OUT_RetCode => :OUT_RetCode,"
  " IN_idSnipReq => :IN_idSnipReq, IN_tThreshold => :IN_tThreshold,"
  " IN_iRequestGroup => :IN_iRequestGroup, IN_iReserveKey => :IN_iReserveKey"
  "); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSnipReq"},
  {0,1,0,0,0,OA_INT,   ":IN_tThreshold"},
  {0,1,0,0,0,OA_INT,   ":IN_iRequestGroup"},
  {0,1,0,0,0,OA_INT,   ":IN_iReserveKey"}
};

#define	NUM_FIELDS	5
#define STRING_LEN 20

/* Insertion Struct for ReserveSnippetRequests statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDBid idSnipReq;
static time_t tThreshold;
static int    iRequestGroup, iReserveKey;
static int    iRetCode;

int PrepReserveSnippetRequestsExec(EWDBid IN_idSnipReq, time_t IN_tThreshold, 
                                   int IN_iRequestGroup, int IN_iReserveKey, 
                                   EWDB_Cursor *ppCursor);
int PostReserveSnippetRequestsExec(void);
int InitReserveSnippetRequestsStatement(char *statement, EWDB_OCIStatementStruct *pSS);


int ewdb_internal_ReserveSnippetRequests(EWDBid idSnipReq,
                                         time_t tThreshold, 
                                         int iRequestGroup,
                                         int iReserveKey)
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

	if(PrepReserveSnippetRequestsExec(idSnipReq, tThreshold, iRequestGroup,
                                    iReserveKey, &pCursor) 
    != EWDB_RETURN_SUCCESS)
	{
		logit("", "ORA_API:ReserveSnippetRequests():PrepReserveSnippetRequestsExec() failed.\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"ReserveSnippetRequests:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

  
	/* DO NOT COMMIT!! THIS IS PART OF A LARGER TRANSACTION
     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   ******************************************************/
	
  rc = PostReserveSnippetRequestsExec();
  if( rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_WARNING)
    {
      return(EWDB_RETURN_FAILURE);
    }
    else
    {
      logit("", "Call to PostReserveSnippetRequestsExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }  /* end ! success */

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateSnippetRequest() */


int InitReserveSnippetRequestsStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) ||(pSS == NULL))
	{
		logit("", "InitReserveSnippetRequestsStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &(iRetCode);
	pSS->FieldArray[1].pVal = &(idSnipReq);
	pSS->FieldArray[2].pVal = &(tThreshold);
	pSS->FieldArray[3].pVal = &(iRequestGroup);
	pSS->FieldArray[4].pVal = &(iReserveKey);


	if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
  {
    logit("", "InitReserveSnippetRequestsStatement(): "
           "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}


int PrepReserveSnippetRequestsExec(EWDBid IN_idSnipReq, time_t IN_tThreshold, 
                                   int IN_iRequestGroup, int IN_iReserveKey, 
                                   EWDB_Cursor *ppCursor)
{

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;


  iRetCode      = 0;
  idSnipReq     = IN_idSnipReq;
  tThreshold    = IN_tThreshold, 
  iRequestGroup = IN_iRequestGroup;
  iReserveKey   = IN_iReserveKey;

  if(InitReserveSnippetRequestsStatement(SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit("", "Call to InitReserveSnippetRequestsStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


int PostReserveSnippetRequestsExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(iRetCode != EWDB_RETURN_SUCCESS)
  {
    logit("","PostReserveSnippetRequestsExec(): ERROR!  SQL Proc Reserve_Snippet_Requests() "
             "failed with error(%d)!\n",
          iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostReserveSnippetRequestsExec() */
