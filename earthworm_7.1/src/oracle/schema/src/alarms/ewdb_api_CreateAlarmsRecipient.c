/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmsRecipient.c,v 1.4 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmsRecipient.c,v $
 *     Revision 1.4  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2005/03/03 17:49:32  mark
 *     Added bActive flag to recipient struct
 *
 *     Revision 1.2  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.1  2001/07/31 20:44:40  lucky
 *     Initial revision
 *
 *     Revision 1.3  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.2  2001/05/15 18:47:40  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:24:59  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
	"Begin CreateAlarmsRecipient (OUT_idRecipient => :OUT_idRecipient,"
	"IN_dPriority => :IN_dPriority, IN_sDescription => :IN_sDescription,"
	"IN_bActive => :IN_bActive); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idRecipient"},
  {0,1,0,0,0,OA_INT,    ":IN_dPriority"},
  {0,1,256,0,0,OA_SZ,   ":IN_sDescription"},
  {0,1,0,0,0,OA_INT,    ":IN_bActive"},
};

#define	NUM_FIELDS	4



static EWDB_OCIStatementStruct SSStatement;

static  EWDB_AlarmsRecipientStruct	Local_RecipientStruct;

static int PrepCreateRecipientExec (EWDB_AlarmsRecipientStruct *, EWDB_Cursor *);
static int PostCreateRecipientExec (EWDB_AlarmsRecipientStruct *);
static int InitCreateRecipientStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateAlarmsRecipient (EWDB_AlarmsRecipientStruct *pRecipient)
{

	EWDB_Cursor	pCursor;

	if (pRecipient == NULL)
	{
		logit ("", "ewdb_api_CreateAlarmsRecipient(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();


	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_CreateAlarmsRecipient(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateRecipientExec (pRecipient, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsRecipient(): PrepCreateRecipientExec failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, 
				pCursor,"ewdb_api_CreateAlarmsRecipient(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC, 
				"ewdb_api_CreateAlarmsRecipient(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (PostCreateRecipientExec (pRecipient) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsRecipient(): PostCreateRecipientExec failed!\n");
		return(EWDB_RETURN_FAILURE);
	}

	return EWDB_RETURN_SUCCESS;
} /* end ewdb_api_CreateAlarmsRecipient() */


/******************* InitCreateRecipientStatement *******************/
static int InitCreateRecipientStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) ||  (pSS == NULL))
	{
		logit ("", "InitCreateRecipientStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(Local_RecipientStruct.idRecipient);
	pSS->FieldArray[1].pVal = &(Local_RecipientStruct.dPriority);
	pSS->FieldArray[2].pVal = (Local_RecipientStruct.sDescription);
	pSS->FieldArray[3].pVal = &(Local_RecipientStruct.bActive);

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
	{
		logit ("", "InitCreateRecipientStatement(): ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}  /* end InitCreateRecipientStatement() */


/******************* PrepCreateRecipientExec *******************/
static int PrepCreateRecipientExec (EWDB_AlarmsRecipientStruct *pRecipient, EWDB_Cursor *ppCursor)
{

	if ((pRecipient == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateRecipientExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	memcpy (&Local_RecipientStruct, pRecipient, sizeof (EWDB_AlarmsRecipientStruct));

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	if (InitCreateRecipientStatement (SQL_STRING, &SSStatement)
												     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateRecipientExec(): InitCreateRecipientStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateRecipientExec() */


/******************* PostCreateRecipientExec *******************/
static int PostCreateRecipientExec (EWDB_AlarmsRecipientStruct *pRecipient)
{
	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor (pCursor);
  
	if (pRecipient == NULL)
	{
		logit ("", "PostCreateRecipientExec(): Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}
  
	pRecipient->idRecipient = Local_RecipientStruct.idRecipient;
  if(Local_RecipientStruct.idRecipient <= 0)
  {
		logit("", "PostCreateRecipientExec(): SQL Proc CreateAlarmsRecipient() returned error:%d.\n", 
          Local_RecipientStruct.idRecipient);
    return(EWDB_RETURN_WARNING);
  }
  
	return EWDB_RETURN_SUCCESS;
}  /* end PostCreateRecipientExec() */
