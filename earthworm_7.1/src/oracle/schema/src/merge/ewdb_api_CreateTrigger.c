
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateTrigger.c,v 1.2 2002/05/28 17:22:22 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateTrigger.c,v $
 *     Revision 1.2  2002/05/28 17:22:22  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2002/03/22 20:01:23  lucky
 *     Initial revision
 *
 */


  
#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
	"Begin Create_Trigger(OUT_idTrigger => :OUT_idTrigger,"
	"IN_idCoincidence => :IN_idCoincidence,"
	"IN_idChan => :IN_idChan,"
	"IN_tTrigger => :IN_tTrigger); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idTrigger"},
  {0,1,0,0,0,OA_INT,    ":IN_idCoincidence"},
  {0,1,0,0,0,OA_INT,    ":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tTrigger"},
};

#define	NUM_FIELDS	4

static char		szTriggerTime[20];

static	EWDB_TriggerStruct	LocalTriggerStruct;


/* Insertion Struct for CreateTrigger statement */
static EWDB_OCIStatementStruct SSStatement;

int PrepCreateTriggerExec(EWDB_TriggerStruct *pTrigger, EWDB_Cursor *ppCursor);
int PostCreateTriggerExec(EWDB_TriggerStruct * pTrigger);
int InitCreateTriggerStatement(char *statement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreateTrigger(EWDB_TriggerStruct *pTrigger)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pTrigger == NULL)
	{
		logit ("", "EWDB_CreateTrigger():Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if (ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	{
		logit("", "Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if (PrepCreateTriggerExec(pTrigger, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit("", "ORA_API:EWDB_CreateTrigger():PrepCreateTriggerExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_CreateTrigger:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport(hEWDBC, hEWDBC,"EWDB_CreateTrigger:ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

  rc = PostCreateTriggerExec(pTrigger);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "Call to PostCreateTriggerExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }


	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateTrigger() */


int InitCreateTriggerStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) || (pSS == NULL))
	{
		logit("", "Null parameters passed in!\n");
		return(EWDB_RETURN_FAILURE);
	}

	pSS->FieldArray[0].pVal = &(LocalTriggerStruct.idTrigger);
	pSS->FieldArray[1].pVal = &(LocalTriggerStruct.idCoincidence);
	pSS->FieldArray[2].pVal = &(LocalTriggerStruct.idChan);
	pSS->FieldArray[3].pVal = szTriggerTime;

	if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
	{
		logit("", "Call to ewdb_base_RequestCursor failed.\n");
		return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
}  /* end InitCreateTriggerStatement() */


int PrepCreateTriggerExec(EWDB_TriggerStruct *pTrigger, EWDB_Cursor *ppCursor)
{

	if((pTrigger == NULL) || (ppCursor == NULL))
	{
		logit("", "Null parameters passed in!\n");
		return(EWDB_RETURN_FAILURE);
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	memcpy(&LocalTriggerStruct, pTrigger, sizeof (EWDB_TriggerStruct));

	sprintf(szTriggerTime, "%0.2f", pTrigger->tTrigger);

	if(InitCreateTriggerStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit("", "Call to InitCreateTriggerStatement failed!\n");
		return(EWDB_RETURN_FAILURE);
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);

}  /* end PrepCreateTriggerExec() */


int PostCreateTriggerExec(EWDB_TriggerStruct * pTrigger)
{
  EWDB_Cursor pCursor;
  
  if (pTrigger == NULL)
  {
    logit("", "Invalid arguments passed in.\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  pTrigger->idTrigger = LocalTriggerStruct.idTrigger;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(pTrigger->idTrigger < 0)
  {
		logit("","PostCreateTriggerExec():  SQL Proc Create_Trigger () returned "
               "the following error(%d).  Please see that proc for details.\n",
       				     pTrigger->idTrigger);

		return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateTriggerExec() */
