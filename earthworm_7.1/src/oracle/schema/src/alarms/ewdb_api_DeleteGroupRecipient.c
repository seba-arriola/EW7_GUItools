/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteGroupRecipient.c,v 1.3 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteGroupRecipient.c,v $
 *     Revision 1.3  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2005/02/03 21:18:59  mark
 *     Renamed idGroupRecipient to IN_idRecipientDelivery to make things clearer
 *
 *     Revision 1.1  2005/02/03 20:42:27  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin DeleteGroupRecipient(OUT_RetCode => :OUT_RetCode,"
		"                         IN_idGroup => :IN_idGroup,"
		"                         IN_idRecipientDelivery => :IN_idRecipientDelivery); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idGroup"},
  {0,1,0,0,0,OA_INT,":IN_idRecipientDelivery"}
};

#define	NUM_FIELDS	3

/* Insertion Struct for DeleteGroupRecipient statement */
static EWDB_OCIStatementStruct SSStatement;

static int Local_iRetCode;
static int Local_idGroup;
static int LocalidRD;

static int PrepDeleteGroupRecipientExec(EWDBid IN_idGroup, EWDBid IN_idRecipientDelivery, EWDB_Cursor *ppCursor);
static int PostDeleteGroupRecipientExec();
static int InitDeleteGroupRecipientStatement(char *statement, EWDB_OCIStatementStruct *pSS);

int ewdb_api_DeleteGroupRecipient(EWDBid IN_idGroup, EWDBid IN_idRecipientDelivery)
{
	EWDB_Cursor pCursor;
	int rc;

	if (IN_idRecipientDelivery <= 0 || IN_idGroup <= 0)
	{
		logit ("", "ewdb_api_DeleteGroupRecipient(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_DeleteGroupRecipient(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepDeleteGroupRecipientExec (IN_idGroup, IN_idRecipientDelivery, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_DeleteGroupRecipient(): PrepDeleteGroupRecipientExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeleteGroupRecipient(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeleteGroupRecipient(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
	rc = PostDeleteGroupRecipientExec();
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "ewdb_api_DeleteGroupRecipient(): PostDeleteGroupRecipientExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
	else if(rc == EWDB_RETURN_WARNING)
	{
		return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
} /* end ewdb_api_DeleteGroupRecipient() */


static int InitDeleteGroupRecipientStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "InitDeleteGroupRecipientStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idGroup;
	pSS->FieldArray[2].pVal = &LocalidRD;

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitDeleteGroupRecipientStatement(): Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


static int PrepDeleteGroupRecipientExec(EWDBid IN_idGroup, EWDBid IN_idRecipientDelivery, EWDB_Cursor *ppCursor)
{
	if (ppCursor == NULL)
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incoming struct into the local struct */
	LocalidRD = IN_idRecipientDelivery;
	Local_idGroup = IN_idGroup;

	if(InitDeleteGroupRecipientStatement (SQL_STRING, &SSStatement)
		 != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepDeleteGroupRecipientExec(): InitDeleteGroupRecipientStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


static int PostDeleteGroupRecipientExec()
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor (pCursor);

	if(Local_iRetCode != 0)
	{
		logit("","PostDeleteGroupRecipientExec():  SQL Proc DeleteGroupRecipient() returned "
				"the following error(%d).  Please see that proc for details.\n",
				Local_iRetCode);
		return(EWDB_RETURN_WARNING);
	}

	return (EWDB_RETURN_SUCCESS);
}
