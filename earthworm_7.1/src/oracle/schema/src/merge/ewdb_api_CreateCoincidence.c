/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateCoincidence.c,v 1.1 2002/03/22 20:01:23 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateCoincidence.c,v $
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
	"Begin Create_Coincidence(OUT_idCoincidence => :OUT_idCoincidence,"
	"IN_tCoincidence => :IN_tCoincidence,"
	"IN_bBindToEvent => :IN_bBindToEvent,"
	"IN_idEvent => :IN_idEvent,"
	"IN_sSource => :IN_sSource,"
	"IN_sComment => :IN_sComment); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idCoincidence"},
  {0,1,0,0,0,OA_FLOAT,":IN_tCoincidence"},
  {0,1,0,0,0,OA_INT,":IN_bBindToEvent"},
  {0,1,0,0,0,OA_INT,":IN_idEvent"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"},
};

#define	NUM_FIELDS	6

static char	sztCoinc[20];


static EWDB_OCIStatementStruct SSStatement;

static	EWDB_CoincEventStruct	LocalCoincStruct;


/*******************************
   FUNCTION PROTOTYPES
*******************************/
int PrepCreateCoincTExec(EWDB_CoincEventStruct *pCoinc, EWDB_Cursor *ppCursor);
int PostCreateCoincTExec(EWDB_CoincEventStruct *pCoinc);
int InitCreateCoincTStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_CreateCoincidence (EWDB_CoincEventStruct *pCoinc)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pCoinc == NULL)
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_CreateCoincT(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateCoincTExec (pCoinc, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:CreateCoincT():PrepCreateCoincTExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreateCoincT:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateCoincT:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	rc = PostCreateCoincTExec(pCoinc);
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "Call to PostCreateCoincTExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateCoincT() */


int InitCreateCoincTStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(LocalCoincStruct.idCoincidence);
	pSS->FieldArray[1].pVal = sztCoinc;
	pSS->FieldArray[2].pVal = &(LocalCoincStruct.bBindToEvent);
	pSS->FieldArray[3].pVal = &(LocalCoincStruct.idEvent);
	pSS->FieldArray[4].pVal = LocalCoincStruct.szSource;
	pSS->FieldArray[5].pVal = LocalCoincStruct.szComment;

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitCreateCoincTStatement(): "
				"Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


int PrepCreateCoincTExec (EWDB_CoincEventStruct *pCoinc, EWDB_Cursor *ppCursor)
{

	if ((pCoinc == NULL) || (ppCursor == NULL))
	{
		logit ("", "PostCreateCoincTExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incomming struct into the local struct */
	memcpy (&LocalCoincStruct, pCoinc, sizeof (EWDB_CoincEventStruct));

	sprintf (sztCoinc, "%0.2f", pCoinc->tCoincidence);

	if (InitCreateCoincTStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitCreateCoincTStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} 


int PostCreateCoincTExec (EWDB_CoincEventStruct *pCoinc)
{

	EWDB_Cursor pCursor;
  
	if (pCoinc == NULL)
	{
		logit ("", "PostCreateCoincTExec(): Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}
  
	pCoinc->idCoincidence = LocalCoincStruct.idCoincidence;

	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

	if(pCoinc->idCoincidence <= 0)
	{
		logit ("","PostCreateCoincTExec():  SQL Proc Create_Coincidence() "
				"failed; return value = %d\n", pCoinc->idCoincidence);
		return (EWDB_RETURN_WARNING);
	}
  
	return (EWDB_RETURN_SUCCESS);

} 
