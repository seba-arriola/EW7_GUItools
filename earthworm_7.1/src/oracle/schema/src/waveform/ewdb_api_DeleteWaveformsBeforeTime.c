/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteWaveformsBeforeTime.c,v 1.3 2004/09/09 17:25:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteWaveformsBeforeTime.c,v $
 *     Revision 1.3  2004/09/09 17:25:13  davidk
 *     Re-Updated for reaper v2.
 *     Now includes a pSQLRetCode, for more precise result description.
 *
 *     Revision 1.2  2004/09/09 05:48:56  davidk
 *     Updated for reaper v2.
 *     Now includes a iMaxRecsToDelete
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
  "  Delete_Waveforms_Before_Time(IN_tTime => :IN_tTime, "
  "                              IN_iMaxRecsToDelete => :IN_iMaxRecsToDelete); End;";

static EWDB_OCI_SFS SQLBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_tTime"},
  {0,1,0,0,0,OA_INT,":IN_iMaxRecsToDelete"}
};

#define	NUM_FIELDS	3

/* Insertion Struct for DeleteWaveformsBeforeTime statement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  iRetCode;
static	int     tTime;
static	int     iMaxRecsToDelete;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepDeleteWaveformsBeforeTimeExec(EWDBid IN_tTime, int IN_iMaxRecsToDelete, 
                                            EWDB_Cursor *ppCursor);
static int PostDeleteWaveformsBeforeTimeExec(int * pRetCode);
static int InitDeleteWaveformsBeforeTimeStatement (char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeleteWaveformsBeforeTime(int IN_tTime, int IN_iMaxRecsToDelete, int * pSQLRetCode)
{

	EWDB_Cursor pCursor;

	if(IN_tTime < 0 || pSQLRetCode == NULL)
	{
		logit("", "EWDB_DeleteWaveformsBeforeTime(): Invalid parameters passed in(%d %u)!\n",
          IN_tTime, pSQLRetCode);
		return EWDB_RETURN_FAILURE;
	}

	/* Set longer timeout -- need this for large events */
	ewdb_base_SetOraConnectionTimeout(15*60);

	if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "EWDB_DeleteWaveformsBeforeTime(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepDeleteWaveformsBeforeTimeExec(IN_tTime, IN_iMaxRecsToDelete, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:DeleteWaveformsBeforeTime():PrepDeleteWaveformsBeforeTimeExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"DeleteWaveformsBeforeTime:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeleteWaveformsBeforeTime:ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  if(PostDeleteWaveformsBeforeTimeExec(pSQLRetCode) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostDeleteWaveformsBeforeTimeExec failed!\n");
    return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if(*pSQLRetCode < 0)
	{
		logit ("", "SQL Proc DeleteWaveformsBeforeTime() failed with error: %d\n", *pSQLRetCode);
    return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_DeleteWaveformsBeforeTime() */


static int InitDeleteWaveformsBeforeTimeStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &iRetCode;
	pSS->FieldArray[1].pVal = &tTime;
	pSS->FieldArray[2].pVal = &iMaxRecsToDelete;

  if(ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}  /* end InitDeleteWaveformsBeforeTimeStatement() */


static int PrepDeleteWaveformsBeforeTimeExec(EWDBid IN_tTime, int IN_iMaxRecsToDelete, 
                                     EWDB_Cursor *ppCursor)
{

	if(ppCursor == NULL)
	{
		logit ("", "PrepDeleteWaveformsBeforeTimeExec(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLBindArray;
	SSStatement.RecordSize = 0;

	iRetCode = 0;
	tTime = IN_tTime;
  iMaxRecsToDelete = IN_iMaxRecsToDelete;

	if(InitDeleteWaveformsBeforeTimeStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitDeleteWaveformsBeforeTimeStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;

}  /* end PrepDeleteWaveformsBeforeTimeExec() */


static int PostDeleteWaveformsBeforeTimeExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  *pRetCode=iRetCode;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PostDeleteWaveformsBeforeTimeExec() */

