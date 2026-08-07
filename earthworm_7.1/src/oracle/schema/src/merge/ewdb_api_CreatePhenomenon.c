/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePhenomenon.c,v 1.1 2002/03/22 20:01:23 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePhenomenon.c,v $
 *     Revision 1.1  2002/03/22 20:01:23  lucky
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
	"Begin Create_Phenomenon (OUT_idPh => :OUT_idPh,"
	"IN_idPrefEvent => :IN_idPrefEvent,"
	"IN_sSource => :IN_sSource,"
	"IN_sComment => :IN_sComment); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idPh"},
  {0,1,0,0,0,OA_INT,":IN_idPrefEvent"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"},
};

#define	NUM_FIELDS	4

static EWDB_OCIStatementStruct SSStatement;

static	EWDB_PhenomenonStruct	LocalPhenoStruct;


/*******************************
   FUNCTION PROTOTYPES
*******************************/
int PrepCreatePhenoTExec(EWDB_PhenomenonStruct *pPhen, EWDB_Cursor *ppCursor);
int PostCreatePhenoTExec(EWDB_PhenomenonStruct *pPhen);
int InitCreatePhenoTStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_CreatePhenomenon (EWDB_PhenomenonStruct *pPhen)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pPhen == NULL)
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_CreatePhenoT(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreatePhenoTExec (pPhen, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:CreatePhenoT():PrepCreatePhenoTExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreatePhenoT:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreatePhenoT:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	rc = PostCreatePhenoTExec(pPhen);
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "Call to PostCreatePhenoTExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreatePhenoT() */


int InitCreatePhenoTStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(LocalPhenoStruct.idPh);
	pSS->FieldArray[1].pVal = &(LocalPhenoStruct.idPrefEvent);
	pSS->FieldArray[2].pVal = LocalPhenoStruct.szSource;
	pSS->FieldArray[3].pVal = LocalPhenoStruct.szComment;

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitCreatePhenoTStatement(): "
				"Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


int PrepCreatePhenoTExec (EWDB_PhenomenonStruct *pPhen, EWDB_Cursor *ppCursor)
{

	if ((pPhen == NULL) || (ppCursor == NULL))
	{
		logit ("", "PostCreatePhenoTExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incomming struct into the local struct */
	memcpy (&LocalPhenoStruct, pPhen, sizeof (EWDB_PhenomenonStruct));

	if (InitCreatePhenoTStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitCreatePhenoTStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} 


int PostCreatePhenoTExec (EWDB_PhenomenonStruct *pPhen)
{

	EWDB_Cursor pCursor;
  
	if (pPhen == NULL)
	{
		logit ("", "PostCreatePhenoTExec(): Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}
  
	pPhen->idPh = LocalPhenoStruct.idPh;

	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

	if(pPhen->idPh <= 0)
	{
		logit ("","PostCreatePhenoTExec():  SQL Proc Create_Phenomenon() "
				"failed; return value = %d\n", pPhen->idPh);
		return (EWDB_RETURN_WARNING);
	}
  
	return (EWDB_RETURN_SUCCESS);

} 
