/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteGroup.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteGroup.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/02/03 20:42:27  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin DeleteGroup(OUT_RetCode => :OUT_RetCode,"
		"IN_idGroup => :IN_idGroup); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idGroup"}
};

#define	NUM_FIELDS	2


/* Insertion Struct for DeleteGroup szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int Local_iRetCode;
static int Local_idGroup;

static int PrepDeleteGroupExec(EWDBid idGroup, EWDB_Cursor *ppCursor);
static int PostDeleteGroupExec();
static int InitDeleteGroupStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_DeleteGroup(EWDBid idGroup)
{
	EWDB_Cursor pCursor;
	int rc;

	if (idGroup <= 0)
	{
		logit ("", "ewdb_api_DeleteGroup(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_DeleteGroup(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepDeleteGroupExec (idGroup, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_DeleteGroup(): PrepDeleteGroupExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeleteGroup(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeleteGroup(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
	rc = PostDeleteGroupExec();
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "ewdb_api_DeleteGroup(): PostDeleteGroupExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
	else if(rc == EWDB_RETURN_WARNING)
	{
		return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
}


static int InitDeleteGroupStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) || (pSS == NULL))
	{
		logit ("", "InitDeleteGroupStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idGroup;

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
	{
		logit ("", "InitDeleteGroupStatement(): Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


static int PrepDeleteGroupExec(EWDBid idGroup, EWDB_Cursor *ppCursor)
{
	if (ppCursor == NULL)
	{
		logit ("", "PrepDeleteGroupExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incoming struct into the local struct */
	Local_idGroup = idGroup;

	if(InitDeleteGroupStatement (SQL_STRING, &SSStatement)
		 != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitDeleteGroupStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


static int PostDeleteGroupExec()
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor (pCursor);

	if(Local_iRetCode != 0)
	{
		logit("","PostDeleteGroupExec():  SQL Proc DeleteGroup() returned "
				"the following error(%d).  Please see that proc for details.\n",
				Local_iRetCode);
		return(EWDB_RETURN_WARNING);
	}

	return (EWDB_RETURN_SUCCESS);
}
