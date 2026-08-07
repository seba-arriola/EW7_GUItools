/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateMergePreference.c,v 1.1 2002/03/22 20:01:23 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateMergePreference.c,v $
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
	"Begin Update_Merge_Preference (OUT_idPrefEvent => :OUT_idPrefEvent,"
	"IN_idPh => :IN_idPh,"
	"IN_idPrefEvent => :IN_idPrefEvent); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idPrefEvent"},
  {0,1,0,0,0,OA_INT,":IN_idPh"},
  {0,1,0,0,0,OA_INT,":IN_idPrefEvent"},
};

#define	NUM_FIELDS	3

static EWDB_OCIStatementStruct SSStatement;

static	IN_idPh;
static	IN_idPrefEvent;
static	OUT_idPrefEvent;


/*******************************
   FUNCTION PROTOTYPES
*******************************/
int PrepUpdatePreferExec(EWDBid idPh, EWDBid idPrefEvent, EWDB_Cursor *ppCursor);
int PostUpdatePreferExec(void);
int InitUpdatePreferStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_UpdateMergePreference (EWDBid idPh, EWDBid idPrefEvent)
{

	EWDB_Cursor pCursor;
	int rc;

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_UpdatePrefer(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepUpdatePreferExec (idPh, idPrefEvent, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:UpdatePrefer():PrepUpdatePreferExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"UpdatePrefer:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"UpdatePrefer:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(PostUpdatePreferExec() == EWDB_RETURN_FAILURE)
	{
		logit("", "Call to PostUpdatePreferExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdatePrefer() */


int InitUpdatePreferStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(OUT_idPrefEvent);
	pSS->FieldArray[1].pVal = &(IN_idPh);
	pSS->FieldArray[2].pVal = &(IN_idPrefEvent);

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitUpdatePreferStatement(): "
				"Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


int PrepUpdatePreferExec (EWDBid idPh, EWDBid idPrefEvent, EWDB_Cursor *ppCursor)
{


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	IN_idPh = idPh;
	IN_idPrefEvent = idPrefEvent;

	if (InitUpdatePreferStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitUpdatePreferStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} 


int PostUpdatePreferExec ()
{

	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

	if (OUT_idPrefEvent < 0)
		return (EWDB_RETURN_FAILURE);

	return (EWDB_RETURN_SUCCESS);

} 
