/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteSMMessagesBeforeTime.c,v 1.1 2001/07/14 07:40:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteSMMessagesBeforeTime.c,v $
 *     Revision 1.1  2001/07/14 07:40:13  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Delete_SMMessages_Before_Time(OUT_RetCode => :OUT_RetCode, "
  "  IN_tTime => :IN_tTime, IN_bForce => :IN_bForce); End;";


static EWDB_OCI_SFS SQLBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_tTime"},
  {0,1,0,0,0,OA_INT,":IN_bForce"}
};

#define	NUM_FIELDS	3

/* Insertion Struct for DeleteSMMessagesBeforeTime statement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  iRetCode;
static	int     tTime;
static	int     bForce;




/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepDeleteSMMessagesBeforeTimeExec(EWDBid IN_tTime, int IN_bForce, 
                                     EWDB_Cursor *ppCursor);
int PostDeleteSMMessagesBeforeTimeExec(int * pRetCode);
int InitDeleteSMMessagesBeforeTimeStatement (char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeleteSMMessagesBeforeTime(int IN_tTime, int IN_bForce)
{

  int RetCode;

	EWDB_Cursor pCursor;

	if(IN_tTime < 0)
	{
		logit ("", "EWDB_DeleteSMMessagesBeforeTime(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	/* Set longer timeout -- need this for large events */
	ewdb_base_SetOraConnectionTimeout(15*60);


	if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "EWDB_DeleteSMMessagesBeforeTime(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepDeleteSMMessagesBeforeTimeExec(IN_tTime, IN_bForce, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:DeleteSMMessagesBeforeTime():PrepDeleteSMMessagesBeforeTimeExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"DeleteSMMessagesBeforeTime:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeleteSMMessagesBeforeTime:ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if(PostDeleteSMMessagesBeforeTimeExec(&RetCode) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostDeleteSMMessagesBeforeTimeExec failed!\n");
    return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if(RetCode != 0)
	{
		logit ("", "SQL Proc DeleteSMMessagesBeforeTime() failed with error: %d\n", RetCode);
    return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_DeleteSMMessagesBeforeTime() */


int InitDeleteSMMessagesBeforeTimeStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &iRetCode;
	pSS->FieldArray[1].pVal = &tTime;
	pSS->FieldArray[2].pVal = &bForce;

  if(ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}  /* end InitDeleteSMMessagesBeforeTimeStatement() */


int PrepDeleteSMMessagesBeforeTimeExec(EWDBid IN_tTime, int IN_bForce, 
                                     EWDB_Cursor *ppCursor)
{

	if(ppCursor == NULL)
	{
		logit ("", "PrepDeleteSMMessagesBeforeTimeExec(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLBindArray;
	SSStatement.RecordSize = 0;

	iRetCode = 0;
	tTime = IN_tTime;
  bForce = IN_bForce;

	if(InitDeleteSMMessagesBeforeTimeStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitDeleteSMMessagesBeforeTimeStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;

}  /* end PrepDeleteSMMessagesBeforeTimeExec() */


int PostDeleteSMMessagesBeforeTimeExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  *pRetCode=iRetCode;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PostDeleteSMMessagesBeforeTimeExec() */

