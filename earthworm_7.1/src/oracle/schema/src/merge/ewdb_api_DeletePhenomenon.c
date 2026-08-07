/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeletePhenomenon.c,v 1.1 2002/06/18 16:10:12 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeletePhenomenon.c,v $
 *     Revision 1.1  2002/06/18 16:10:12  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Delete_Phenomenon (OUT_RetCode => :OUT_RetCode,"
  "IN_idPh => :IN_idPh); End;";

static EWDB_OCI_SFS SQLBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idPh"},
};

#define	NUM_FIELDS	2

/* Insertion Struct for DeletePheno statement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  iRetCode;
static	EWDBid	  idPh;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepDeletePhenoExec(EWDBid IN_idPh, EWDB_Cursor *ppCursor);
int PostDeletePhenoExec(int * pRetCode);
int InitDeletePhenoStatement (char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeletePhenomenon (EWDBid IN_idPh)
{

  int RetCode;

	EWDB_Cursor pCursor;

	if (IN_idPh <= 0)
	{
		logit ("", "EWDB_DeletePheno(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_DeletePheno(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepDeletePhenoExec(IN_idPh, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:DeletePheno():PrepDeletePhenoExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"DeletePheno:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeletePheno:ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if(PostDeletePhenoExec(&RetCode) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostDeletePhenoExec failed!\n");
    return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if(RetCode < 0)
	{
		logit ("", "SQL Proc DeletePheno() failed with error: %d\n", RetCode);
    	return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime();

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_DeletePheno() */


int InitDeletePhenoStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &iRetCode;
	pSS->FieldArray[1].pVal = &idPh;

  if(ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}


int PrepDeletePhenoExec(EWDBid IN_idPh, EWDB_Cursor *ppCursor)
{

	if(ppCursor == NULL)
	{
		logit ("", "PrepDeletePhenoExec(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLBindArray;
	SSStatement.RecordSize = 0;

	iRetCode = 0;
	idPh = IN_idPh;

	if(InitDeletePhenoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitDeletePhenoStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;

}

int PostDeletePhenoExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  *pRetCode=iRetCode;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return(EWDB_RETURN_SUCCESS);
}
