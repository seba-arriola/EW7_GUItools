/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateGroupRecipient.c,v 1.3 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateGroupRecipient.c,v $
 *     Revision 1.3  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2005/02/03 21:18:34  mark
 *     Renamed idRecipient to idRecipientDelivery to make things clearer
 *
 *     Revision 1.1  2005/02/03 20:43:00  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_GroupRecipient(OUT_idGroupRecipient => :OUT_idGroupRecipient,"
		"IN_idGroup => :IN_idGroup,"
		"IN_idRecipientDelivery => :IN_idRecipientDelivery,"
		"IN_bActive => :IN_bActive); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idGroupRecipient"},
  {0,1,0,0,0,OA_INT,":IN_idGroup"},
  {0,1,0,0,0,OA_INT,":IN_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,":IN_bActive"},
};

#define	NUM_FIELDS	4

/* Insertion Struct for CreatePolygon szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	EWDB_AlarmsGroupRecipientStruct	LocalGroupRecipientStruct;

static int PrepCreateGroupRecipientExec(EWDB_AlarmsGroupRecipientStruct *pGroupRecipient, EWDB_Cursor *ppCursor);
static int PostCreateGroupRecipientExec(EWDB_AlarmsGroupRecipientStruct *pGroupRecipient);
static int InitCreateGroupRecipientStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

int ewdb_api_CreateGroupRecipient(EWDB_AlarmsGroupRecipientStruct *pGroupRecipient)
{
	EWDB_Cursor pCursor;
	int rc;

	if (pGroupRecipient == NULL)
	{
		logit ("", "ewdb_api_CreateGroupRecipient(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_CreateGroupRecipient(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateGroupRecipientExec (pGroupRecipient, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateGroupRecipient(): PrepCreateGroupRecipientExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateGroupRecipient(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreateGroupRecipient(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
	rc = PostCreateGroupRecipientExec(pGroupRecipient);
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "ewdb_api_CreateGroupRecipient(): PostCreateGroupRecipientExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
	else if(rc == EWDB_RETURN_WARNING)
	{
		return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateGroupRecipient() */


static int InitCreateGroupRecipientStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	if ((szStatement == NULL) || (pSS == NULL))
	{
		logit ("", "InitCreateGroupRecipientStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(LocalGroupRecipientStruct.idGroupRecipient);
	pSS->FieldArray[1].pVal = &(LocalGroupRecipientStruct.idGroup);
	pSS->FieldArray[2].pVal = &(LocalGroupRecipientStruct.idRecipientDelivery);
	pSS->FieldArray[3].pVal = &(LocalGroupRecipientStruct.bActive);

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
	{
		logit ("", "InitCreateGroupRecipientStatement(): Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


static int PrepCreateGroupRecipientExec(EWDB_AlarmsGroupRecipientStruct *pGroupRecipient, EWDB_Cursor *ppCursor)
{
	if ((pGroupRecipient == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateGroupRecipientExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incoming struct into the local struct */
	memcpy (&LocalGroupRecipientStruct, pGroupRecipient, sizeof(EWDB_AlarmsGroupRecipientStruct));

	if(InitCreateGroupRecipientStatement (SQL_STRING, &SSStatement)
		 != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateGroupRecipientExec(): InitCreateGroupRecipientStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


static int PostCreateGroupRecipientExec(EWDB_AlarmsGroupRecipientStruct *pGroupRecipient)
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;

  if (pGroupRecipient == NULL)
	{
		logit ("", "PostCreateGroupRecipientExec(): Invalid arguments passed in.\n");
    ewdb_base_ReleaseCursor (pCursor);
		return EWDB_RETURN_FAILURE;
	}

	pGroupRecipient->idGroupRecipient = LocalGroupRecipientStruct.idGroupRecipient;
	ewdb_base_ReleaseCursor (pCursor);

	if(pGroupRecipient->idGroupRecipient <= 0)
	{
		if(pGroupRecipient->idGroupRecipient == -1)
		{
			logit("","PostCreateGroupRecipientExec():  SQL Proc Create_GroupRecipient() returned "
					"\"Unknown error\".  See SQL debug table for details.\n");
		}
		else
		{
			logit("","PostCreateGroupRecipientExec():  SQL Proc Create_GroupRecipient() returned "
					"the following error(%d).  Please see that proc for details.\n",
					pGroupRecipient->idGroupRecipient);
		}
		return(EWDB_RETURN_WARNING);
	}

	return (EWDB_RETURN_SUCCESS);
}  /* end PostCreateGroupRecipientExec() */
