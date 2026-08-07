/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateMerge.c,v 1.1 2002/03/22 20:01:23 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateMerge.c,v $
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
	"Begin Create_Merge(OUT_idMerge => :OUT_idMerge,"
	"IN_idPh => :IN_idPh,"
	"IN_idEvent => :IN_idEvent,"
	"IN_sSource => :IN_sSource,"
	"IN_sComment => :IN_sComment); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idMerge"},
  {0,1,0,0,0,OA_INT,":IN_idPh"},
  {0,1,0,0,0,OA_INT,":IN_idEvent"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"},
};

#define	NUM_FIELDS	5

static EWDB_OCIStatementStruct SSStatement;

static	EWDB_MergeStruct	LocalMergeStruct;


/*******************************
   FUNCTION PROTOTYPES
*******************************/
int PrepCreateMergeTExec(EWDB_MergeStruct *pMerge, EWDB_Cursor *ppCursor);
int PostCreateMergeTExec(EWDB_MergeStruct *pMerge);
int InitCreateMergeTStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_CreateMerge (EWDB_MergeStruct *pMerge)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pMerge == NULL)
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_CreateMergeT(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateMergeTExec (pMerge, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:CreateMergeT():PrepCreateMergeTExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreateMergeT:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateMergeT:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	rc = PostCreateMergeTExec(pMerge);
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "Call to PostCreateMergeTExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateMergeT() */


int InitCreateMergeTStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(LocalMergeStruct.idMerge);
	pSS->FieldArray[1].pVal = &(LocalMergeStruct.idPh);
	pSS->FieldArray[2].pVal = &(LocalMergeStruct.idEvent);
	pSS->FieldArray[3].pVal = LocalMergeStruct.szSource;
	pSS->FieldArray[4].pVal = LocalMergeStruct.szComment;

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitCreateMergeTStatement(): "
				"Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


int PrepCreateMergeTExec (EWDB_MergeStruct *pMerge, EWDB_Cursor *ppCursor)
{

	if ((pMerge == NULL) || (ppCursor == NULL))
	{
		logit ("", "PostCreateMergeTExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incomming struct into the local struct */
	memcpy (&LocalMergeStruct, pMerge, sizeof (EWDB_MergeStruct));

	if (InitCreateMergeTStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitCreateMergeTStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} 


int PostCreateMergeTExec (EWDB_MergeStruct *pMerge)
{

	EWDB_Cursor pCursor;
  
	if (pMerge == NULL)
	{
		logit ("", "PostCreateMergeTExec(): Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}
  
	pMerge->idMerge = LocalMergeStruct.idMerge;

	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

	if(pMerge->idMerge <= 0)
	{
		logit ("","PostCreateMergeTExec():  SQL Proc Create_Merge() "
				"failed; return value = %d\n", pMerge->idMerge);
		return (EWDB_RETURN_WARNING);
	}
  
	return (EWDB_RETURN_SUCCESS);

} 
